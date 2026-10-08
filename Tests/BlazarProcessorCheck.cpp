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

}

void runProcessorChecks(Processor& processor) {
    checkStateValidation(processor);
}
