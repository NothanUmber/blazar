#pragma once
#include "Processor.h"
#include "UserPatches.h"
#include <BinaryData.h>
#include <map>
namespace blazar {
inline juce::Colour colour(int world){static const juce::uint32 c[]={0xffffa52d,0xff20c9ff,0xffba8af5,0xffff5a9b,0xff57e69b};return juce::Colour(c[std::clamp(world,0,4)]);}
inline juce::Colour ink(){return juce::Colour(0xffb2e9fa);}
inline juce::Colour panel(){return juce::Colour(0xff04131d);}
inline juce::Font font(float size,bool bold=false){return juce::Font(juce::FontOptions(size,bold?juce::Font::bold:juce::Font::plain));}
inline void text(juce::Graphics& g,const juce::String& s,juce::Rectangle<int> r,float size=18,juce::Colour c=ink(),juce::Justification j=juce::Justification::centredLeft){g.setColour(c);g.setFont(font(size));g.drawFittedText(s,r,j,2);}
class LookAndFeel final:public juce::LookAndFeel_V4 {
public:
 LookAndFeel(){setColour(juce::Slider::textBoxTextColourId,ink());setColour(juce::Slider::textBoxBackgroundColourId,juce::Colours::transparentBlack);setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);
  setColour(juce::ComboBox::backgroundColourId,panel());setColour(juce::ComboBox::textColourId,ink());setColour(juce::ComboBox::outlineColourId,juce::Colour(0xff335764));setColour(juce::ComboBox::arrowColourId,ink());
  setColour(juce::PopupMenu::backgroundColourId,panel());setColour(juce::PopupMenu::textColourId,ink());setColour(juce::PopupMenu::highlightedBackgroundColourId,juce::Colour(0xff123c4b));
  setColour(juce::TextButton::textColourOffId,ink());setColour(juce::TextButton::textColourOnId,juce::Colours::white);
 }
 juce::Font getComboBoxFont(juce::ComboBox& box)override{return font(std::min(19.f,box.getHeight()*.48f));}
 juce::Font getTextButtonFont(juce::TextButton&,int height)override{return font(std::min(20.f,height*.44f));}
 juce::Label* createSliderTextBox(juce::Slider& s)override{auto* label=juce::LookAndFeel_V4::createSliderTextBox(s);label->setFont(font(18));label->setColour(juce::Label::outlineColourId,juce::Colours::transparentBlack);label->setColour(juce::Label::backgroundColourId,juce::Colours::transparentBlack);return label;}
 void drawButtonBackground(juce::Graphics& g,juce::Button& b,const juce::Colour&,bool hover,bool down)override{
  auto r=b.getLocalBounds().toFloat().reduced(1);auto c=b.findColour(juce::TextButton::buttonColourId);bool on=b.getToggleState();
  g.setGradientFill(juce::ColourGradient(on?c.withAlpha(.22f):panel().brighter(.12f),0,r.getY(),panel(),0,r.getBottom(),false));g.fillRoundedRectangle(r,4);
  g.setColour(c.withAlpha(on?.95f:(hover?.8f:.45f)));g.drawRoundedRectangle(r,4,on?2.f:1.f);
  if(on){g.setColour(c.withAlpha(.25f));g.drawRoundedRectangle(r.reduced(4),3,2);}
  if(down){g.setColour(c.withAlpha(.15f));g.fillRoundedRectangle(r,4);}
 }
 void drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,float pos,float start,float end,juce::Slider& s)override{
  auto r=juce::Rectangle<float>(float(x),float(y),float(w),float(h)).reduced(8);float radius=std::min(r.getWidth(),r.getHeight())*.5f;auto centre=r.getCentre();
  auto c=s.findColour(juce::Slider::rotarySliderFillColourId);float angle=start+pos*(end-start);
  auto arc=[&](float rad,float to){juce::Path p;p.addCentredArc(centre.x,centre.y,rad,rad,0,start,to,true);return p;};
  g.setColour(juce::Colour(0xff00060b));g.fillEllipse(centre.x-radius-1,centre.y-radius+3,2*radius+2,2*radius+2);
  g.setColour(juce::Colour(0xff34434e));g.strokePath(arc(radius-3,end),juce::PathStrokeType(7,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
  g.setColour(c.withAlpha(.14f));g.strokePath(arc(radius-3,angle),juce::PathStrokeType(14,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
  g.setColour(c);g.strokePath(arc(radius-3,angle),juce::PathStrokeType(5,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
  float inner=radius-10;g.setGradientFill(juce::ColourGradient(juce::Colour(0xff22343f),centre.x,centre.y-inner,juce::Colour(0xff02090e),centre.x,centre.y+inner,false));g.fillEllipse(centre.x-inner,centre.y-inner,inner*2,inner*2);
  g.setColour(juce::Colours::black.withAlpha(.8f));g.drawEllipse(centre.x-inner,centre.y-inner,inner*2,inner*2,1);
  juce::Path pointer;pointer.startNewSubPath(0,-inner+3);pointer.lineTo(0,-inner*.45f);pointer.applyTransform(juce::AffineTransform::rotation(angle).translated(centre.x,centre.y));g.setColour(c.brighter(.5f));g.strokePath(pointer,juce::PathStrokeType(1.5f));
 }
 void drawLinearSlider(juce::Graphics& g,int x,int y,int w,int h,float sliderPos,float,float,juce::Slider::SliderStyle style,juce::Slider& s)override{
  if(style!=juce::Slider::LinearVertical){juce::LookAndFeel_V4::drawLinearSlider(g,x,y,w,h,sliderPos,0,0,style,s);return;}
  auto c=s.findColour(juce::Slider::trackColourId);g.setColour(juce::Colour(0xff010609));g.fillRoundedRectangle(float(x+w/2-13),float(y),26.f,float(h),4);
  g.setColour(juce::Colour(0xff284350));g.drawRoundedRectangle(float(x+w/2-13),float(y),26.f,float(h),4,1);
  g.setColour(c.withAlpha(.15f));g.fillRoundedRectangle(float(x+w/2-8),sliderPos-22,16,44,3);
  g.setColour(c);g.fillRoundedRectangle(float(x+w/2-3),sliderPos-20,6,40,2);
 }
};
class Knob final:public juce::Component {
public:
 juce::Slider slider;
private:
 juce::Label label;
 bool headerVolume=false;
 // Members are destroyed in reverse order: detach while the slider still exists.
 std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
public:
 Knob(juce::AudioProcessorValueTreeState& state,const juce::String& id,const juce::String& caption,juce::Colour c){
  headerVolume=id=="volume";setComponentID("control:"+id);slider.setComponentID("param:"+id);label.setText(caption,juce::dontSendNotification);label.setJustificationType(juce::Justification::centred);label.setFont(font(17));label.setColour(juce::Label::textColourId,ink());
  slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);slider.setRotaryParameters(juce::MathConstants<float>::pi*1.2f,juce::MathConstants<float>::pi*2.8f,true);
  slider.setTextBoxStyle(juce::Slider::TextBoxBelow,false,100,25);slider.setColour(juce::Slider::rotarySliderFillColourId,c);slider.setMouseDragSensitivity(180);slider.setScrollWheelEnabled(false);
  slider.setTooltip(caption+" - drag to edit; double-click restores the parameter default.");
  addAndMakeVisible(label);addAndMakeVisible(slider);attachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(state,id,slider);
  auto* p=state.getParameter(id);slider.setDoubleClickReturnValue(true,p->convertFrom0to1(p->getDefaultValue()));
  if(id=="volume"){slider.textFromValueFunction=[](double x){return x<=-60?juce::String("MUTE"):juce::String(x,1)+" dB";};slider.valueFromTextFunction=[](const juce::String& x){return x.containsIgnoreCase("mute")?-60.:x.getDoubleValue();};}
  else if(id=="pulseRate"){slider.textFromValueFunction=[](double x){return juce::String(x,2)+" /beat";};slider.valueFromTextFunction=[](const juce::String& x){return x.getDoubleValue();};}
  else {bool seconds=id=="attack"||id=="release"||id=="fieldAttack"||id=="fieldRelease";slider.textFromValueFunction=[seconds](double x){return juce::String(x,2)+(seconds?" s":"");};slider.valueFromTextFunction=[](const juce::String& x){return x.getDoubleValue();};}
 }
 void resized()override{auto r=getLocalBounds();if(headerVolume){label.setBounds(r.removeFromLeft(75).withHeight(65));slider.setBounds(r);}else{label.setBounds(r.removeFromTop(28));slider.setBounds(r);}}
};
class Selector final:public juce::Component {
public:
 juce::ComboBox box;
private:
 juce::Label label;
 // The attachment removes a listener from box during destruction.
 std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;
public:
 Selector(juce::AudioProcessorValueTreeState& state,const juce::String& id,const juce::String& caption,juce::Colour c){
  setComponentID("control:"+id);box.setComponentID("param:"+id);label.setText(caption,juce::dontSendNotification);label.setFont(font(18));label.setColour(juce::Label::textColourId,ink());
  auto* p=dynamic_cast<juce::AudioParameterChoice*>(state.getParameter(id));jassert(p);box.addItemList(p->choices,1);box.setColour(juce::ComboBox::outlineColourId,c.withAlpha(.5f));box.setTooltip(caption);
  addAndMakeVisible(label);addAndMakeVisible(box);attachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(state,id,box);
 }
 void resized()override{auto r=getLocalBounds();label.setBounds(r.removeFromTop(29));box.setBounds(r.reduced(0,2));}
};
class Artwork final:public juce::Component {
 juce::Image original,cache;
public:
 Artwork(){setInterceptsMouseClicks(false,false);replace(juce::ImageCache::getFromMemory(BinaryData::BlazarAccretion_png,BinaryData::BlazarAccretion_pngSize));}
 void replace(juce::Image image){original=image;resized();repaint();}
 void resized()override{if(original.isValid()&&getWidth()>0&&getHeight()>0)cache=original.rescaled(getWidth(),getHeight(),juce::Graphics::highResamplingQuality);}
 void paint(juce::Graphics& g)override{
  if(cache.isValid())g.drawImageAt(cache,0,0);
  // Fade artwork edges without hiding its white-hot centre.
  auto r=getLocalBounds().toFloat();g.setGradientFill(juce::ColourGradient(panel(),0,0,panel().withAlpha(0.f),70,0,false));g.fillRect(r.withWidth(70));
  g.setGradientFill(juce::ColourGradient(panel(),r.getRight(),0,panel().withAlpha(0.f),r.getRight()-70,0,false));g.fillRect(r.withLeft(r.getRight()-70));
 }
};
class Meter final:public juce::Component,private juce::Timer {
 Processor& processor;std::array<float,2> peak{},rms{};
public:
 explicit Meter(Processor& p):processor(p){setComponentID("outputMeter");startTimerHz(30);}
 void timerCallback()override{for(int i=0;i<2;++i){peak[i]=std::max(processor.takePeak(i),peak[i]*.9f);rms[i]=processor.rms(i);}repaint();}
 void paint(juce::Graphics& g)override{
  auto r=getLocalBounds();text(g,"OUTPUT",r.removeFromTop(20),14);
  for(int ch=0;ch<2;++ch){int y=26+ch*22;text(g,ch?"R":"L",{0,y,15,16},13);float db=juce::Decibels::gainToDecibels(peak[ch],-60.f);float rd=juce::Decibels::gainToDecibels(rms[ch],-60.f);
   int count=18;float width=(getWidth()-20.f)/count;
   for(int n=0;n<count;++n){float threshold=-60+60.f*n/count;g.setColour(db>=threshold?(n>=17?juce::Colour(0xffffa52d):juce::Colour(0xff28dfcf)):juce::Colour(0xff142c36));g.fillRect(20.f+n*width,float(y),width-2,9.f);}
   g.setColour(ink().withAlpha(.8f));g.fillRect(20.f+(getWidth()-22.f)*juce::jlimit(0.f,1.f,(rd+60)/60),float(y+11),2.f,4.f);
  }
 }
};
class Canvas final:public juce::Component,private juce::Timer {
 Processor& processor;LookAndFeel look;Artwork artwork;Meter meter;
 UserPatches userPatches;juce::ComboBox bank;juce::TextButton savePatch{"SAVE"},saveAsPatch{"SAVE AS"},deletePatch{"DELETE"},newBank{"NEW BANK"},deleteBank{"DELETE BANK"};
 juce::ComboBox program;juce::TextButton previous{"<"},next{">"},initial{"INIT"};
 std::array<juce::TextButton,5> tabs;
 std::vector<std::unique_ptr<Knob>> globals,details;
 std::vector<std::unique_ptr<Selector>> selectors;
 std::vector<int> detailWorld,selectorWorld;
 Knob rate;
 juce::MidiKeyboardComponent keys;
 juce::Slider pitch,mod;juce::TextButton octaveDown{"<"},octaveUp{">"};juce::Label octaveLabel,voicesLabel;
 juce::TooltipWindow tips{this,650};int selected=0,octave=0;
 uint32_t lastProgramRevision=0;
 void refreshRecalledProgram(){
  const auto revision=processor.getProgramRevision();
  if(revision==lastProgramRevision)return;
  lastProgramRevision=revision;
  if(bank.getSelectedId()!=1){refreshBanks();refreshPrograms();}
  else program.setSelectedId(processor.getCurrentProgram()+1,juce::dontSendNotification);
 }
 void timerCallback()override{
  refreshRecalledProgram();
  if(!pitch.isMouseButtonDown())pitch.setValue((processor.pitchValue()-8192)/8192.,juce::dontSendNotification);
  if(!mod.isMouseButtonDown())mod.setValue(processor.modValue()/127.,juce::dontSendNotification);
  auto v=juce::String(processor.voices())+" / 8 VOICES";if(voicesLabel.getText()!=v)voicesLabel.setText(v,juce::dontSendNotification);
 }
 void error(const juce::Result& r){if(r.failed())juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,"User patches",r.getErrorMessage());}
 juce::String selectedBank()const{return bank.getSelectedId()==1?juce::String():bank.getText();}
 void refreshBanks(const juce::String& choose={}){bank.clear(juce::dontSendNotification);bank.addItem("Factory",1);int id=2;for(auto name:userPatches.banks()){bank.addItem(name,id);if(name==choose)bank.setSelectedId(id,juce::dontSendNotification);++id;}if(bank.getSelectedId()==0)bank.setSelectedId(1,juce::dontSendNotification);}
 void refreshPrograms(const juce::String& choose={}){
  program.clear(juce::dontSendNotification);bool factory=bank.getSelectedId()==1;int id=1;
  if(factory){for(int i=0;i<processor.getNumPrograms();++i)program.addItem(processor.getProgramName(i),i+1);program.setSelectedId(processor.getCurrentProgram()+1,juce::dontSendNotification);}
  else for(auto name:userPatches.patches(selectedBank())){program.addItem(name,id);if(name==choose)program.setSelectedId(id,juce::dontSendNotification);++id;}
  deleteBank.setEnabled(!factory);
  program.setTextWhenNothingSelected(factory?"Factory":"Select a user patch");deletePatch.setEnabled(!factory&&program.getSelectedId()>0);
 }
 void chooseProgram(){
  if(program.getSelectedId()<=0)return;
  if(bank.getSelectedId()==1)processor.setCurrentProgram(program.getSelectedId()-1);
  else error(userPatches.load(selectedBank(),program.getText()));
  lastProgramRevision=processor.getProgramRevision();
  deletePatch.setEnabled(bank.getSelectedId()!=1&&program.getSelectedId()>0);
 }
 void nameDialog(bool bankOnly){
  auto* dialog=new juce::AlertWindow(bankOnly?"New user bank":"Save patch as","Names are yours. Factory patches are never overwritten.",juce::MessageBoxIconType::NoIcon);
  dialog->addTextEditor("bank",selectedBank(),"Bank name:");if(!bankOnly)dialog->addTextEditor("patch",bank.getSelectedId()==1?"":program.getText(),"Patch name:");
  dialog->addButton("Cancel",0);dialog->addButton(bankOnly?"Create":"Save",1);
  juce::Component::SafePointer<Canvas> safe(this);juce::Component::SafePointer<juce::AlertWindow> window(dialog);
  dialog->enterModalState(true,juce::ModalCallbackFunction::create([safe,window,bankOnly](int accepted){if(!accepted||!safe||!window)return;
   auto b=window->getTextEditorContents("bank").trim();auto n=bankOnly?juce::String():window->getTextEditorContents("patch").trim();
   auto result=bankOnly?safe->userPatches.createBank(b):safe->userPatches.save(b,n,false);safe->error(result);
   if(result.wasOk()){safe->refreshBanks(b);safe->refreshPrograms(n);}
  }),true);
 }
 void confirmDeleteBank(){
  if(bank.getSelectedId()==1)return;auto b=selectedBank();auto count=userPatches.patches(b).size();juce::Component::SafePointer<Canvas> safe(this);
  auto message="Delete user bank "+b+"?\n"+juce::String(count>0?"This will also delete all "+juce::String(count)+" patches in the bank and any other contents.":"This bank is empty.")+"\nThis cannot be undone.";
  juce::AlertWindow::showOkCancelBox(juce::MessageBoxIconType::WarningIcon,"Delete user bank?",message,"Delete Bank","Cancel",nullptr,juce::ModalCallbackFunction::create([safe,b](int yes){if(yes&&safe){auto result=safe->userPatches.removeBank(b);safe->error(result);if(result.wasOk()){safe->refreshBanks();safe->refreshPrograms();}}}));
 }
 void confirmDelete(){if(bank.getSelectedId()==1||program.getSelectedId()==0)return;auto b=selectedBank(),n=program.getText();juce::Component::SafePointer<Canvas> safe(this);
  juce::AlertWindow::showOkCancelBox(juce::MessageBoxIconType::WarningIcon,"Delete user patch?","Delete "+n+" from "+b+"? This cannot be undone.","Delete","Cancel",nullptr,juce::ModalCallbackFunction::create([safe,b,n](int yes){if(yes&&safe){auto result=safe->userPatches.remove(b,n);safe->error(result);if(result.wasOk())safe->refreshPrograms();}}));
 }
 void addKnob(int world,const char* id,const char* caption,juce::Rectangle<int> r){
  auto k=std::make_unique<Knob>(processor.parameters(),id,caption,colour(world));k->setBounds(r);addChildComponent(*k);details.push_back(std::move(k));detailWorld.push_back(world);
 }
 void addSelector(int world,const char* id,const char* caption,juce::Rectangle<int> r){
  auto c=std::make_unique<Selector>(processor.parameters(),id,caption,colour(world));c->setBounds(r);addChildComponent(*c);selectors.push_back(std::move(c));selectorWorld.push_back(world);
 }
public:
 explicit Canvas(Processor& p):processor(p),userPatches(p),meter(p),rate(p.parameters(),"pulseRate","RATE",colour(1)),keys(p.keyboardState(),juce::MidiKeyboardComponent::horizontalKeyboard){
  setLookAndFeel(&look);setSize(1536,1024);setComponentID("blazarCanvas");
  addAndMakeVisible(artwork);artwork.setBounds(392,338,726,504);
  program.setComponentID("program");for(int i=0;i<p.getNumPrograms();++i)program.addItem(p.getProgramName(i),i+1);program.setSelectedId(p.getCurrentProgram()+1,juce::dontSendNotification);
  program.onChange=[this]{chooseProgram();};program.setBounds(738,23,292,32);addAndMakeVisible(program);
  previous.setComponentID("previousProgram");next.setComponentID("nextProgram");initial.setComponentID("init");
  auto step=[this](int d){int count=program.getNumItems();if(count>0)program.setSelectedId((std::max(0,program.getSelectedId()-1)+d+count)%count+1,juce::sendNotificationSync);};
  previous.onClick=[step]{step(-1);};next.onClick=[step]{step(1);};initial.onClick=[this]{processor.initialise();refreshRecalledProgram();};
  previous.setBounds(696,23,34,32);next.setBounds(1038,23,34,32);initial.setBounds(1050,63,50,32);
  for(auto* b:{&previous,&next,&initial}){b->setColour(juce::TextButton::buttonColourId,colour(1));addAndMakeVisible(b);}
  bank.setComponentID("patchBank");bank.setBounds(491,23,197,32);bank.setTooltip("Factory or a named user bank");addAndMakeVisible(bank);refreshBanks();bank.onChange=[this]{refreshPrograms();};
  savePatch.setBounds(491,63,85,32);saveAsPatch.setBounds(584,63,100,32);deletePatch.setBounds(692,63,85,32);newBank.setBounds(785,63,120,32);deleteBank.setBounds(913,63,129,32);
  for(auto* b:{&savePatch,&saveAsPatch,&deletePatch,&newBank,&deleteBank}){b->setColour(juce::TextButton::buttonColourId,colour(1));addAndMakeVisible(b);}
  savePatch.setComponentID("savePatch");
  savePatch.onClick=[this]{
   // A host recall may have arrived since the last timer callback. Never save
   // its parameters over the user patch that was selected before that recall.
   refreshRecalledProgram();
   if(bank.getSelectedId()==1||program.getSelectedId()==0){nameDialog(false);return;}
   error(userPatches.save(selectedBank(),program.getText(),true));
  };
  saveAsPatch.onClick=[this]{nameDialog(false);};newBank.onClick=[this]{nameDialog(true);};deletePatch.onClick=[this]{confirmDelete();};deleteBank.setComponentID("deleteBank");deleteBank.onClick=[this]{confirmDeleteBank();};deleteBank.setEnabled(false);
  deletePatch.setEnabled(false);lastProgramRevision=processor.getProgramRevision();
  const char* ids[]={"matter","field","particles","plasma","coupling","energy","motion","evolve","space","pulse"};
  const char* labels[]={"MATTER","FIELD","PARTICLES","PLASMA","TENSEGRITY","ENERGY","MOTION","EVOLVE","SPACE","PULSE"};
  for(int i=0;i<10;++i){auto k=std::make_unique<Knob>(p.parameters(),ids[i],labels[i],i<5?colour(i):i==5?colour(0):i==7?colour(2):ink());
   k->setBounds(30+i*128,113,118,132);addAndMakeVisible(*k);globals.push_back(std::move(k));}
  auto volume=std::make_unique<Knob>(p.parameters(),"volume","VOLUME",colour(1));volume->setBounds(1111,7,185,90);addAndMakeVisible(*volume);globals.push_back(std::move(volume));
  rate.setBounds(1322,113,176,132);rate.slider.setTooltip("Continuous tempo-relative rate: 0.25 to 4 cycles per beat.");addAndMakeVisible(rate);meter.setBounds(1316,22,170,74);addAndMakeVisible(meter);
  const char* names[]={"MATTER","FIELD","PARTICLES","PLASMA","TENSEGRITY"};
  for(int i=0;i<5;++i){tabs[i].setButtonText(names[i]);tabs[i].setComponentID("tab:"+juce::String(i));tabs[i].setColour(juce::TextButton::buttonColourId,colour(i));tabs[i].setBounds(15+i*302,274,296,50);tabs[i].onClick=[this,i]{selectTab(i);};addAndMakeVisible(tabs[i]);}
  addSelector(0,"matterModel","MATTER MODEL",{42,351,322,78});addSelector(0,"exciter","EXCITER",{42,462,322,78});
  addKnob(0,"force","FORCE",{40,548,140,137});addKnob(0,"hardness","HARDNESS",{212,548,140,137});addKnob(0,"position","POSITION",{40,697,140,137});
  addKnob(0,"speed","SPEED",{212,697,140,137});addKnob(0,"structure","STRUCTURE",{1144,400,155,154});addKnob(0,"tone","TONE",{1335,400,155,154});
  addKnob(0,"attack","ATTACK",{1144,666,155,154});addKnob(0,"release","RELEASE",{1335,666,155,154});
  addSelector(1,"fieldModel","FIELD MODEL",{42,351,322,78});
  addKnob(1,"fieldShape","SHAPE",{40,512,140,145});addKnob(1,"fieldBrightness","BRIGHTNESS",{212,512,140,145});
  addKnob(1,"fieldModulation","MODULATION",{40,684,140,145});addKnob(1,"fieldSpread","SPREAD",{212,684,140,145});
  addKnob(1,"fieldStructure","STRUCTURE",{1144,400,155,154});addKnob(1,"fieldTone","TONE",{1335,400,155,154});
  addKnob(1,"fieldAttack","ATTACK",{1144,666,155,154});addKnob(1,"fieldRelease","RELEASE",{1335,666,155,154});
  addSelector(2,"particleModel","PARTICLE MODEL",{42,351,322,78});
  addKnob(2,"particleRate","RATE",{40,512,140,145});addKnob(2,"particleDecay","DECAY",{212,512,140,145});
  addKnob(2,"particlePitch","PITCH / SIZE",{40,684,140,145});addKnob(2,"particleSpread","SPREAD",{212,684,140,145});
  addKnob(2,"particleSelf","SELF",{1144,428,155,154});addKnob(2,"particleImpact","IMPACT",{1335,428,155,154});addKnob(2,"density","DENSITY",{1240,691,155,137});
  addSelector(3,"plasmaModel","PLASMA MODEL",{42,351,322,78});
  addKnob(3,"plasmaFlow","FLOW / SPEED",{40,512,140,145});addKnob(3,"plasmaPressure","PRESSURE",{212,512,140,145});
  addKnob(3,"plasmaTurbulence","TURBULENCE",{40,684,140,145});addKnob(3,"plasmaScale","SCALE",{212,684,140,145});
  addKnob(3,"plasmaSelf","SELF",{1144,392,155,128});addKnob(3,"plasmaDrive","DRIVE",{1335,392,155,128});
  addKnob(3,"plasmaRate","RATE",{1144,578,155,116});addKnob(3,"plasmaCoherence","COHERENCE",{1335,578,155,116});
  addKnob(3,"plasmaIntermittency","INTERMITTENCY",{1144,715,155,116});addKnob(3,"plasmaResonance","RESONANCE",{1335,715,155,116});
  addKnob(4,"coupling","TENSEGRITY",{1231,382,180,190});
  keys.setComponentID("keyboard");keys.setAvailableRange(24,108);keys.setLowestVisibleKey(48);keys.setKeyWidth(juce::MathConstants<float>::sqrt2*28);keys.setBounds(169,860,1217,138);
  keys.setColour(juce::MidiKeyboardComponent::whiteNoteColourId,juce::Colour(0xffe4edf0));keys.setColour(juce::MidiKeyboardComponent::blackNoteColourId,juce::Colour(0xff08141e));
  keys.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId,colour(1).withAlpha(.6f));keys.setColour(juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId,colour(1).withAlpha(.2f));addAndMakeVisible(keys);
  auto wheel=[&](juce::Slider& s,const char* id){s.setComponentID(id);s.setSliderStyle(juce::Slider::LinearVertical);s.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);s.setScrollWheelEnabled(false);s.setColour(juce::Slider::trackColourId,colour(0));addAndMakeVisible(s);};
  wheel(pitch,"pitchWheel");pitch.setRange(-1,1,.0001);pitch.setValue(0);pitch.setBounds(27,865,55,121);pitch.setTooltip("Pitch bend +/-2 semitones; returns to centre on release.");
  pitch.onValueChange=[this]{processor.uiPitch(juce::jlimit(0,16383,int(std::round(8192+8192*pitch.getValue()))));};pitch.onDragEnd=[this]{pitch.setValue(0,juce::sendNotificationSync);};
  wheel(mod,"modWheel");mod.setRange(0,1,.001);mod.setValue(0);mod.setBounds(96,865,55,121);mod.setTooltip("CC1: contact speed and evolution.");mod.onValueChange=[this]{processor.uiMod(int(std::round(127*mod.getValue())));};
  voicesLabel.setFont(font(14));voicesLabel.setColour(juce::Label::textColourId,ink());voicesLabel.setJustificationType(juce::Justification::centred);voicesLabel.setBounds(1398,862,120,30);addAndMakeVisible(voicesLabel);
  octaveLabel.setFont(font(18));octaveLabel.setColour(juce::Label::textColourId,ink());octaveLabel.setJustificationType(juce::Justification::centred);octaveLabel.setBounds(1436,953,38,32);octaveLabel.setText("0",juce::dontSendNotification);addAndMakeVisible(octaveLabel);
  octaveDown.setComponentID("octaveDown");octaveUp.setComponentID("octaveUp");octaveDown.setBounds(1406,953,30,32);octaveUp.setBounds(1474,953,30,32);
  auto move=[this](int d){octave=std::clamp(octave+d,-2,3);keys.setLowestVisibleKey(48+octave*12);octaveLabel.setText(juce::String(octave),juce::dontSendNotification);};
  octaveDown.onClick=[move]{move(-1);};octaveUp.onClick=[move]{move(1);};addAndMakeVisible(octaveDown);addAndMakeVisible(octaveUp);
  selectTab(0);startTimerHz(20);
 }
 ~Canvas()override{stopTimer();setLookAndFeel(nullptr);}
 void selectTab(int i){selected=i;for(int j=0;j<5;++j)tabs[j].setToggleState(j==i,juce::dontSendNotification);
  for(size_t j=0;j<details.size();++j)details[j]->setVisible(detailWorld[j]==i);for(size_t j=0;j<selectors.size();++j)selectors[j]->setVisible(selectorWorld[j]==i);repaint();}
 void paint(juce::Graphics& g)override{
  g.fillAll(juce::Colour(0xff020b12));
  // Deterministic small stars outside the control surfaces.
  juce::Random stars(1985);for(int i=0;i<155;++i){int x=stars.nextInt(1536),y=stars.nextInt(1024);g.setColour(ink().withAlpha(.08f+stars.nextFloat()*.13f));g.fillEllipse(float(x),float(y),1.2f,1.2f);}
  auto box=[&](juce::Rectangle<int> r,juce::Colour c){g.setColour(panel().withAlpha(.92f));g.fillRoundedRectangle(r.toFloat(),6);g.setColour(c.withAlpha(.4f));g.drawRoundedRectangle(r.toFloat(),6,1);};
  box({15,101,1506,156},ink());box({15,337,373,505},colour(selected));box({1126,337,395,505},colour(selected));box({15,853,1506,157},ink());
  // Abstract disc mark, drawn as vectors rather than a bitmap brand.
  g.setColour(colour(1).withAlpha(.75f));for(int i=0;i<3;++i){juce::Path p;p.addEllipse(-31.f-i*4,-15.f-i*3,62.f+i*8,30.f+i*6);p.applyTransform(juce::AffineTransform::rotation(-.65f).translated(60,52));g.strokePath(p,juce::PathStrokeType(1));}
  g.setGradientFill(juce::ColourGradient(juce::Colours::white,60,52,colour(1).withAlpha(0.f),78,68,true));g.fillEllipse(42,34,36,36);
  text(g,"BANK",{491,4,197,17},12,ink().withAlpha(.8f));text(g,"PATCH",{738,4,292,17},12,ink().withAlpha(.8f));text(g,"B L A Z A R",{111,34,367,40},36);text(g,"PHYSICAL MODELLING SYNTHESIZER",{112,78,365,17},12,ink().withAlpha(.7f));
  g.setColour(ink().withAlpha(.25f));for(int x:{538,1175})g.drawVerticalLine(x,121,234);
  text(g,"PITCH",{22,987,65,21},14);text(g,"MOD",{93,987,65,21},14);text(g,"OCTAVE",{1393,921,125,27},14,ink(),juce::Justification::centred);
  if(selected==0){text(g,"Choose an object. Give it energy.",{42,435,322,25},17,ink().withAlpha(.72f));text(g,"RESONATOR / NOTE SHAPE",{1147,350,355,28},18);text(g,"Force and Speed sustain Friction / Air.\nSpeed is unused by one-shot Strike / Pluck.\nAttack / Release shape the note lifecycle.",{1148,580,346,66},16,ink().withAlpha(.7f));}
  if(selected==1){text(g,"Organised wave energy. Spectral geometry,\npropagation and coupled motion.",{42,438,322,48},17,ink().withAlpha(.72f));text(g,"WAVES THAT DISTURB MATTER",{1147,354,355,42},19);text(g,"Independent FIELD Structure / Tone.\nAttack / Release shape its audible lifetime.",{1148,587,347,70},17,ink().withAlpha(.8f));}
  if(selected==2){text(g,"Discrete events. RATE sets timing scale;\nDENSITY sets population activity.",{42,438,322,48},17,ink().withAlpha(.72f));text(g,"SOUND                    ACTION",{1147,359,349,31},19,colour(2));text(g,"SELF: audible events.\nIMPACT: force into Matter, independently.\n\nSELF = 0  /  IMPACT > 0\nInvisible particles playing another world.\nParticle energy can also disturb FIELD.",{1148,580,348,102},17,ink().withAlpha(.85f));}
  if(selected==3){text(g,"Continuous turbulent energy.\nFlow, pressure fronts and nonlinear motion.",{42,438,322,48},17,ink().withAlpha(.72f));text(g,"SOUND                    ACTION",{1147,359,349,31},19,colour(3));text(g,"SELF: sound. DRIVE: force into other worlds.\nSELF = 0 / DRIVE > 0: an invisible medium.",{1148,528,348,40},16,ink().withAlpha(.85f));}
  if(selected==4){text(g,"HOW THE WORLDS AFFECT ONE ANOTHER",{39,355,337,50},18,colour(4));
   const char* routes[]={"FIELD  \xe2\x86\x92  MATTER","MATTER  \xe2\x86\x92  FIELD","PARTICLES  \xe2\x86\x92  MATTER","PARTICLES  \xe2\x86\x92  FIELD","PLASMA  \xe2\x86\x92  MATTER / FIELD","SPACE  \xe2\x86\x92  MATTER"};
   const char* laws[]={"Continuous force / boundary exchange","Modal motion / phase disturbance","Event waveform x IMPACT","Event-energy disturbance","Turbulent force x DRIVE","Delayed return force"};
   for(int i=0;i<6;++i){text(g,juce::String::fromUTF8(routes[i]),{40,435+i*59,329,27},17,colour(i<2?1:i<4?2:i==4?3:4));text(g,laws[i],{40,461+i*59,329,24},14,ink().withAlpha(.75f));}
   text(g,"ONE INTERACTION MACRO",{1148,349,348,28},18,colour(4));text(g,"TENSEGRITY scales the existing internal\nroute coefficients.\n\nPARTICLE IMPACT and PLASMA DRIVE\nset independent source actions in their tabs.\n\nThese paths are real DSP relationships.\nIndividual routing gains are not exposed\nas host parameters in this version.\n\nSPACE also returns energy to Matter.",{1148,602,347,224},18,ink().withAlpha(.85f));}
 }
};
class Editor final:public juce::AudioProcessorEditor {
 Canvas canvas;juce::ComponentBoundsConstrainer constraint;
public:
 explicit Editor(Processor& p):AudioProcessorEditor(p),canvas(p){
  addAndMakeVisible(canvas);setResizable(true,true);constraint.setFixedAspectRatio(1.5);constraint.setSizeLimits(960,640,1920,1280);setConstrainer(&constraint);
  int height=800;if(auto* display=juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())height=juce::jlimit(640,800,std::min(display->userArea.getHeight()-80,(display->userArea.getWidth()-40)*2/3));
  height-=height%2;setSize(height*3/2,height);
 }
 ~Editor()override{setConstrainer(nullptr);}
 void resized()override{float scale=getWidth()/1536.f;canvas.setTransform(juce::AffineTransform::scale(scale));canvas.setBounds(0,0,1536,1024);}
};
}
