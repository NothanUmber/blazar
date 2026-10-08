#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "Engine.h"
#include "Parameters.h"
#include <limits>
#include <locale>
#include <sstream>
class Processor final:public juce::AudioProcessor{
 quasar::Engine engine;
 juce::AudioProcessorValueTreeState state;
 std::array<std::atomic<float>*,quasar::parameterCount> values{};
 std::atomic<int> program{0};juce::MidiKeyboardState keyboard;
 // A recall is significant even when the factory program number stays the same.
 std::atomic<uint32_t> programRevision{0};
 std::atomic<float>* volumeValue=nullptr;
 juce::SmoothedValue<float,juce::ValueSmoothingTypes::Linear> volumeGain;
 std::array<std::atomic<float>,2> meterPeak{},meterRms{};
 std::atomic<int> voiceCount{0},pitchUi{8192},modUi{0};
 std::atomic<int> pendingPitch{-1},pendingMod{-1};
 static_assert(std::atomic<float>::is_always_lock_free && std::atomic<int>::is_always_lock_free,"Meter/expression atomics must be lock-free");
 static juce::AudioProcessorValueTreeState::ParameterLayout layout(){
  juce::AudioProcessorValueTreeState::ParameterLayout l;auto defaults=quasar::patch(0);
  for(auto& item:quasar::floatParameters){
   juce::NormalisableRange<float> range(0,1);
   if((juce::String(item.id)=="attack"||juce::String(item.id)=="fieldAttack")){range={.01f,4.f};range.setSkewForCentre(.7f);}
   if((juce::String(item.id)=="release"||juce::String(item.id)=="fieldRelease")){range={.1f,12.f};range.setSkewForCentre(3.f);}
   if(juce::String(item.id)=="pulseRate")range=juce::NormalisableRange<float>(.25f,4.f,[](float lo,float hi,float x){return lo*std::pow(hi/lo,x);},[](float lo,float hi,float x){return std::log(x/lo)/std::log(hi/lo);});
   l.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{item.id,4},item.name,range,float(defaults.*item.member)));
  }
  for(auto& item:quasar::choiceParameters){
   auto choices=juce::StringArray::fromTokens(item.choices,"|","");
   l.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{item.id,4},item.name,choices,defaults.*item.member));
  }
  l.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"volume",5},"MASTER VOLUME",juce::NormalisableRange<float>{-60.f,0.f},0.f));
  return l;
 }
 static bool readFiniteNumber(const juce::var& property,double& value){
  // XML stores numeric properties as strings. Require the entire string to be
  // a finite number, rather than accepting a numeric prefix or NaN/Infinity.
  std::istringstream input(property.toString().trim().toStdString());
  input.imbue(std::locale::classic());
  input>>value;
  return !input.fail()&&input.eof()&&std::isfinite(value);
 }
 void handle(const juce::MidiMessage& m){
  int ch=std::clamp(m.getChannel()-1,0,15);
  if(m.isNoteOn())engine.noteOn(m.getNoteNumber(),ch,m.getFloatVelocity());
  else if(m.isNoteOff())engine.noteOff(m.getNoteNumber(),ch);
  else if(m.isAllSoundOff())engine.silenceChannel(ch);
  else if(m.isAllNotesOff())engine.releaseChannel(ch);
  else if(m.isPitchWheel()){engine.bend(ch,(m.getPitchWheelValue()-8192)/8192.);if(ch==0)pitchUi=m.getPitchWheelValue();}
  else if(m.isChannelPressure())engine.pressure(ch,m.getChannelPressureValue()/127.);
  else if(m.isController()){
   if(m.getControllerNumber()==64)engine.sustain(ch,m.getControllerValue()>=64);
   else if(m.getControllerNumber()==1){engine.wheel(ch,m.getControllerValue()/127.);if(ch==0)modUi=m.getControllerValue();}
  }
 }

public:
 Processor():AudioProcessor(BusesProperties().withOutput("Stereo",juce::AudioChannelSet::stereo(),true)),state(*this,nullptr,"BlazarGui5",layout()){
  volumeValue=state.getRawParameterValue("volume");
  for(int i=0;i<quasar::floatCount;++i)values[i]=state.getRawParameterValue(quasar::floatParameters[i].id);
  for(int i=0;i<quasar::choiceCount;++i)values[quasar::floatCount+i]=state.getRawParameterValue(quasar::choiceParameters[i].id);
 }
 const juce::String getName()const override{return "Blazar";}
 bool acceptsMidi()const override{return true;}bool producesMidi()const override{return false;}bool isMidiEffect()const override{return false;}
 double getTailLengthSeconds()const override{return 60;}
 int getNumPrograms()override{return quasar::programCount;}int getCurrentProgram()override{return program.load();}
 uint32_t getProgramRevision()const{return programRevision.load();}
 const juce::String getProgramName(int i)override{return quasar::patchName(i);}
 void changeProgramName(int,const juce::String&)override{}
 void setCurrentProgram(int i)override{
  const int selectedProgram=std::clamp(i,0,quasar::programCount-1);
  auto p=quasar::patch(selectedProgram);
  for(auto& item:quasar::floatParameters){auto* a=state.getParameter(item.id);a->setValueNotifyingHost(a->convertTo0to1(float(p.*item.member)));}
  for(auto& item:quasar::choiceParameters)state.getParameter(item.id)->setValueNotifyingHost(float(p.*item.member)/(item.count-1));
  program=selectedProgram;
  ++programRevision;
 }
 bool isBusesLayoutSupported(const BusesLayout& b)const override{return b.getMainInputChannelSet().isDisabled()&&b.getMainOutputChannelSet()==juce::AudioChannelSet::stereo();}
 void prepareToPlay(double sr,int)override{engine.prepare(sr);keyboard.reset();volumeGain.reset(sr,.02);volumeGain.setCurrentAndTargetValue(gainForDb(volumeValue->load()));clearIndicators();}
 void releaseResources()override{engine.reset();}void reset()override{engine.reset();keyboard.reset();clearIndicators();volumeGain.setCurrentAndTargetValue(gainForDb(volumeValue->load()));}
 void processBlock(juce::AudioBuffer<float>& audio,juce::MidiBuffer& midi)override{
  juce::ScopedNoDenormals guard;audio.clear();quasar::Settings p;
  for(int i=0;i<quasar::floatCount;++i)p.*quasar::floatParameters[i].member=values[i]->load();
  for(int i=0;i<quasar::choiceCount;++i)p.*quasar::choiceParameters[i].member=int(std::round(values[quasar::floatCount+i]->load()));
  engine.setSettings(p);
  volumeGain.setTargetValue(gainForDb(volumeValue->load()));
  int pitch=pendingPitch.exchange(-1),mod=pendingMod.exchange(-1);
  if(pitch>=0)handle(juce::MidiMessage::pitchWheel(1,pitch));
  if(mod>=0)handle(juce::MidiMessage::controllerEvent(1,1,mod));
  std::array<float,2> peaks{};std::array<double,2> squares{};
  if(auto* playhead=getPlayHead())if(auto pos=playhead->getPosition())if(auto bpm=pos->getBpm())engine.setTempo(*bpm);
  keyboard.processNextMidiBuffer(midi,0,audio.getNumSamples(),true);
  int position=0;auto render=[&](int end){for(;position<end;++position){auto x=engine.tick();float gain=volumeGain.getNextValue();float samples[]={float(x.l)*gain,float(x.r)*gain};
   for(int ch=0;ch<2;++ch){audio.setSample(ch,position,samples[ch]);peaks[ch]=std::max(peaks[ch],std::abs(samples[ch]));squares[ch]+=double(samples[ch])*samples[ch];}}};
  for(const auto event:midi){render(std::clamp(event.samplePosition,position,audio.getNumSamples()));handle(event.getMessage());}
  render(audio.getNumSamples());midi.clear();
  for(int ch=0;ch<2;++ch){meterRms[ch].store(float(std::sqrt(squares[ch]/std::max(1,audio.getNumSamples()))),std::memory_order_relaxed);
   float previous=meterPeak[ch].load(std::memory_order_relaxed);
   while(previous<peaks[ch]&&!meterPeak[ch].compare_exchange_weak(previous,peaks[ch],std::memory_order_relaxed)){} }
  voiceCount.store(engine.activeVoices(),std::memory_order_relaxed);
 }
 bool hasEditor()const override{return true;}juce::AudioProcessorEditor* createEditor()override;
 juce::AudioProcessorValueTreeState& parameters(){return state;}
 juce::MidiKeyboardState& keyboardState(){return keyboard;}
 float takePeak(int ch){return meterPeak[ch].exchange(0,std::memory_order_relaxed);}
 float rms(int ch)const{return meterRms[ch].load(std::memory_order_relaxed);}
 int voices()const{return voiceCount.load(std::memory_order_relaxed);}
 int pitchValue()const{return pitchUi.load();}int modValue()const{return modUi.load();}
 void uiPitch(int value){pendingPitch=std::clamp(value,0,16383);pitchUi=value;}
 void uiMod(int value){pendingMod=std::clamp(value,0,127);modUi=value;}
 void initialise(){auto p=quasar::Settings{};p.pulseRate=1;
  for(auto& item:quasar::floatParameters){auto* a=state.getParameter(item.id);a->setValueNotifyingHost(a->convertTo0to1(float(p.*item.member)));}
  for(auto& item:quasar::choiceParameters)state.getParameter(item.id)->setValueNotifyingHost(float(p.*item.member)/(item.count-1));program=0;
  ++programRevision;
 }
 static float gainForDb(float db){return db<=-60.f?0.f:juce::Decibels::decibelsToGain(db);}
 void clearIndicators(){voiceCount=0;for(auto& x:meterPeak)x=0;for(auto& x:meterRms)x=0;pitchUi=8192;modUi=0;pendingPitch=-1;pendingMod=-1;}

 void getStateInformation(juce::MemoryBlock& b)override{auto copy=state.copyState();copy.setProperty("factoryProgram",program.load(),nullptr);copy.setProperty("pulseRateContinuous",true,nullptr);copyXmlToBinary(*copy.createXml(),b);}
 void setStateInformation(const void* data,int size)override{
  auto xml=getXmlFromBinary(data,size);
  if(!xml||!xml->hasTagName("BlazarGui5"))return;
  auto tree=juce::ValueTree::fromXml(*xml);
  const bool legacyRate=!bool(tree.getProperty("pulseRateContinuous",false));
  juce::StringArray restoredIDs;

  // Validate and migrate the detached tree first. A bad parameter must not
  // partially replace the current sound or poison the audio thread with NaNs.
  // Missing parameters remain compatible with states from older versions.
  for(auto child:tree){
   const auto id=child.getProperty("id").toString();
   auto* parameter=state.getParameter(id);
   if(!parameter)continue;
   if(restoredIDs.contains(id))return;
   restoredIDs.add(id);
   double value=parameter->convertFrom0to1(parameter->getDefaultValue());
   if(child.hasProperty("value")&&!readFiniteNumber(child.getProperty("value"),value))return;
   if(legacyRate&&id=="pulseRate"){
    static constexpr float rates[]={.25f,.5f,1.f,2.f,3.f,4.f};
    // Clamp before converting to int, including very large legacy values.
    const int index=child.hasProperty("value")?int(std::clamp(value,0.,5.)):2;
    value=rates[index];
   }
   if(value<-std::numeric_limits<float>::max()||value>std::numeric_limits<float>::max())return;
   const float parameterValue=float(value);
   const auto& range=parameter->getNormalisableRange();
   if(parameterValue<range.start||parameterValue>range.end)return;
   child.setProperty("value",parameterValue,nullptr);
  }
  double restoredProgram=0;
  if(tree.hasProperty("factoryProgram")&&!readFiniteNumber(tree.getProperty("factoryProgram"),restoredProgram))return;
  tree.setProperty("pulseRateContinuous",true,nullptr);
  state.replaceState(tree);
  program=int(std::clamp(restoredProgram,0.,double(quasar::programCount-1)));
  ++programRevision;
 }
};
