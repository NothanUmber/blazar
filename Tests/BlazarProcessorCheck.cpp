#include "../Source/Processor.h"
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
void require(bool condition, const char* message) {
    if (!condition)
        throw std::runtime_error(message);
}

std::vector<float> parameterValues(Processor& processor) {
    std::vector<float> result;
    for (auto* parameter : processor.getParameters())
        result.push_back(parameter->getValue());
    return result;
}

juce::ValueTree savedState(Processor& processor) {
    juce::MemoryBlock data;
    processor.getStateInformation(data);
    return juce::ValueTree::fromXml(*juce::AudioProcessor::getXmlFromBinary(data.getData(), int(data.getSize())));
}

juce::ValueTree parameterNode(juce::ValueTree tree, const juce::String& id) {
    for (auto child : tree)
        if (child["id"].toString() == id)
            return child;
    throw std::runtime_error("Missing parameter in test state");
}

void restore(Processor& processor, const juce::ValueTree& tree) {
    juce::MemoryBlock data;
    juce::AudioProcessor::copyXmlToBinary(*tree.createXml(), data);
    processor.setStateInformation(data.getData(), int(data.getSize()));
}

void checkStateValidation(Processor& processor) {
    processor.setCurrentProgram(28);
    const auto original = savedState(processor);
    const auto expected = parameterValues(processor);
    const auto revision = processor.getProgramRevision();
    auto reject = [&](const juce::ValueTree& tree) {
        restore(processor, tree);
        require(parameterValues(processor) == expected, "Invalid state changed parameters");
        require(processor.getCurrentProgram() == 28, "Invalid state changed factory program");
        require(processor.getProgramRevision() == revision, "Invalid state announced a recall");
    };

    for (auto* parameter : processor.getParameters()) {
        auto* identified = dynamic_cast<juce::AudioProcessorParameterWithID*>(parameter);
        require(identified != nullptr, "Parameter has no ID");
        auto invalid = original.createCopy();
        invalid.setProperty("factoryProgram", 3, nullptr);
        // A valid edit before the invalid value must not be partially applied.
        parameterNode(invalid, "matter").setProperty("value", .123, nullptr);
        parameterNode(invalid, identified->paramID).setProperty("value", "nan", nullptr);
        reject(invalid);
    }
    for (const char* value : {"inf", "-inf", "1e999", "1e999999999999999999999", "", ".", "+", "1e", "0.5junk", "-1", "5"}) {
        auto invalid = original.createCopy();
        parameterNode(invalid, "pulseRate").setProperty("value", value, nullptr);
        reject(invalid);
    }
    for (const char* value : {"nan", "inf", "1e999", "not a number"}) {
        auto invalid = original.createCopy();
        invalid.setProperty("factoryProgram", value, nullptr);
        reject(invalid);
    }
    auto duplicate = original.createCopy();
    duplicate.appendChild(parameterNode(duplicate, "matter").createCopy(), nullptr);
    reject(duplicate);

    auto legacy = original.createCopy();
    legacy.removeProperty("pulseRateContinuous", nullptr);
    parameterNode(legacy, "pulseRate").setProperty("value", "nan", nullptr);
    reject(legacy);

    // XML round trips must accept exact float endpoints, scientific notation,
    // and the discrete pulse-rate values used by older sessions.
    for (float endpoint : {0.f, 1.f}) {
        for (auto* parameter : processor.getParameters())
            parameter->setValueNotifyingHost(endpoint);
        const auto edge = savedState(processor);
        const auto edgeValues = parameterValues(processor);
        processor.setCurrentProgram(0);
        restore(processor, edge);
        require(parameterValues(processor) == edgeValues, "Parameter endpoint state round trip");
    }
    static constexpr float legacyRates[] = {.25f, .5f, 1.f, 2.f, 3.f, 4.f};
    for (int index = 0; index < 6; ++index) {
        legacy = original.createCopy();
        legacy.removeProperty("pulseRateContinuous", nullptr);
        parameterNode(legacy, "pulseRate").setProperty("value", index, nullptr);
        restore(processor, legacy);
        require(std::abs(processor.parameters().getRawParameterValue("pulseRate")->load() - legacyRates[index]) < 1e-5f, "Legacy pulse-rate migration");
    }
    auto compatible = original.createCopy();
    compatible.removeChild(parameterNode(compatible, "plasmaRate"), nullptr);
    parameterNode(compatible, "pulseRate").setProperty("value", " 1.25e+0 ", nullptr);
    restore(processor, compatible);
    require(std::abs(processor.parameters().getRawParameterValue("pulseRate")->load() - 1.25f) < 1e-5f, "Older state with missing parameter");

    const auto beforeRecall = processor.getProgramRevision();
    processor.setCurrentProgram(processor.getCurrentProgram());
    require(processor.getProgramRevision() != beforeRecall, "Same-index recall must announce a change");
    const auto beforeRestore = processor.getProgramRevision();
    restore(processor, savedState(processor));
    require(processor.getProgramRevision() != beforeRestore, "State restore must announce a change");
    std::cout << "PASS state validation, atomic rejection, endpoints, legacy rates and recall revisions\n";
}

void checkMidiChannels(Processor& processor) {
    juce::AudioBuffer<float> audio(2, 256);
    juce::MidiBuffer midi;
    auto render = [&](int blocks) {
        for (int i = 0; i < blocks; ++i) {
            processor.processBlock(audio, midi);
            for (int ch = 0; ch < 2; ++ch)
                for (int sample = 0; sample < audio.getNumSamples(); ++sample)
                    require(std::isfinite(audio.getSample(ch, sample)), "Non-finite processor audio");
        }
    };
    auto start = [&] {
        processor.setCurrentProgram(39);
        processor.parameters().getParameter("volume")->setValueNotifyingHost(1.f);
        processor.parameters().getParameter("release")->setValueNotifyingHost(0.f);
        processor.parameters().getParameter("fieldRelease")->setValueNotifyingHost(0.f);
        processor.reset();
        midi.addEvent(juce::MidiMessage::noteOn(1, 60, juce::uint8(100)), 0);
        midi.addEvent(juce::MidiMessage::noteOn(2, 67, juce::uint8(100)), 0);
        render(100);
        require(processor.voices() == 2, "Two-channel setup");
    };

    start();
    midi.addEvent(juce::MidiMessage::allSoundOff(1), 0);
    render(1);
    require(processor.voices() == 1, "CC120 stopped another channel");
    render(100);
    require(audio.getMagnitude(0, 256) > 1e-7f, "Other channel became silent after CC120");
    midi.addEvent(juce::MidiMessage::allSoundOff(2), 0);
    render(1);
    require(processor.voices() == 0 && audio.getMagnitude(0, 256) == 0.f, "CC120 did not silence the last channel and effects");

    start();
    midi.addEvent(juce::MidiMessage::controllerEvent(2, 64, 127), 0);
    midi.addEvent(juce::MidiMessage::noteOff(2, 67), 0);
    midi.addEvent(juce::MidiMessage::allNotesOff(1), 0);
    render(1000);
    require(processor.voices() == 1, "CC123 released another channel or its sustain");
    require(audio.getMagnitude(0, 256) > 1e-7f, "Sustained other channel became silent after CC123");
    midi.addEvent(juce::MidiMessage::controllerEvent(2, 64, 0), 0);
    render(1000);
    require(processor.voices() == 0, "Other channel did not release on pedal up");

    // Stolen-note tails are also channel-specific. Silence the new channel's
    // voices, leaving an old channel-2 tail, then silence that tail with CC120.
    processor.reset();
    for (int note = 60; note < 68; ++note)
        midi.addEvent(juce::MidiMessage::noteOn(2, note, juce::uint8(100)), 0);
    render(100);
    midi.addEvent(juce::MidiMessage::allNotesOff(2), 0);
    render(1);
    for (int note = 40; note < 48; ++note) {
        midi.addEvent(juce::MidiMessage::noteOn(1, note, juce::uint8(100)), 0);
        render(1);
    }
    audio.setSize(2, 64); // Observe the tail before its 8 ms fade completes.
    midi.addEvent(juce::MidiMessage::allSoundOff(1), 0);
    render(1);
    require(processor.voices() == 0 && audio.getMagnitude(0, 64) > 0.f, "CC120 stopped another channel's stolen-note tail");
    midi.addEvent(juce::MidiMessage::allSoundOff(2), 0);
    render(1);
    require(processor.voices() == 0 && audio.getMagnitude(0, 64) == 0.f, "CC120 left stolen-note tails sounding");
    std::cout << "PASS channel-scoped CC120/CC123, sustain, stolen-note tails and finite audio\n";
}
}

void runProcessorChecks(Processor& processor) {
    checkStateValidation(processor);
    checkMidiChannels(processor);
}
