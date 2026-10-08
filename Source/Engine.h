#pragma once
#include <array>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdint>
#include "MatterModels.h"
namespace quasar {
constexpr double pi=3.14159265358979323846;
inline double clip(double x,double a=0,double b=1){return std::clamp(x,a,b);}
struct Settings {
 double matter=.65,field=.65,particles=.3,coupling=.65,motion=.3,evolve=.65,space=.75,energy=.6;
 double tone=.25,structure=.5,density=.35,attack=1.2,release=5,pulse=0;
 int matterModel=0;
 int fieldModel=0,particleModel=0;
 double fieldShape=.5,fieldBrightness=.5,fieldModulation=.5,fieldSpread=.5;
 double particleRate=.5,particleDecay=.5,particlePitch=.5,particleSpread=.5;
 int exciter=0,plasmaModel=0;
 double pulseRate=1; // continuous cycles per host beat: 0.25 to 4
 double force=.45,hardness=.5,position=.27,speed=.45;
 double particleSelf=1,particleImpact=.65;
 double fieldStructure=.5,fieldTone=.25,fieldAttack=1.2,fieldRelease=5;
 double plasmaRate=.5,plasmaCoherence=.5,plasmaIntermittency=.5,plasmaResonance=.5;
 double plasma=0,plasmaSelf=1,plasmaDrive=.6,plasmaFlow=.5,plasmaPressure=.5,plasmaTurbulence=.5,plasmaScale=.5;
};
constexpr int legacyProgramCount=28;
inline const char* legacyPatchName(int i){static const char* n[]={"Event Horizon","Solar Wind","Glass Nebula","Pulsar","Dark Matter","First Light","Stiff String","Bronze Bar","Impossible Plate","Taut Membrane","Broken Glass","Deep Bell","Weird Matter","Heavy Harmonics","Dry Bronze Pluck","Droplet Keys","Spark Chimes","Hollow Reed","Metallic FM","Glass Shimmer","Soft Cluster Keys","Bright Digital","Chirp Radar","Sparse Bubbles","Dust Cloud","Air Observatory","Stone Knocks","Strange Scrapes"};return n[std::clamp(i,0,legacyProgramCount-1)];}
inline Settings legacyPatch(int i){
 switch(i){
 case 1:return {.35,.45,.8,.6,.6,.8,.65,.5,.65,.3,.7,.9,4,0};
 case 2:return {.85,.3,.35,.7,.35,.65,.75,.5,.75,.9,.35,.4,6,0};
 case 3:return {.65,.75,.4,.75,.65,.65,.45,.6,.55,.25,.55,.3,3,.85};
 case 4:return {.75,.4,.5,.9,.15,.9,.8,.5,.12,1,.45,2,7,0};
 case 5:return {.25,.85,.15,.25,.3,.4,.55,.6,.3,.12,.2,.6,3.5,0};
 case 6:return {.9,.45,.18,.72,.32,.55,.55,.6,.35,.65,.24,.5,5,0,1};
 case 7:return {.9,.25,.45,.8,.25,.45,.55,.65,.55,.6,.4,.03,5,0,2};
 case 8:return {.85,.4,.65,.88,.32,.7,.8,.58,.5,.85,.6,1.3,7,0,3};
 case 9:return {.9,.18,.65,.68,.3,.45,.45,.6,.4,.65,.4,.08,3,0,4};
 case 10:return {.95,.2,.5,.85,.25,.6,.7,.5,.8,.8,.3,.07,8,0,5};
 case 11:return {.95,.22,.65,.82,.2,.5,.65,.58,.5,.55,.3,.06,8,0,6};
 case 12:return {.85,.45,.65,.9,.35,.85,.7,.55,.65,.85,.65,1.2,6,0,7};
 default:{
  if(i<13)return {};
  Settings p;
  p.attack=.04;p.release=2;p.space=.35;p.motion=.25;p.evolve=.35;
  p.fieldShape=.5;p.fieldBrightness=.55;p.fieldModulation=.4;p.fieldSpread=.35;
  p.particleRate=.45;p.particleDecay=.45;p.particlePitch=.5;p.particleSpread=.5;
  switch(i){
   case 13:p.fieldModel=1;p.field=.95;p.matter=.25;p.particles=.04;p.tone=.1;p.structure=.1;p.coupling=.25;p.release=1;break;
   case 14:p.matterModel=2;p.matter=.95;p.field=0;p.particles=.06;p.particleModel=3;p.particleRate=.05;p.density=0;p.coupling=.5;p.structure=.7;p.release=1.5;p.space=.15;break;
   case 15:p.fieldModel=7;p.field=.2;p.matterModel=5;p.matter=.5;p.particles=.8;p.particleModel=1;p.particleRate=.4;p.density=.5;p.coupling=.75;break;
   case 16:p.field=.05;p.matterModel=6;p.matter=.85;p.particles=.9;p.particleModel=2;p.particleRate=.6;p.particlePitch=.7;p.coupling=.85;p.space=.55;break;
   case 17:p.fieldModel=5;p.field=.95;p.particles=.05;p.matter=.2;p.fieldShape=.8;p.fieldBrightness=.25;p.coupling=.35;p.attack=.15;break;
   case 18:p.fieldModel=3;p.field=.8;p.fieldModulation=.9;p.matterModel=2;p.matter=.7;p.particles=.15;p.coupling=.85;p.tone=.7;break;
   case 19:p.fieldModel=6;p.field=.7;p.matterModel=5;p.matter=.65;p.particles=.25;p.particleModel=0;p.fieldBrightness=.85;p.space=.65;break;
   case 20:p.fieldModel=4;p.field=.8;p.fieldShape=.15;p.fieldBrightness=.3;p.matter=.3;p.particles=.08;p.coupling=.3;p.attack=.2;break;
   case 21:p.fieldModel=2;p.field=.9;p.fieldShape=.8;p.fieldBrightness=.9;p.fieldModulation=.65;p.matterModel=7;p.matter=.3;p.particles=.08;p.space=.25;break;
   case 22:p.fieldModel=3;p.field=.15;p.matter=.1;p.particles=.9;p.particleModel=4;p.pulse=.8;p.particleRate=.7;p.particleSpread=.8;p.density=.5;break;
   case 23:p.field=0;p.matter=.15;p.particles=.95;p.particleModel=6;p.particleRate=.1;p.density=.3;p.particleDecay=.65;p.space=.65;p.coupling=.5;break;
   case 24:p.fieldModel=7;p.field=.25;p.matterModel=3;p.matter=.65;p.particles=.8;p.particleModel=7;p.particleRate=.95;p.density=.9;p.particlePitch=.7;p.coupling=.8;p.space=.6;break;
   case 25:p.field=.08;p.matterModel=3;p.matter=.3;p.particles=.9;p.particleModel=7;p.particleRate=.1;p.density=.05;p.tone=.8;p.attack=1;p.release=5;p.space=.85;p.evolve=.85;break;
   case 26:p.field=0;p.matterModel=4;p.matter=.85;p.particles=.8;p.particleModel=3;p.particlePitch=.25;p.particleDecay=.35;p.particleRate=.3;p.coupling=.9;p.space=.2;break;
   case 27:p.fieldModel=8;p.field=.4;p.matterModel=7;p.matter=.7;p.particles=.8;p.particleModel=5;p.particleDecay=.8;p.particleRate=.6;p.particleSpread=.9;p.coupling=.9;p.evolve=.8;p.space=.55;break;
  }
  return p;
 }
 }}

constexpr int programCount=40;
inline const char* patchName(int i){
 static const char* names[]={"Bowed Impossible Object","Breathing Plate","Invisible Mallets","Dust on Glass","Scraped Universe","Standing Field","Matter Disturbs Field","Plasma Engine","Invisible Storm","Unstable Equilibrium","Playable Air","Playable Friction"};
 return i<28?legacyPatchName(i):names[std::clamp(i-28,0,11)];
}
inline Settings synthesisPatch(int i){
 if(i<28){auto p=legacyPatch(i);p.plasma=.15*p.particles;p.particleImpact=.65;return p;}
 Settings p;p.matter=.9;p.field=0;p.particles=0;p.coupling=.75;p.evolve=.45;p.motion=.35;p.space=.35;p.attack=.04;p.release=2;
 p.force=.55;p.speed=.4;p.hardness=.45;p.particleSelf=0;p.particleImpact=.9;
 switch(i){
 case 28:p.matterModel=3;p.exciter=2;p.position=.37;p.structure=.6;p.force=.65;break;
 case 29:p.matterModel=3;p.exciter=3;p.force=.7;p.position=.31;p.speed=.65;break;
 case 30:p.matterModel=6;p.force=0;p.particles=.85;p.particleModel=3;p.particleRate=.28;p.density=.35;p.particlePitch=.3;break;
 case 31:p.matterModel=5;p.force=0;p.particles=1;p.particleModel=7;p.particleRate=.85;p.density=.8;p.particleDecay=.3;p.particlePitch=.65;break;
 case 32:p.matterModel=1;p.exciter=4;p.field=.35;p.fieldModel=10;p.force=.8;p.speed=.75;p.coupling=.9;p.structure=.8;break;
 case 33:p.matter=.12;p.force=0;p.field=1;p.fieldModel=9;p.fieldShape=.65;p.fieldSpread=.75;p.coupling=.35;p.space=.55;break;
 case 34:p.matterModel=4;p.exciter=2;p.field=.7;p.fieldModel=10;p.fieldModulation=.8;p.coupling=.95;p.force=.6;break;
 case 35:p.matterModel=7;p.exciter=5;p.plasma=.85;p.plasmaModel=2;p.plasmaSelf=.8;p.plasmaDrive=.85;p.field=.3;p.fieldModel=10;p.force=.3;p.coupling=.9;break;
 case 36:p.matterModel=3;p.force=0;p.plasma=1;p.plasmaModel=1;p.plasmaSelf=0;p.plasmaDrive=1;p.plasmaPressure=.85;p.plasmaTurbulence=.85;p.field=.25;p.fieldModel=9;p.space=.6;break;
 case 37:p.matterModel=7;p.exciter=5;p.force=.85;p.speed=.8;p.hardness=.8;p.fieldModel=10;p.field=.4;p.plasma=.25;p.plasmaModel=2;p.coupling=.95;p.space=.6;break;
 case 38:p.matterModel=1;p.exciter=3;p.force=.55;p.hardness=.3;p.speed=.45;p.position=.23;p.space=.25;break;
 case 39:p.matterModel=1;p.exciter=2;p.force=.6;p.hardness=.6;p.speed=.3;p.position=.18;p.space=.25;break;
 }
 return p;
}
inline Settings patch(int i){auto p=synthesisPatch(i);p.fieldStructure=p.structure;p.fieldTone=p.tone;p.fieldAttack=p.attack;p.fieldRelease=p.release;int oldRate=int(std::floor(1+p.pulse*3));p.pulseRate=oldRate;return p;}
struct Random {uint32_t state=1; double unit(){state^=state<<13;state^=state>>17;state^=state<<5;return state/4294967296.0;}double bi(){return unit()*2-1;}};
struct Sine {
 std::array<double,2049> t{};
 Sine(){for(int i=0;i<=2048;++i)t[i]=std::sin(2*pi*i/2048);}
 double operator()(double p)const {p-=double(static_cast<int64_t>(p));if(p<0)p+=1;if(p>=1)p=0;double f=p*2048;int i=int(f);return t[i]+(t[i+1]-t[i])*(f-i);}
};
struct Frame {double l=0,r=0,particle=0,matterL=0,matterR=0;};
struct Channel {double bend=0,wheel=0,pressure=0;bool sustain=false;};
struct Mode {
 double x=0,y=0,c=1,s=0,r=.99,pan=0;
 double tick(double input){double a=x;x=r*(c*x-s*y)+input;y=r*(s*a+c*y);return x;}
 void tune(double f,double sr,double decay){double w=2*pi*clip(f,10,sr*.39)/sr;c=std::cos(w);s=std::sin(w);r=std::exp(-6.907755/(sr*decay));}
};
#include "Limits.h"
#include "WorldModels.h"
#include "Dynamics.h"
class Delay {
 std::vector<double> data;size_t head=0;
public:
 void prepare(size_t n){data.assign(n,0);head=0;}
 void clear(){std::fill(data.begin(),data.end(),0);head=0;}
 double read(double delay)const{
  double pos=double(head)-clip(delay,1,double(data.size()-2));if(pos<0)pos+=data.size();
  size_t a=size_t(pos),b=(a+1)%data.size();return data[a]+(data[b]-data[a])*(pos-a);
 }
 void write(double x){data[head]=x;head=(head+1)%data.size();}
};
class Space {
 std::array<Delay,4> line;std::array<Delay,2> echo;
 std::array<double,4> low{};double sr=48000,phase=0,delayTime=18000,quiet=0;
public:
 double returned=0;
 void prepare(double rate){sr=rate;for(auto& d:line)d.prepare(size_t(sr*.2)+4);for(auto& d:echo)d.prepare(size_t(sr*2)+4);clear();}
 void clear(){for(auto& d:line)d.clear();for(auto& d:echo)d.clear();low={};returned=phase=quiet=0;delayTime=sr*.375;}
 Frame tick(Frame input,const Settings& p,double bpm,bool active,const Sine& sine){
  if constexpr(!Limits::spaceEnabled){returned=0;return input;}
  static const double seconds[]={.0371,.0533,.0719,.0893};
  phase+=(.07+.13*p.motion*p.evolve)/sr;phase-=std::floor(phase);
  std::array<double,4> a{};
  for(int i=0;i<4;++i){double x=line[i].read(sr*(seconds[i]+.00035*p.space*sine(phase+i*.23)));low[i]+=.25*(x-low[i]);a[i]=low[i];}
  std::array<double,4> mix{{(a[0]+a[1]+a[2]+a[3])*.5,(a[0]-a[1]+a[2]-a[3])*.5,(a[0]+a[1]-a[2]-a[3])*.5,(a[0]-a[1]-a[2]+a[3])*.5}};
  delayTime+=.00002*(sr*60/clip(bpm,40,240)*.75-delayTime);
  double el=echo[0].read(delayTime),er=echo[1].read(delayTime*1.013);
  for(int i=0;i<4;++i){double in=(i%2?input.r:input.l)*.3+input.particle*.15+(i%2?el:er)*.025;line[i].write(std::tanh(in*p.space+mix[i]*(.4+.44*p.space)));}
  double wl=(a[0]+a[1]-a[2])*.7,wr=(a[1]+a[2]-a[3])*.7;
  echo[0].write(std::tanh((input.l*.25+wl*.06)*p.space+er*(.25+.38*p.space)));
  echo[1].write(std::tanh((input.r*.25+wr*.06)*p.space+el*(.25+.38*p.space)));
  returned=std::tanh((wl+wr+el+er)*.3);
  double wet=.65*p.space;Frame out{input.l*(1-.4*wet)+wet*(wl+.7*el),input.r*(1-.4*wet)+wet*(wr+.7*er),0};
  if(!active&&std::abs(out.l)+std::abs(out.r)+std::abs(returned)<1e-8){if(++quiet>sr*2.5)clear();}else quiet=0;
  return out;
 }
};
class Engine {
 std::array<Voice,Limits::voices> voices;
 struct Retired {Mode l,r;double particle=0,fade=0;int channel=0;};std::array<Retired,Limits::voices> retired;
 std::array<Channel,16> channels{};
 // Only the low bits are used for scheduling; unsigned wrap is intentional.
 Settings settings,targetSettings;bool fresh=true;uint32_t controlClock=0;Space space;Sine sine;Random rng;double sr=48000,bpm=120,motionPhase=0,beat=0,pulsePhase=0,drift=0,driftVelocity=0,evolutionRate=.12,target=0,next=0;
 double previousL=0,previousR=0,dcL=0,dcR=0,preSafety=0;uint32_t serial=1;size_t retirement=0;
public:
 void prepare(double rate){sr=rate;space.prepare(rate);reset();}
 void reset(){fresh=true;controlClock=0;for(auto& v:voices)v.active=false;for(auto& r:retired)r.fade=0;channels={};space.clear();motionPhase=beat=pulsePhase=drift=driftVelocity=target=next=previousL=previousR=dcL=dcR=0;rng.state=1;serial=1;retirement=0;preSafety=0;}
 void setSettings(Settings p){targetSettings=p;if(fresh){settings=p;fresh=false;}}
 void setTempo(double x){bpm=clip(x,40,240);}
 double preSafetyPeak()const{return preSafety;}
 double internalEnergy()const{double x=0;for(auto& v:voices)if(v.active)x=std::max(x,v.internalEnergy());return x;}
 int activeVoices()const{int n=0;for(auto& v:voices)if(v.active)++n;return n;}
 void noteOn(int note,int ch,double vel){
  ch=std::clamp(ch,0,15);Voice* v=nullptr;
  for(auto& q:voices)if(!q.active){v=&q;break;}
  if(!v){v=&*std::min_element(voices.begin(),voices.end(),[](const Voice&a,const Voice&b){return a.level()*(a.keyDown?2:1)<b.level()*(b.keyDown?2:1);});
   auto& old=retired[retirement++%Limits::voices];old.fade=1;old.channel=v->channel;
   auto last=v->continuation(),previous=v->previous();
   auto seedTail=[&](Mode& tail,double x,double previous){
    tail.tune(v->frequency(),sr,.02);tail.x=x;
    double y=(previous-tail.c*x)/std::max(.005,std::abs(tail.s));
    tail.y=clip(y,-std::max(.03,3*std::abs(x)),std::max(.03,3*std::abs(x)));
   };
   seedTail(old.l,last.l,previous.l);seedTail(old.r,last.r,previous.r);old.particle=last.particle;}
  v->start(std::clamp(note,0,127),ch,clip(vel),sr,(++serial)*2654435761u+note);
 }
 void noteOff(int note,int ch){ch=std::clamp(ch,0,15);for(auto& v:voices)if(v.active&&v.note==note&&v.channel==ch){v.keyDown=false;v.sustained=channels[ch].sustain;}}
 void sustain(int ch,bool on){ch=std::clamp(ch,0,15);channels[ch].sustain=on;if(!on)for(auto& v:voices)if(v.channel==ch&&!v.keyDown)v.sustained=false;}
 void bend(int ch,double x){channels[std::clamp(ch,0,15)].bend=clip(x,-1,1);}
 void wheel(int ch,double x){channels[std::clamp(ch,0,15)].wheel=clip(x);}
 void pressure(int ch,double x){channels[std::clamp(ch,0,15)].pressure=clip(x);}
 void releaseChannel(int ch){
  ch=std::clamp(ch,0,15);
  for(auto& v:voices)if(v.channel==ch)v.release();
  channels[ch].sustain=false;
 }
 void silenceChannel(int ch){
  ch=std::clamp(ch,0,15);
  for(auto& v:voices)if(v.channel==ch)v.active=false;
  for(auto& r:retired)if(r.channel==ch)r.fade=0;
  // Space and the DC filter are shared. Preserve other channels' voices and
  // stolen-note tails; clear the remaining effects once all voices are silent.
  const bool hasRetired=std::any_of(retired.begin(),retired.end(),[](const Retired& r){return r.fade>0;});
  if(activeVoices()==0&&!hasRetired){space.clear();previousL=previousR=dcL=dcR=preSafety=0;}
 }
 void releaseAll(){for(auto& v:voices)v.release();for(auto& c:channels)c.sustain=false;}
 Frame tick(){
  if((controlClock++&(sr>=64000?63:31))==0){
   auto slew=[](double& a,double b){a+=(b-a)*.025;};
   slew(settings.matter,targetSettings.matter);slew(settings.field,targetSettings.field);slew(settings.particles,targetSettings.particles);slew(settings.coupling,targetSettings.coupling);
   slew(settings.motion,targetSettings.motion);slew(settings.evolve,targetSettings.evolve);slew(settings.space,targetSettings.space);slew(settings.energy,targetSettings.energy);
   slew(settings.fieldTone,targetSettings.fieldTone);slew(settings.fieldStructure,targetSettings.fieldStructure);
   slew(settings.tone,targetSettings.tone);slew(settings.structure,targetSettings.structure);slew(settings.density,targetSettings.density);slew(settings.pulse,targetSettings.pulse);
   slew(settings.fieldShape,targetSettings.fieldShape);slew(settings.fieldBrightness,targetSettings.fieldBrightness);
   slew(settings.fieldModulation,targetSettings.fieldModulation);slew(settings.fieldSpread,targetSettings.fieldSpread);
   slew(settings.particleRate,targetSettings.particleRate);slew(settings.particleDecay,targetSettings.particleDecay);
   slew(settings.particlePitch,targetSettings.particlePitch);slew(settings.particleSpread,targetSettings.particleSpread);
   settings.fieldModel=targetSettings.fieldModel;settings.particleModel=targetSettings.particleModel;
   settings.exciter=targetSettings.exciter;settings.plasmaModel=targetSettings.plasmaModel;
   slew(settings.force,targetSettings.force);slew(settings.hardness,targetSettings.hardness);slew(settings.position,targetSettings.position);slew(settings.speed,targetSettings.speed);
   slew(settings.particleSelf,targetSettings.particleSelf);slew(settings.particleImpact,targetSettings.particleImpact);
   slew(settings.plasma,targetSettings.plasma);slew(settings.plasmaSelf,targetSettings.plasmaSelf);slew(settings.plasmaDrive,targetSettings.plasmaDrive);
   slew(settings.plasmaRate,targetSettings.plasmaRate);slew(settings.plasmaCoherence,targetSettings.plasmaCoherence);slew(settings.plasmaIntermittency,targetSettings.plasmaIntermittency);slew(settings.plasmaResonance,targetSettings.plasmaResonance);
   slew(settings.plasmaFlow,targetSettings.plasmaFlow);slew(settings.plasmaPressure,targetSettings.plasmaPressure);slew(settings.plasmaTurbulence,targetSettings.plasmaTurbulence);slew(settings.plasmaScale,targetSettings.plasmaScale);
   slew(settings.pulseRate,clip(targetSettings.pulseRate,.25,4.));
   evolutionRate=.015*std::exp2(8*settings.motion);
   settings.fieldAttack=targetSettings.fieldAttack;settings.fieldRelease=targetSettings.fieldRelease;
   settings.attack=targetSettings.attack;settings.release=targetSettings.release;settings.matterModel=targetSettings.matterModel;
  }
  // Keep phase unwrapped: fractional frequency trajectories must not jump at wraps.
  motionPhase+=evolutionRate*.22/sr;beat+=bpm/(60*sr);pulsePhase+=bpm*settings.pulseRate/(60*sr);pulsePhase-=std::floor(pulsePhase);
  if(next--<=0){target=rng.bi();next=sr*(.6+.8*rng.unit())/evolutionRate;}
  driftVelocity+=(6*evolutionRate*(target-drift)-4*driftVelocity)*evolutionRate/sr;
  drift+=driftVelocity/sr;
  double shared=clip(drift,-1.,1.);
  double depth=clip(settings.pulse);
  // Low depth is gentle; high depth closes fully with a tighter rhythmic opening.
  double openness=std::pow(.5+.5*sine(pulsePhase),1.5+4.5*depth);
  double pulse=1-depth+depth*openness;
  double returned=space.returned;Frame sum{};
  for(auto& v:voices){auto a=v.tick(settings,channels[v.channel],shared,pulse,returned,sine);sum.l+=a.l;sum.r+=a.r;sum.particle+=a.particle;sum.matterL+=a.matterL;sum.matterR+=a.matterR;}
  for(auto& r:retired)if(r.fade>0){sum.l+=r.l.tick(0)*r.fade;sum.r+=r.r.tick(0)*r.fade;sum.particle+=r.particle*r.fade;r.fade-=1/(sr*.008);}
  auto out=space.tick(sum,settings,bpm,activeVoices()>0,sine);
  // Audible Matter trim only: do not change Space input or any feedback force.
  const double matterTrim=-.25*(1-.4*.65*settings.space);
  out.l+=sum.matterL*matterTrim;out.r+=sum.matterR*matterTrim;
  dcL=out.l-previousL+.9997*dcL;dcR=out.r-previousR+.9997*dcR;previousL=out.l;previousR=out.r;
  preSafety=std::max(std::abs(dcL),std::abs(dcR));
  // +6.02 dB output gain; linear until 0.65, smoothly bounded below 0.95.
  auto protect=[](double x){x*=1.9;double a=std::abs(x);return a<=.65?x:std::copysign(.65+.30*std::tanh((a-.65)/.30),x);};
  return {protect(dcL),protect(dcR),0};
 }
};
}


