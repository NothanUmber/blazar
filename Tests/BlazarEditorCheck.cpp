#include "../Source/Processor.h"
#include "../Source/UserPatches.h"
#include <chrono>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <set>
#include <thread>
void require(bool x,const char* m){if(!x)throw std::runtime_error(m);}
juce::Component* find(juce::Component& root,const juce::String& id){if(root.getComponentID()==id)return &root;for(auto* c:root.getChildren())if(auto* result=find(*c,id))return result;return nullptr;}
void pump(){juce::MessageManager::getInstance()->runDispatchLoopUntil(150);}

template <typename Predicate>
bool pumpUntil(Predicate ready, std::chrono::milliseconds timeout = std::chrono::seconds(5)) {
 // GUI timers can be delayed on busy CI runners. Wait for the observable result
 // while dispatching messages, with a monotonic deadline for broken updates.
 const auto deadline = std::chrono::steady_clock::now() + timeout;
 while (!ready()) {
  if (std::chrono::steady_clock::now() >= deadline)
   return false;
  if (!juce::MessageManager::getInstance()->runDispatchLoopUntil(10))
   return ready();
 }
 return true;
}

void inventory(juce::Component& root,std::set<juce::String>& ids){if(root.getComponentID().startsWith("param:"))ids.insert(root.getComponentID().substring(6));for(auto* c:root.getChildren())inventory(*c,ids);}
void parameterControls(juce::Component& root,std::vector<juce::Component*>& controls){if(root.getComponentID().startsWith("param:"))controls.push_back(&root);for(auto* c:root.getChildren())parameterControls(*c,controls);}
struct AudioWorker {
 Processor& processor;std::atomic<bool> running{true},safe{true};std::atomic<int> blocks{0};std::thread thread;
 explicit AudioWorker(Processor& p):processor(p),thread([this]{juce::AudioBuffer<float> audio(2,256);juce::MidiBuffer midi;
  while(running){processor.processBlock(audio,midi);for(int c=0;c<2;++c)for(int n=0;n<256;++n)if(!std::isfinite(audio.getSample(c,n))||std::abs(audio.getSample(c,n))>=.951)safe=false;++blocks;std::this_thread::sleep_for(std::chrono::milliseconds(2));}}){}
 void stop(){running=false;if(thread.joinable())thread.join();}
 ~AudioWorker(){stop();}
};
struct AutomationObserver final:juce::AudioProcessorParameter::Listener {
 juce::AudioProcessorParameter& parameter;int values=0,begins=0,ends=0;
 explicit AutomationObserver(juce::AudioProcessorParameter& p):parameter(p){parameter.addListener(this);}
 ~AutomationObserver(){parameter.removeListener(this);}
 void parameterValueChanged(int,float)override{++values;}
 void parameterGestureChanged(int,bool starting)override{if(starting)++begins;else++ends;}
};
int main(int argc,char** argv){try{
 juce::ScopedJuceInitialiser_GUI init;Processor p;p.prepareToPlay(48000,256);
 if(argc==3&&juce::String(argv[1])=="--pulse-gui"){
  blazar::UserPatches legacy(p,juce::File(argv[2]));require(legacy.load("ANOMALIES","Gravitational Stutter").wasOk(),"older user patch loads without file changes");require(std::abs(p.parameters().getRawParameterValue("pulseRate")->load()-2.f)<1e-5,"old discrete pulse rate mapped to continuous frequency");
  auto* editor=p.createEditorIfNeeded();auto* slider=dynamic_cast<juce::Slider*>(find(*editor,"param:pulseRate"));require(slider&&slider->getSliderStyle()==juce::Slider::RotaryHorizontalVerticalDrag,"rotary rate control");slider->setValue(1.137,juce::sendNotificationSync);require(std::abs(p.parameters().getRawParameterValue("pulseRate")->load()-1.137f)<1e-5,"rate knob continuous APVTS attachment");
  juce::MemoryBlock state;p.getStateInformation(state);slider->setValue(4,juce::sendNotificationSync);p.setStateInformation(state.getData(),int(state.getSize()));require(std::abs(p.parameters().getRawParameterValue("pulseRate")->load()-1.137f)<1e-5,"continuous rate state restore");p.editorBeingDeleted(editor);delete editor;std::cout<<"PASS quick rotary rate/attachment/state/older user patch compatibility"<<std::endl;return 0;
 }
 if(argc==3&&juce::String(argv[1])=="--plasma12-gui"){
  blazar::UserPatches old(p,juce::File(argv[2]));require(old.load("RADIATION","Coastal Reactor").wasOk(),"old generated Storm patch loads");require(int(p.parameters().getRawParameterValue("plasmaModel")->load())==1,"legacy Storm retained");require(old.load("RADIATION","Intermittent Sun").wasOk(),"old generated Chaos patch loads");require(int(p.parameters().getRawParameterValue("plasmaModel")->load())==2,"legacy Chaos retained");
  auto* editor=p.createEditorIfNeeded();require(editor!=nullptr,"editor");editor->setVisible(true);auto* tab=dynamic_cast<juce::Button*>(find(*editor,"tab:3"));require(tab!=nullptr,"Plasma tab");tab->onClick();pump();
  for(auto id:{"plasmaRate","plasmaCoherence","plasmaIntermittency","plasmaResonance"}){auto* c=find(*editor,"param:"+juce::String(id));require(c!=nullptr,"new Plasma control exists");for(auto* ancestor=c;ancestor;ancestor=ancestor->getParentComponent())require(ancestor->isVisible(),"new Plasma control hierarchy visible");auto* slider=dynamic_cast<juce::Slider*>(c);require(slider!=nullptr,"parameter slider");slider->setValue(.72,juce::sendNotificationSync);require(std::abs(p.parameters().getRawParameterValue(id)->load()-.72f)<1e-5,"new control attached");}
  editor->setSize(1536,1024);auto image=editor->createComponentSnapshot(editor->getLocalBounds());juce::FileOutputStream snapshot(juce::File(argv[2]).getParentDirectory().getChildFile("BLAZAR-PLASMA12.png"));require(snapshot.openedOk(),"screenshot output");snapshot.setPosition(0);snapshot.truncate();juce::PNGImageFormat png;require(png.writeImageToStream(image,snapshot),"Plasma screenshot");
  p.editorBeingDeleted(editor);delete editor;std::cout<<"PASS quick Plasma tab/four attached controls and legacy patch model mapping"<<std::endl;return 0;
 }
 if(argc==4&&juce::String(argv[1])=="--forge-banks"){
  auto recipes=juce::JSON::parse(juce::File(argv[2]));require(recipes.isArray(),"bank recipes");blazar::UserPatches store(p,juce::File(argv[3]));int count=0;double peak=0;
  juce::AudioBuffer<float> audio(2,256);juce::MidiBuffer midi;
  for(auto bank:*recipes.getArray())for(auto patch:*bank["patches"].getArray()){
   p.reset();p.setCurrentProgram(0);auto* settings=patch["parameters"].getDynamicObject();require(settings!=nullptr,"recipe values");
   for(auto property:settings->getProperties()){auto* parameter=p.parameters().getParameter(property.name.toString());require(parameter!=nullptr,"recipe parameter ID");parameter->setValueNotifyingHost(parameter->convertTo0to1(float(property.value)));}
   std::vector<float> expected;for(auto* a:p.getParameters())expected.push_back(a->getValue());
   require(store.save(bank["name"].toString(),patch["name"].toString(),false).wasOk(),"save authored recipe without overwriting");p.setCurrentProgram(1);require(store.load(bank["name"].toString(),patch["name"].toString()).wasOk(),"reload authored patch");
   for(int i=0;i<int(expected.size());++i)require(std::abs(expected[i]-p.getParameters()[i]->getValue())<1e-6,"authored patch exact parameter roundtrip");
   p.reset();midi.clear();midi.addEvent(juce::MidiMessage::noteOn(1,60,juce::uint8(100)),0);double energy=0;
   for(int block=0;block<375;++block){if(block==350)midi.addEvent(juce::MidiMessage::noteOff(1,60),0);p.processBlock(audio,midi);midi.clear();for(int ch=0;ch<2;++ch)for(int n=0;n<256;++n){double x=audio.getSample(ch,n);require(std::isfinite(x)&&std::abs(x)<.951,"authored patch finite bounded audio");peak=std::max(peak,std::abs(x));energy+=x*x;}}
   std::cout<<"PATCH "<<bank["name"].toString()<<" / "<<patch["name"].toString()<<" RMS "<<std::sqrt(energy/(375*512))<<std::endl;++count;
  }
  require(count==60,"five banks of twelve");std::cout<<"PASS 60 authored recipes, all 50 values saved/reloaded, short finite audio; peak "<<peak<<std::endl;return 0;
 }
 if(argc==3&&juce::String(argv[1])=="--patch-smoke"){
  blazar::UserPatches store(p,juce::File(argv[2]));require(store.createBank("Test Bank").wasOk(),"create bank");p.setCurrentProgram(28);
  std::vector<float> expected;for(auto* parameter:p.getParameters())expected.push_back(parameter->getValue());
  require(store.save("Test Bank","Saved Sound",false).wasOk(),"save");require(store.patches("Test Bank").contains("Saved Sound"),"list patch");
  p.setCurrentProgram(1);require(store.load("Test Bank","Saved Sound").wasOk(),"load");
  for(int i=0;i<int(expected.size());++i)require(std::abs(p.getParameters()[i]->getValue()-expected[i])<1e-6,"all recipe parameters restored");
  require(store.save("Test Bank","Another Sound",false).wasOk(),"save as");require(store.save("Test Bank","Saved Sound",true).wasOk(),"overwrite Save");
  require(!store.save("Test Bank","Saved Sound",false).wasOk(),"prevent accidental overwrite");require(!store.createBank("../unsafe").wasOk(),"reject path traversal");
  require(store.remove("Test Bank","Another Sound").wasOk(),"delete user patch");require(store.createBank("Empty Bank").wasOk(),"create empty bank");require(store.removeBank("Empty Bank").wasOk(),"delete empty bank");require(!store.removeBank("Factory").wasOk(),"Factory protected");require(!store.removeBank("../unsafe").wasOk(),"unsafe bank protected");require(store.removeBank("Test Bank").wasOk(),"delete populated bank");require(!store.banks().contains("Test Bank"),"bank removed");
  auto* editor=p.createEditorIfNeeded();require(editor!=nullptr,"custom GUI opens");require(find(*editor,"deleteBank")!=nullptr,"Delete Bank control exists");require(!find(*editor,"deleteBank")->isEnabled(),"Factory Delete Bank disabled");
  auto* patch=find(*editor,"program");auto* bank=find(*editor,"patchBank");require(patch&&bank&&patch->getHeight()==32&&bank->getHeight()==32,"roomy header selectors");
  editor->setSize(1200,800);editor->setSize(960,640);p.editorBeingDeleted(editor);delete editor;
  std::cout<<"PASS quick custom GUI/header/Factory protection/empty and populated bank deletion"<<std::endl;
  std::cout<<"PASS user banks/save/save-as/overwrite/load/delete: all "<<expected.size()<<" parameter values restored"<<std::endl;return 0;
 }
 require(juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()!=nullptr,"EditorCheck requires a graphical desktop session (use CTest -LE gui when headless)");
 juce::AudioBuffer<float> audio(2,256);juce::MidiBuffer midi;auto block=[&]{p.processBlock(audio,midi);for(int c=0;c<2;++c)for(int n=0;n<256;++n)require(std::isfinite(audio.getSample(c,n))&&std::abs(audio.getSample(c,n))<.951,"finite bounded audio");};
 auto deleteEditor=[&p](juce::AudioProcessorEditor* e){if(e){p.editorBeingDeleted(e);delete e;}};
 std::unique_ptr<juce::AudioProcessorEditor,decltype(deleteEditor)> editor(p.createEditorIfNeeded(),deleteEditor);require(editor&&editor->getWidth()>=960&&editor->getWidth()<=1200,"custom editor");editor->setName("BLAZAR editor validation");editor->addToDesktop(juce::ComponentPeer::windowIsTemporary);editor->setTopLeftPosition(0,0);editor->setVisible(true);editor->toFront(true);
 std::set<juce::String> ids;inventory(*editor,ids);require(ids.size()==p.getParameters().size(),"all parameters exposed");
 int changed=0;auto verify=[&]{std::vector<juce::Component*> controls;parameterControls(*editor,controls);for(auto* c:controls){auto id=c->getComponentID().substring(6);auto* parameter=p.parameters().getParameter(id);require(parameter,"UI id exists");float expected=parameter->getValue();
   if(auto* slider=dynamic_cast<juce::Slider*>(c))require(std::abs(parameter->convertTo0to1(float(slider->getValue()))-expected)<1e-4,"parameter->slider");
   else if(auto* box=dynamic_cast<juce::ComboBox*>(c))require(box->getSelectedId()-1==int(std::round(parameter->convertFrom0to1(expected))),"parameter->choice");
   else require(false,"parameter control type");}};
 for(auto& id:ids){auto* parameter=p.parameters().getParameter(id);AutomationObserver observer(*parameter);auto* c=find(*editor,"param:"+id);std::vector<float> before;for(auto* a:p.getParameters())before.push_back(a->getValue());
  if(auto* slider=dynamic_cast<juce::Slider*>(c))slider->setValue(parameter->convertFrom0to1(.731f),juce::sendNotificationSync);
  else dynamic_cast<juce::ComboBox*>(c)->setSelectedId(parameter->getNumSteps(),juce::sendNotificationSync);
  pump();float expected=dynamic_cast<juce::Slider*>(c)?parameter->convertTo0to1(float(dynamic_cast<juce::Slider*>(c)->getValue())):1.f;
  require(std::abs(parameter->getValue()-expected)<1e-4,"GUI->parameter");require(observer.values>0,"GUI->host value notification");
  for(size_t i=0;i<before.size();++i)if(p.getParameters()[int(i)]!=parameter)require(before[i]==p.getParameters()[int(i)]->getValue(),"GUI changed unrelated parameter");
  parameter->setValueNotifyingHost(.233f);pump();verify();++changed;
 }
 std::cout<<"PASS "<<changed<<" attachment IDs: GUI->parameter isolation, host->GUI"<<std::endl;
 auto* programs=dynamic_cast<juce::ComboBox*>(find(*editor,"program"));require(programs&&programs->getNumItems()==40,"40 program selector");
 for(int i=0;i<40;++i){
  programs->setSelectedId(i+1,juce::sendNotificationSync);
  pump();require(p.getCurrentProgram()==i,"GUI program");verify();
  const int hostProgram=(i+7)%40;
  p.setCurrentProgram(hostProgram);
  const bool refreshed=pumpUntil([&]{return programs->getSelectedId()==hostProgram+1;});
  if(!refreshed)
   std::cout<<"PROGRAM_REFRESH i "<<i<<" processor "<<p.getCurrentProgram()<<" combo "<<programs->getSelectedId()<<" expected "<<hostProgram+1<<std::endl;
  require(refreshed,"host program->GUI timed out after 5 seconds");
  verify();
 }
 std::cout<<"PASS 40 GUI/host programs refresh every attachment"<<std::endl;
 auto* forceSlider=dynamic_cast<juce::Slider*>(find(*editor,"param:force"));
 {AutomationObserver observer(*p.parameters().getParameter("force"));auto point=forceSlider->getLocalBounds().getCentre().toFloat();auto now=juce::Time::getCurrentTime();
  auto mouse=[&](juce::ModifierKeys mods){return juce::MouseEvent(juce::Desktop::getInstance().getMainMouseSource(),point,mods,1,0,0,0,0,forceSlider,forceSlider,now,point,now,1,false);};
  forceSlider->mouseDown(mouse(juce::ModifierKeys::leftButtonModifier));forceSlider->setValue(.632,juce::sendNotificationSync);forceSlider->mouseUp(mouse(juce::ModifierKeys{}));
  require(observer.values>0&&observer.begins>0&&observer.ends>0,"native knob drag host gesture notifications");}
 std::cout<<"PASS native knob drag begin/value/end host automation notifications"<<std::endl;
 p.parameters().getParameter("volume")->setValueNotifyingHost(.6f);p.parameters().getParameter("pulseRate")->setValueNotifyingHost(1.f);juce::MemoryBlock state;p.getStateInformation(state);
 p.parameters().getParameter("volume")->setValueNotifyingHost(1);p.setCurrentProgram(0);p.setStateInformation(state.getData(),int(state.getSize()));pump();verify();require(std::abs(p.parameters().getParameter("volume")->getValue()-.6f)<1e-6,"volume restored");require(p.parameters().getParameter("pulseRate")->getValue()==1,"rate restored");
 p.setCurrentProgram(39);p.parameters().getParameter("volume")->setValueNotifyingHost(1);
 for(int i=0;i<80;++i){midi.addEvent(juce::MidiMessage::noteOn(1,48+i%12,juce::uint8(100)),0);block();
  std::vector<float> before;for(auto* a:p.getParameters())before.push_back(a->getValue());auto* tab=dynamic_cast<juce::TextButton*>(find(*editor,"tab:"+juce::String(i%5)));require(tab,"tab");tab->onClick();
  for(size_t j=0;j<before.size();++j)require(before[j]==p.getParameters()[int(j)]->getValue(),"tab changed synthesis");}
 auto* keys=dynamic_cast<juce::MidiKeyboardComponent*>(find(*editor,"keyboard"));require(keys,"keyboard");
 p.reset();editor->setSize(960,640);editor->setTopLeftPosition(0,0);editor->setAlwaysOnTop(true);editor->toFront(true);pump();auto rectangle=keys->getRectangleForKey(60);auto point=rectangle.getCentre();point.y=keys->getHeight()*.85f;auto now=juce::Time::getCurrentTime();
 auto event=[&](juce::ModifierKeys modifiers){return juce::MouseEvent(juce::Desktop::getInstance().getMainMouseSource(),point,modifiers,1,0,0,0,0,keys,keys,now,point,now,1,false);};
 auto hit=editor->getComponentAt(editor->getLocalPoint(keys,point));std::cout<<"KEY_HIT point "<<point.toString()<<" editor "<<editor->getLocalPoint(keys,point).toString()<<" hit "<<(hit?hit->getComponentID():"null")<<" contains "<<keys->contains(point)<<" desktop "<<juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()->userArea.toString()<<std::endl;
 keys->mouseDown(event(juce::ModifierKeys::leftButtonModifier));for(int n=0;n<128;++n)if(p.keyboardState().isNoteOn(1,n))std::cout<<"KEY_ON "<<n<<std::endl;require(p.keyboardState().isNoteOn(1,60),"mouse keyboard note on");
 for(int n=0;n<400;++n)block();require(audio.getMagnitude(0,256)>1e-7,"mouse keyboard audio");keys->mouseUp(event(juce::ModifierKeys{}));require(!p.keyboardState().isNoteOn(1,60),"mouse keyboard off");block();editor->setAlwaysOnTop(false);
 midi.addEvent(juce::MidiMessage::noteOn(1,64,juce::uint8(100)),0);midi.addEvent(juce::MidiMessage::pitchWheel(1,12000),0);midi.addEvent(juce::MidiMessage::controllerEvent(1,1,100),0);block();pump();
 require(p.keyboardState().isNoteOn(1,64),"external MIDI visible");require(p.pitchValue()==12000&&p.modValue()==100,"external performance values");
 auto* pitch=dynamic_cast<juce::Slider*>(find(*editor,"pitchWheel"));auto* mod=dynamic_cast<juce::Slider*>(find(*editor,"modWheel"));
 pitch->setValue(-.5,juce::sendNotificationSync);mod->setValue(.5,juce::sendNotificationSync);block();require(p.pitchValue()==4096&&p.modValue()==64,"GUI pitch/mod audio path");pitch->onDragEnd();block();require(p.pitchValue()==8192,"pitch spring return");
 for(int n=0;n<400;++n)block();require(p.rms(0)>0&&p.rms(1)>0&&p.takePeak(0)>0,"stereo output metering");
 p.setCurrentProgram(0);editor->setSize(1536,1024);dynamic_cast<juce::TextButton*>(find(*editor,"tab:0"))->onClick();pump();verify();auto png=juce::File(argc>1?argv[1]:"BLAZAR-GUI.png");
 auto snapshot=[&](const juce::File& file){auto image=editor->createComponentSnapshot(editor->getLocalBounds(),true,1.f);juce::FileOutputStream stream(file);require(stream.openedOk(),"screenshot output");stream.setPosition(0);stream.truncate();juce::PNGImageFormat format;require(format.writeImageToStream(image,stream),"screenshot PNG");stream.flush();};snapshot(png);
 for(int t=1;t<5;++t){dynamic_cast<juce::TextButton*>(find(*editor,"tab:"+juce::String(t)))->onClick();snapshot(png.getSiblingFile("BLAZAR-tab-"+juce::String(t)+".png"));}
 AudioWorker worker(p);
 for(int i=0;i<12;++i){editor.reset();pump();editor.reset(p.createEditorIfNeeded());editor->addToDesktop(juce::ComponentPeer::windowIsTemporary);editor->setTopLeftPosition(0,0);editor->setVisible(true);editor->setSize(i%2?960:1536,i%2?640:1024);pump();verify();}
 worker.stop();require(worker.safe&&worker.blocks>100,"concurrent editor/audio lifecycle");std::cout<<"PASS concurrent audio worker blocks "<<worker.blocks<<std::endl;
 std::cout<<"PASS editor closed/reopened while audio continues, sizes, 80 tabs, mouse keyboard, MIDI, pitch/mod, meter, volume/rate state"<<std::endl;
 std::cout<<"PASS EditorCheck screenshot "<<png.getFullPathName()<<std::endl;return 0;
 }catch(std::exception& e){std::cerr<<"FAIL "<<e.what()<<std::endl;return 1;}}
