#include <juce_audio_utils/juce_audio_utils.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <chrono>
void check(bool x,const char* msg){if(!x)throw std::runtime_error(msg);}
struct EditorDelete {juce::AudioProcessor* processor;void operator()(juce::AudioProcessorEditor* e)const{if(e){processor->editorBeingDeleted(e);delete e;}}};
using EditorOwner=std::unique_ptr<juce::AudioProcessorEditor,EditorDelete>;
struct Host {
 juce::VST3PluginFormat format;juce::OwnedArray<juce::PluginDescription> types;
 std::unique_ptr<juce::AudioPluginInstance> plugin;
 juce::AudioBuffer<float> audio{2,256};juce::MidiBuffer midi;
 double sr=48000,peak=0;size_t frames=0;
 explicit Host(const char* path){
  format.findAllTypesForFile(types,path);check(types.size()==1&&types[0]->isInstrument,"instrument discovery");
  juce::String error;plugin=format.createInstanceFromDescription(*types[0],sr,256,error);
  if(!plugin)throw std::runtime_error(error.toStdString());
  check(plugin->acceptsMidi()&&plugin->getTotalNumInputChannels()==0&&plugin->getTotalNumOutputChannels()==2,"MIDI stereo buses");
  check(plugin->getNumPrograms()==40,"40 programs");plugin->prepareToPlay(sr,256);
 }
 void reset(int program){plugin->reset();plugin->setCurrentProgram((program+1)%plugin->getNumPrograms());plugin->setCurrentProgram(program);midi.clear();block();plugin->reset();midi.clear();}
 juce::AudioProcessorParameter* parameter(const char* name){for(auto* p:plugin->getParameters())if(p->getName(64)==name)return p;throw std::runtime_error(std::string("Missing parameter: ")+name);}
 void set(const char* name,float x){parameter(name)->setValueNotifyingHost(x);}
 void block(){plugin->processBlock(audio,midi);for(int n=0;n<audio.getNumSamples();++n)for(int ch=0;ch<2;++ch){double x=audio.getSample(ch,n);check(std::isfinite(x)&&std::abs(x)<.951,"finite bounded plugin audio");peak=std::max(peak,std::abs(x));}frames+=256;}
 void wait(double sec){for(int n=0;n<int(sr*sec/256);++n)block();}
 void chord(){for(int n:{48,55,60,64})midi.addEvent(juce::MidiMessage::noteOn(1,n,juce::uint8(100)),41);}
 void off(){midi.addEvent(juce::MidiMessage::allNotesOff(1),0);}
 void render(const std::string& name,double seconds,double held){
  std::ofstream out(name+".f32",std::ios::binary);check(bool(out),"render file");double energy=0,stereo=0,tail=0;
  int blocks=int(sr*seconds/256),offBlock=int(sr*held/256);
  for(int b=0;b<blocks;++b){
   if(b==offBlock)off();block();
   for(int n=0;n<256;++n){float l=audio.getSample(0,n),r=audio.getSample(1,n);out.write(reinterpret_cast<char*>(&l),4);out.write(reinterpret_cast<char*>(&r),4);
    if(b<offBlock){energy+=l*l+r*r;stereo+=(l-r)*(l-r);}
    if(b>blocks-int(sr/256))tail+=l*l+r*r;
    if(b==0&&n<41)check(l==0&&r==0,"sample-offset MIDI");
   }
  }
  check(energy>.0001,"audible chord");
  if(held<seconds-12)check(tail<energy*.001,"release tail");
  std::cout<<name<<" energy "<<energy<<" stereo "<<stereo<<" last-second energy "<<tail<<std::endl;
 }
};

int main(int argc,char** argv){try{
 juce::ScopedJuceInitialiser_GUI init;check(argc==2||argc==3,"bundle path");Host h(argv[1]);std::string mode=argc==3?argv[2]:"all";
 std::cout<<"DISCOVERED "<<h.types[0]->name<<" instrument stereo, parameters "<<h.plugin->getParameters().size()<<", 40 programs"<<std::endl;
 if(mode=="--pulse-balance-smoke"){
  auto* rate=h.parameter("PULSE RATE");check(rate->getNumSteps()>1000,"genuinely continuous rate parameter");
  for(float rateValue:{.25f,1.137f,4.f})for(float depth:{0.f,.4f,1.f}){
   h.reset(0);h.set("MATTER",0);h.set("FIELD",1);h.set("PARTICLES",0);h.set("PLASMA",0);h.set("SPACE",0);h.set("EVOLVE",0);h.set("PULSE",depth);rate->setValueNotifyingHost(float(std::log2(rateValue/.25)/4));h.plugin->reset();h.chord();h.wait(3);
   check(std::abs(rate->getText(rate->getValue(),64).getFloatValue()-rateValue)<1e-4,"arbitrary non-step rate preserved");
   std::cout<<"PULSE continuous rate "<<rateValue<<" depth "<<depth<<std::endl;
  }
  for(int i=0;i<100;++i){rate->setValueNotifyingHost(float(i)/99);h.block();}
  h.reset(28);h.chord();h.wait(2);h.off();h.wait(.2);
  std::cout<<"PASS quick actual VST3 continuous rates/depth/rate automation/Matter audio finite bounded peak "<<h.peak<<std::endl;return 0;
 }
 if(mode=="--living-smoke"){
  std::vector<float> reference;
  for(float speed:{0.f,1.f}){
   h.reset(0);h.set("MATTER",0);h.set("FIELD",1);h.set("PARTICLES",0);h.set("PLASMA",0);h.set("SPACE",0);h.set("PULSE",0);h.set("FIELD MODEL",.1f);h.set("EVOLVE",0);h.set("MOTION",speed);h.plugin->reset();h.chord();double difference=0;
   for(int b=0;b<2250;++b){h.block();for(int n=0;n<256;++n){float x=h.audio.getSample(0,n);if(speed==0)reference.push_back(x);else difference=std::max(difference,double(std::abs(x-reference[size_t(b)*256+n])));}}
   if(speed>0){check(difference<1e-6,"MOTION inactive at zero EVOLVE");std::cout<<"PASS EVOLVE zero, MOTION zero vs maximum: sample delta "<<difference<<std::endl;}h.off();h.wait(.1);
  }
  for(int program:{0,28,35})for(int setting=0;setting<3;++setting){
   const float motion[]={.08f,.5f,1.f},evolve[]={0.f,.4f,1.f};h.reset(program);h.set("MOTION",motion[setting]);h.set("EVOLVE",evolve[setting]);h.plugin->reset();h.chord();
   double minRms=10,maxRms=0,minShape=1e9,maxShape=0,previous=0;
   for(int window=0;window<40;++window){double energy=0,edge=0;for(int b=0;b<93;++b){h.block();for(int n=0;n<256;++n){double x=h.audio.getSample(0,n);energy+=x*x;edge+=(x-previous)*(x-previous);previous=x;}}
    if(window>5){double rms=std::sqrt(energy/(93*256)),shape=edge/std::max(1e-12,energy);minRms=std::min(minRms,rms);maxRms=std::max(maxRms,rms);minShape=std::min(minShape,shape);maxShape=std::max(maxShape,shape);}}
   check(maxRms>.0001,"held patch audible");h.off();h.wait(.5);
   std::cout<<"LIVING held 20s program "<<program<<" motion "<<motion[setting]<<" evolve "<<evolve[setting]<<" RMS "<<minRms<<".."<<maxRms<<" normalized spectral-motion proxy "<<minShape<<".."<<maxShape<<std::endl;
  }
  std::cout<<"PASS lightweight long low/mid/high holds, zero-evolve invariance, finite bounded peak "<<h.peak<<std::endl;return 0;
 }
 if(mode=="--continuous-plasma-smoke"){
  for(int model:{0,1,2,3,4,10}){
   h.reset(0);
   for(auto name:{"MATTER","FIELD","PARTICLES","SPACE","EVOLVE","MOTION","PULSE","TENSEGRITY"})h.set(name,0);
   h.set("PLASMA",1);h.set("PLASMA SELF",1);h.set("PLASMA DRIVE",0);h.set("PLASMA MODEL",model/11.f);
   for(auto name:{"PLASMA SCALE","PLASMA FLOW","PLASMA RATE","PLASMA COHERENCE","PLASMA INTERMITTENCY","PLASMA RESONANCE"})h.set(name,.5f);
   h.set("PLASMA PRESSURE",.7f);h.set("PLASMA TURBULENCE",.7f);
   h.plugin->reset();h.midi.addEvent(juce::MidiMessage::noteOn(1,60,juce::uint8(100)),0);
   double minRms=10,maxRms=0,minShape=10,maxShape=0,previous=0;
   for(int second=0;second<30;++second){
    double energy=0,edge=0;for(int b=0;b<187;++b){h.block();for(int n=0;n<256;++n){double x=h.audio.getSample(0,n);energy+=x*x;edge+=(x-previous)*(x-previous);previous=x;}}
    if(second>2){double rms=std::sqrt(energy/(187*256)),shape=edge/std::max(1e-12,energy);minRms=std::min(minRms,rms);maxRms=std::max(maxRms,rms);minShape=std::min(minShape,shape);maxShape=std::max(maxShape,shape);}
   }
   check(minRms>.00001,"continuous held medium remains audible");
   check(maxShape>minShape*1.08,"internal normalized spectral movement");
   std::cout<<"CONTINUOUS model "<<model<<" 30s RMS "<<minRms<<".."<<maxRms<<" normalized spectral proxy "<<minShape<<".."<<maxShape<<std::endl;
   h.off();h.wait(18);check(h.audio.getMagnitude(0,256)<1e-6,"Plasma release retires");
  }
  std::cout<<"PASS continuous medium, internal spectral evolution with global EVOLVE zero, release, finite bounded peak "<<h.peak<<std::endl;return 0;
 }
 if(mode=="--plasma12-smoke"){
  check(h.parameter("PLASMA MODEL")->getNumSteps()==12,"twelve Plasma models");
  for(double sampleRate:{48000.,96000.}){
   h.plugin->releaseResources();h.sr=sampleRate;h.plugin->prepareToPlay(sampleRate,256);
   for(int model=0;model<12;++model){
    h.reset(0);h.set("MATTER",0);h.set("FIELD",0);h.set("PARTICLES",0);h.set("PLASMA",1);h.set("SPACE",0);h.set("PLASMA SELF",1);h.set("PLASMA MODEL",model/11.f);h.set("PLASMA PRESSURE",.8f);h.set("PLASMA RATE",.65f);h.set("PLASMA COHERENCE",.6f);h.set("PLASMA INTERMITTENCY",.5f);h.set("PLASMA RESONANCE",.6f);h.chord();
    double energy=0;for(int b=0;b<int(sampleRate*1.5/256);++b){h.block();for(int n=0;n<256;++n)energy+=double(h.audio.getSample(0,n))*h.audio.getSample(0,n);}check(energy>1e-10,"model audible");
    for(float corner:{0.f,1.f}){h.set("PLASMA SCALE",corner);h.set("PLASMA TURBULENCE",corner);h.set("PLASMA RATE",corner);h.set("PLASMA COHERENCE",corner);h.set("PLASMA INTERMITTENCY",corner);h.set("PLASMA RESONANCE",corner);h.set("PLASMA PRESSURE",1);h.wait(.15);}
    h.set("PLASMA SELF",0);h.set("PLASMA DRIVE",1);h.set("MATTER",.5f);h.set("FIELD",.2f);h.set("TENSEGRITY",1);h.wait(.15);h.off();h.wait(.15);
    std::cout<<"PLASMA12 rate "<<sampleRate<<" model "<<model<<" energy "<<energy<<std::endl;
   }
  }
  std::cout<<"PASS lightweight actual VST3 all 12 Plasma models/control corners/SELF-DRIVE/release finite bounded peak "<<h.peak<<std::endl;return 0;
 }
 if(mode=="--evolution-smoke"){
  std::vector<float> stable,slow,fast;
  auto render=[&](float motion,float evolve,std::vector<float>& out){h.reset(0);h.set("MATTER",0);h.set("FIELD",1);h.set("PARTICLES",0);h.set("PLASMA",0);h.set("SPACE",0);h.set("FIELD MODEL",.2f);h.set("FIELD SHAPE",.4f);h.set("FIELD MODULATION",.4f);h.set("MOTION",motion);h.set("EVOLVE",evolve);h.chord();
   for(int b=0;b<2250;++b){h.block();for(int n=0;n<256;++n)out.push_back(h.audio.getSample(0,n));}h.off();h.wait(.25);
  };
  render(.5f,0,stable);render(.1f,1,slow);render(.9f,1,fast);
  auto distance=[](const std::vector<float>& a,const std::vector<float>& b){double d=0,e=0;for(size_t i=48000;i<a.size();++i){d+=(a[i]-b[i])*(a[i]-b[i]);e+=double(a[i])*a[i];}return std::sqrt(d/std::max(1e-12,e));};
  double ds=distance(stable,slow),df=distance(stable,fast),pace=distance(slow,fast);check(df>.05&&pace>.05,"evolution/pace audibly meaningful signal difference");
  std::cout<<"PASS short actual VST3 Motion/Evolve: stable-to-slow normalized delta "<<ds<<", stable-to-fast "<<df<<", slow-to-fast "<<pace<<", finite bounded peak "<<h.peak<<std::endl;return 0;
 }
 if(mode=="--plasma-smoke"){
  for(int model=0;model<3;++model){
   h.reset(0);h.set("MATTER",0);h.set("FIELD",0);h.set("PARTICLES",0);h.set("PLASMA",1);h.set("SPACE",0);h.set("PLASMA SELF",1);h.set("PLASMA MODEL",model/2.f);h.set("PLASMA PRESSURE",.8f);h.set("PLASMA FLOW",.5f);h.set("PLASMA SCALE",.65f);h.set("PLASMA TURBULENCE",.8f);
   h.chord();double minRms=1,maxRms=0;
   for(int window=0;window<24;++window){double energy=0;for(int b=0;b<46;++b){h.block();for(int n=0;n<256;++n)energy+=double(h.audio.getSample(0,n))*h.audio.getSample(0,n);}double rms=std::sqrt(energy/(46*256));if(window>2){minRms=std::min(minRms,rms);maxRms=std::max(maxRms,rms);}}
   check(maxRms>.001,"Plasma audible");std::cout<<"PLASMA model "<<model<<" moving RMS min "<<minRms<<" max "<<maxRms<<std::endl;
   for(float scale:{0.f,1.f})for(float turbulence:{0.f,1.f}){h.set("PLASMA SCALE",scale);h.set("PLASMA TURBULENCE",turbulence);h.set("PLASMA PRESSURE",1);h.wait(.5);}
   h.set("PLASMA SELF",0);h.set("PLASMA DRIVE",1);h.set("MATTER",.6f);h.set("FIELD",.3f);h.set("TENSEGRITY",1);h.wait(.5);h.off();h.wait(.5);
  }
  std::cout<<"PASS short actual-VST3 Plasma audio, control corners, SELF/DRIVE, release: finite bounded peak "<<h.peak<<std::endl;return 0;
 }
 if(mode=="--gain-smoke"){h.reset(0);h.chord();h.wait(2);check(h.peak>.001,"audible chord");h.off();h.wait(1);std::cout<<"PASS quick finite bounded actual VST3 MIDI audio peak "<<h.peak<<std::endl;return 0;}
 if(mode=="--gui"){
  check(h.types[0]->name=="Blazar","product identity");
  check(h.parameter("MASTER VOLUME")->getDefaultValue()==1,"unity volume default");
  check(h.parameter("PULSE RATE")->getNumSteps()>1000,"continuous tempo rate");
  for(int i=0;i<20;++i){
   h.reset(i*2);h.chord();h.wait(.04);
   EditorOwner editor(h.plugin->createEditorIfNeeded(),EditorDelete{h.plugin.get()});check(bool(editor),"native VST3 editor open");
   editor->setSize(i%2?1152:1536,i%2?768:1024);h.wait(.04);editor.reset();h.wait(.04);
  }
  h.reset(5);h.chord();h.wait(.2);h.set("MASTER VOLUME",0);h.wait(.1);check(h.audio.getMagnitude(0,256)==0,"master mute");
  h.set("MASTER VOLUME",1);h.wait(.1);check(h.audio.getMagnitude(0,256)>1e-5,"master unity");
  for(int rate=0;rate<6;++rate){h.reset(3);h.set("PULSE RATE",rate/5.f);h.set("PULSE",1);h.chord();h.wait(.5);}
  juce::MemoryBlock state;h.set("MASTER VOLUME",.7f);h.set("PULSE RATE",.2f);h.plugin->getStateInformation(state);
  h.set("MASTER VOLUME",1);h.set("PULSE RATE",1);h.plugin->setStateInformation(state.getData(),int(state.getSize()));
  check(std::abs(h.parameter("MASTER VOLUME")->getValue()-.7f)<1e-5,"actual volume restore");check(std::abs(h.parameter("PULSE RATE")->getValue()-.2f)<1e-5,"actual rate restore");
  std::cout<<"PASS actual native VST3 20 editor cycles, resize, audio closed/open, mute/unity, continuous rate values, new state"<<std::endl;
 }
 if(mode=="--compare"){
  auto oldPath=juce::File::getCurrentWorkingDirectory().getChildFile("../../build-dynamics/Quasar_artefacts/Release/VST3/Quasar.vst3");
  Host old(oldPath.getFullPathName().toRawUTF8());double maxDelta=0;int different=0;
  for(int p=0;p<40;++p){old.reset(p);h.reset(p);old.chord();h.chord();double delta=0;
   for(int b=0;b<400;++b){if(b==300){old.off();h.off();}old.block();h.block();for(int c=0;c<2;++c)for(int n=0;n<256;++n)delta=std::max(delta,std::abs(double(old.audio.getSample(c,n))-h.audio.getSample(c,n)));}
   maxDelta=std::max(maxDelta,delta);if(delta>1e-7)++different;std::cout<<"SOUND_COMPARE program "<<p<<" delta "<<delta<<std::endl;
  }
  check(different==0,"0.4 factory sound changed");std::cout<<"PASS all 40 Quasar 0.4 / Blazar program renders maxDelta "<<maxDelta<<std::endl;
 }
 if(mode=="--cpu"){
  for(double sr:{48000.,96000.})for(int f:{7,10})for(int ex:{2,3,4}){
   h.plugin->releaseResources();h.sr=sr;h.plugin->prepareToPlay(sr,256);h.reset(37);
   h.set("FIELD MODEL",f/10.f);h.set("MATTER EXCITER",ex/5.f);h.set("PARTICLE MODEL",1);
   for(auto n:{"MATTER","FIELD","PARTICLES","PLASMA","TENSEGRITY","ENERGY","SPACE","PARTICLE RATE","PARTICLE DECAY","Density","EXCITER FORCE","FIELD MODULATION"})h.set(n,1);
   for(int n=48;n<56;++n)h.midi.addEvent(juce::MidiMessage::noteOn(1,n,juce::uint8(127)),0);
   h.wait(1);auto start=std::chrono::steady_clock::now();h.wait(6);
   auto seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
   std::cout<<"PLUGIN_CPU rate "<<sr<<" voices 8 field "<<f<<" exciter "<<ex<<" fraction "<<seconds/6<<std::endl;
  }
 }
 if(mode=="--render"||mode=="all"){
  EditorOwner editor(h.plugin->createEditorIfNeeded(),EditorDelete{h.plugin.get()});check(editor&&editor->getHeight()>0,"custom editor / keyboard");editor.reset();
  for(int p=28;p<40;++p){h.reset(p);h.chord();h.render("audition-"+std::to_string(p)+"-"+h.plugin->getProgramName(p).replaceCharacter(' ','-').toStdString(),34,14);}
  h.reset(28);h.chord();h.render("bowed-object-long",60,40);
  h.reset(36);h.chord();h.render("invisible-storm-long",60,40);
  // Isolated Matter: sustained contact cannot borrow energy from any other world.
  for(int ex=0;ex<6;++ex){
   h.reset(39);h.set("MATTER EXCITER",ex/5.f);h.set("MATTER MODEL",1/7.f);
   for(auto n:{"FIELD","PARTICLES","PLASMA","TENSEGRITY","SPACE","EVOLVE","MOTION"})h.set(n,0);
   h.set("EXCITER FORCE",.65f);h.set("EXCITER SPEED",.4f);h.set("EXCITER HARDNESS",.5f);h.set("EXCITER POSITION",.27f);
   h.midi.addEvent(juce::MidiMessage::noteOn(1,60,juce::uint8(100)),41);h.render("contact-"+std::to_string(ex),24,12);
  }
  for(int f=9;f<11;++f){
   h.reset(33);h.set("FIELD MODEL",f/10.f);h.set("FIELD",1);
   for(auto n:{"MATTER","PARTICLES","PLASMA","TENSEGRITY","SPACE","EVOLVE","MOTION"})h.set(n,0);
   h.midi.addEvent(juce::MidiMessage::noteOn(1,60,juce::uint8(100)),41);h.render("dynamic-field-"+std::to_string(f),24,12);
  }
  for(int p=0;p<3;++p){
   h.reset(35);h.set("PLASMA MODEL",p/2.f);h.set("PLASMA",1);h.set("PLASMA SELF",1);h.set("PLASMA DRIVE",0);
   for(auto n:{"MATTER","FIELD","PARTICLES","TENSEGRITY","SPACE"})h.set(n,0);
   h.chord();h.render("plasma-self-"+std::to_string(p),24,12);
  }
  for(int c=0;c<2;++c){h.reset(34);h.set("TENSEGRITY",float(c));h.chord();h.render("bidirectional-"+std::to_string(c),24,12);}
  for(int c=0;c<2;++c){h.reset(30);h.set("PARTICLE IMPACT",float(c));h.chord();
   if(c)h.render("invisible-mallets-drive",24,12);
   else {h.wait(5);check(h.audio.getMagnitude(0,256)<1e-9,"inaudible particles impact zero");std::cout<<"PASS invisible particle off zero"<<std::endl;}
  }
  for(int c=0;c<2;++c){h.reset(36);h.set("FIELD",0);h.set("PLASMA DRIVE",float(c));h.chord();
   if(c)h.render("invisible-storm-drive",24,12);
   else {h.wait(5);check(h.audio.getMagnitude(0,256)<1e-9,"inaudible plasma drive zero");std::cout<<"PASS invisible Plasma off zero"<<std::endl;}
  }
  std::cout<<"PASS actual VST3 audition and controlled renders"<<std::endl;
 }
 if(mode=="--validate"||mode=="all"){
  // Both GUI and host choice names/counts reflect the actual build.
  check(h.parameter("MATTER EXCITER")->getNumSteps()==6,"six exciters");
  check(h.parameter("FIELD MODEL")->getNumSteps()==11,"eleven Field models");
  check(h.parameter("PLASMA MODEL")->getNumSteps()==12,"twelve Plasma models");
  double delta=0;
  for(int ex=0;ex<6;++ex){
   h.reset(28+ex);h.set("MATTER EXCITER",ex/5.f);h.set("PLASMA MODEL",(ex%12)/11.f);h.set("PLASMA",.8f);
   h.set("PARTICLE SELF",ex%2);h.set("PARTICLE IMPACT",1);h.set("FIELD MODEL",(ex%2?9:10)/10.f);h.block();
   std::vector<float> expected;for(int i=0;i<h.plugin->getParameters().size();++i)expected.push_back(h.plugin->getParameters()[i]->getValue());
   juce::MemoryBlock state;h.plugin->getStateInformation(state);
   h.plugin->reset();h.chord();std::vector<float> reference;
   for(int b=0;b<100;++b){h.block();for(int n=0;n<256;++n)for(int c=0;c<2;++c)reference.push_back(h.audio.getSample(c,n));}
   h.reset(0);h.plugin->setStateInformation(state.getData(),int(state.getSize()));h.block();
   for(int i=0;i<h.plugin->getParameters().size();++i)check(std::abs(expected[i]-h.plugin->getParameters()[i]->getValue())<1e-5,"all parameters restored");
   check(h.plugin->getCurrentProgram()==28+ex,"program restored");
   h.plugin->reset();h.chord();size_t index=0;
   for(int b=0;b<100;++b){h.block();for(int n=0;n<256;++n)for(int c=0;c<2;++c)delta=std::max(delta,std::abs(double(h.audio.getSample(c,n))-reference[index++]));}
  }
  check(delta<1e-6,"deterministic state replay");std::cout<<"PASS all parameters / six state replays delta "<<delta<<std::endl;
  int cases=0;
  for(double sr:{44100.,48000.,96000.}){
   h.plugin->releaseResources();h.sr=sr;h.plugin->prepareToPlay(sr,256);
   for(int ex=0;ex<6;++ex)for(int m=0;m<8;++m){
    h.reset(39);h.set("MATTER EXCITER",ex/5.f);h.set("MATTER MODEL",m/7.f);
    for(auto n:{"FIELD","PARTICLES","PLASMA","TENSEGRITY","SPACE"})h.set(n,0);
    h.set("EXCITER FORCE",ex%2?1:.1f);h.set("EXCITER SPEED",m%2);h.set("EXCITER HARDNESS",m%2);
    h.set("MATTER STRUCTURE",m%2);h.set("EXCITER POSITION",m%2?.97f:.03f);
    h.midi.addEvent(juce::MidiMessage::noteOn(1,36+(m%3)*24,juce::uint8(127)),0);h.wait(.5);h.off();h.wait(.2);++cases;
   }
   for(int f=0;f<11;++f)for(int plasma=0;plasma<12;++plasma){
    h.reset(37);h.set("FIELD MODEL",f/10.f);h.set("PLASMA MODEL",plasma/11.f);h.set("PARTICLE MODEL",(f%9)/8.f);
    for(auto n:{"FIELD","MATTER","PARTICLES","PLASMA","PLASMA DRIVE","PARTICLE IMPACT","ENERGY","TENSEGRITY","SPACE","EVOLVE","MOTION"})h.set(n,1);
    h.set("PARTICLE SELF",f%2);h.set("PLASMA SELF",plasma%2);h.chord();h.wait(.5);h.off();h.wait(.2);++cases;
   }
  }
  std::cout<<"PASS actual VST3 "<<cases<<" model/sample-rate cases"<<std::endl;
  h.plugin->releaseResources();h.sr=48000;h.plugin->prepareToPlay(48000,256);
  for(int p=0;p<40;++p){h.reset(p);h.chord();h.wait(.8);check(h.audio.getMagnitude(0,256)>1e-8,"program audio");h.off();}
  std::cout<<"PASS all 40 programs audio"<<std::endl;
  // CC64, bend, CC1 and pressure plus rapid choice/continuous automation.
  h.reset(39);h.midi.addEvent(juce::MidiMessage::controllerEvent(1,64,127),0);h.chord();h.wait(1);
  h.off();h.wait(1); // allNotesOff intentionally releases sustain; use noteOff for pedal test below.
  h.reset(39);h.midi.addEvent(juce::MidiMessage::controllerEvent(1,64,127),0);
  h.midi.addEvent(juce::MidiMessage::noteOn(1,60,juce::uint8(100)),0);h.wait(.5);
  h.midi.addEvent(juce::MidiMessage::noteOff(1,60),0);h.wait(2);check(h.audio.getMagnitude(0,256)>1e-4,"CC64 sustains");
  h.midi.addEvent(juce::MidiMessage::controllerEvent(1,64,0),0);h.wait(14);check(h.audio.getMagnitude(0,256)<1e-6,"CC64 releases");
  h.reset(37);
  for(int i=0;i<120;++i){
   h.set("MATTER EXCITER",(i%6)/5.f);h.set("MATTER MODEL",(i%8)/7.f);h.set("FIELD MODEL",(i%11)/10.f);
   h.set("PLASMA MODEL",(i%3)/2.f);h.set("PARTICLE MODEL",(i%9)/8.f);
   for(auto n:{"EXCITER FORCE","EXCITER SPEED","EXCITER POSITION","PARTICLE IMPACT","PARTICLE SELF","PLASMA DRIVE","PLASMA SELF","TENSEGRITY","ENERGY","SPACE"})h.set(n,i%2);
   h.midi.addEvent(juce::MidiMessage::noteOn(1,48+i%24,juce::uint8(100)),0);
   h.midi.addEvent(juce::MidiMessage::controllerEvent(1,1,(i%3)*63),17);
   h.midi.addEvent(juce::MidiMessage::channelPressureChange(1,(i%3)*63),33);
   h.midi.addEvent(juce::MidiMessage::pitchWheel(1,4096+(i%3)*4096),71);h.wait(.025);
   h.midi.addEvent(juce::MidiMessage::noteOff(1,48+i%24),0);
  }
  h.off();h.wait(22);check(h.audio.getMagnitude(0,256)<1e-6,"rapid changes retire");
  std::cout<<"PASS MIDI expression, sustain, 120 model switches/repeated notes/stealing, release"<<std::endl;
  for(int ex=0;ex<6;++ex){
   h.reset(37);h.set("MATTER EXCITER",ex/5.f);
   for(auto n:{"MATTER","FIELD","PARTICLES","PLASMA","TENSEGRITY","ENERGY","SPACE","PARTICLE IMPACT","PLASMA DRIVE","EXCITER FORCE","PARTICLE RATE","PARTICLE DECAY","Density"})h.set(n,1);
   for(int n=48;n<56;++n)h.midi.addEvent(juce::MidiMessage::noteOn(1,n,juce::uint8(127)),41);
   h.render("eight-feedback-"+std::to_string(ex),60,30);
   check(h.audio.getMagnitude(0,256)<1e-6,"maximum feedback release to silence");
  }
  std::cout<<"PASS six actual VST3 60-second eight-voice feedback renders peak "<<h.peak<<std::endl;
 }
 std::cout<<"PASS DynamicsPluginCheck "<<mode<<std::endl;return 0;
}catch(std::exception& x){std::cerr<<"FAIL "<<x.what()<<std::endl;return 1;}}
