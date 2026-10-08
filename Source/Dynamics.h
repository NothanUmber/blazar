// Included within namespace quasar. All synthesis state is bounded/fixed-size.
inline double soft(double x){return x/(1+std::abs(x));}
struct Interaction {
 double particlesMatter=1.8,fieldMatter=.16,matterField=.10,particlesField=.07,plasmaMatter=.6,plasmaField=.12,spaceMatter=.12;
};
struct PlasmaProcess {
 Random rng;
 std::array<double,6> position{},velocity{},goal{},hold{},heat{};
 std::array<double,6> real{},imag{},cx{},cy{},tx{},ty{},drive{},targetDrive{},weight{},targetWeight{};
 double amount=.5,scale=.5,energy=.5,flow=.5,renewal=1,link=.5,gaps=.5,resonance=.5;
 double sr=48000,dt=.008,time=1,pressure=.55,pressureVelocity=0,pressureGoal=.55,pressureHold=0;
 double reservoir=.4,front=.3,recovery=0,x=.17,y=-.23,z=.31,detail=0,dc=0;
 double slew=.001,dcPole=.0005,detailLoss=.999,carrierCorrelation=.4,carrierNormal=1;
 int clock=0,interval=400;bool initialised=false,draining=false;
 void configure(const Settings& p,double sampleRate){
  sr=sampleRate;amount=clip(p.plasmaTurbulence);scale=clip(p.plasmaScale);energy=clip(p.plasmaPressure);flow=clip(p.plasmaFlow);
  renewal=std::exp2(3*(clip(p.plasmaRate)-.5));link=clip(p.plasmaCoherence);gaps=clip(p.plasmaIntermittency);resonance=clip(p.plasmaResonance);
  interval=std::max(1,int(sr/120));dt=interval/sr;
  time=.15*std::exp2(6*scale)/((.3+2*flow)*renewal);
  slew=1-std::exp(-1/(sr*.035));dcPole=1-std::exp(-2*pi*5/sr);detailLoss=std::exp(-1/(sr*(.006+.08*scale)));
  carrierCorrelation=.8*link;carrierNormal=1/std::sqrt(carrierCorrelation*carrierCorrelation+(1-carrierCorrelation)*(1-carrierCorrelation));
 }
 void advance(int model){
  static constexpr double times[]={3.8,2.4,1.6,1.1,.8,.5};
  static constexpr double centres[]={28,90,280,850,2600,8000};
  static constexpr double colours[12][6]={
   {1,.9,.65,.45,.22,.12},{1.3,1,.65,.45,.28,.14},{.6,.8,1,.8,.7,.45},
   {1,.85,.85,.65,.35,.18},{.25,.45,.8,1,.8,.45},{.15,.4,.7,1,.7,.35},
   {.7,1,1,.6,.5,.2},{.1,.25,.5,.9,1,.7},{.7,.9,1,.85,.5,.25},
   {.9,1,.75,.8,.6,.25},{1,.9,.75,.45,.22,.12},{.55,.7,.8,1,.8,.45}};
  model=std::clamp(model,0,11);
  double mediumTime=time*(model==10?4.:model==1||model==3?1.7:1.);
  double h=std::min(.08,dt/std::max(.02,mediumTime));
  if(pressureHold<=0){pressureGoal=clip(.35+.45*energy+(.2+.4*gaps)*rng.bi(),.08,1.25);pressureHold=mediumTime*(.7+2*rng.unit());}
  pressureHold-=dt;
  pressureVelocity+=h*(2.2*(pressureGoal-pressure)-2.7*pressureVelocity);
  pressure=clip(pressure+h*pressureVelocity,.06,1.4);
  auto previous=position;double common=rng.bi(),average=0;
  for(int i=0;i<6;++i){
   double localTime=mediumTime*times[i];double step=std::min(.08,dt/std::max(.015,localTime));
   if(hold[i]<=0){goal[i]=(.28+.72*amount)*((1-link*.65)*rng.bi()+link*.65*common);hold[i]=localTime*(.6+1.6*rng.unit());}
   hold[i]-=dt;
   double left=previous[(i+5)%6],right=previous[(i+1)%6];
   double force=2.5*(goal[i]-previous[i])+.7*link*(left+right-2*previous[i])+.5*flow*(left-previous[i]);
   if(model==4)force+=(.15+amount)*velocity[(i+1)%6]*(pressure-.35); // flexible flutter within a broad gust
   if(model==6)force-=amount*.8*soft(velocity[i]*8); // continuous frictional sliding/contact
   if(model==9)force+=1.4*flow*(left-right)*(.4+pressure); // circulating pressure transport
   if(model==8){heat[i]=clip(heat[i]+step*(energy*(.25+.5*pressure)-.35*heat[i]-.2*std::abs(velocity[i])),0.,2.);force+=.65*(heat[i]-.5)-.25*previous[i]*std::abs(previous[i]);}
   if(model==5)force+=.3*(pressure-previous[(i+3)%6]); // overlapping transported rainfall densities
   velocity[i]=clip(velocity[i]+step*(force-(1.8+.7*(1-amount))*velocity[i]),-4.,4.);
   position[i]=clip(previous[i]+step*velocity[i],-1.5,1.5);average+=position[i]/6;
  }
  // Storm/Surf store pressure and release it into a smooth advancing/retreating front.
  if(model==1||model==3){
   if(reservoir>.8+.3*gaps)draining=true;else if(reservoir<.15)draining=false;
   reservoir=clip(reservoir+h*(draining?-(.22+.25*flow)*reservoir:.12+.3*energy+.12*average),.05,1.5);
   double desired=draining?reservoir*(model==3?1.5:1.):.12+reservoir*.6;
   recovery+=h*.18*(desired-recovery);front+=h*(model==3?.65:.3)*(desired-front);
  }else front+=h*.2*(pressure-front);
  if(model==2||model==11){
   double k=std::min(.03,h*.8),dx=x-x*x*x/3-y+.25+.6*pressure+.2*average;
   double dy=.16*(x+.65-.8*y),dz=.22*(soft(x*y+position[3]*2)-z);
   x=clip(x+k*dx,-2.5,2.5);y=clip(y+k*dy,-2.,2.);z=clip(z+k*dz,-1.,1.);
   if(model==11&&pressureHold<dt*2)goal[int(rng.unit()*6)]=soft((x-y*z)*3); // bounded topology folding, not a repeated trigger train
  }
  double norm=0;
  for(int i=0;i<6;++i){
   double own=position[i],other=position[(i+2)%6],travel=(1.1+2.2*amount)*own+.45*other;
   double shift=(.5+flow*.7-scale*.8)+travel;
   double emphasis=1.5*own+.6*pressure;
   if(model==1){shift+=1.5*(front-.45);emphasis+=(i/2.5-1)*2*(front-.45);}
   if(model==3){shift+=1.2*(front-recovery);emphasis+=(i/2.5-1)*(1.5*front-.6);}
   if(model==2){shift+=.65*soft(x-y)+(i%2?.8:-.8)*z;emphasis+=.7*soft(x*other);}
   if(model==4){shift+=.7*soft(velocity[i]*3);emphasis+=.4*pressure*(i-2.5);}
   if(model==5){shift+=.9*(pressure-.55);emphasis+=(i<2?-.7:.5)*pressure;}
   if(model==6){shift+=.5*soft(velocity[i]*6);emphasis+=(i<3?.5:-.2)*amount;}
   if(model==7){shift+=.8*pressure+.4*soft(velocity[i]*4);emphasis+=(i/2.5-1)*pressure;}
   if(model==8){shift+=1.1*(heat[i]-.5);emphasis+=.8*heat[(i+1)%6];}
   if(model==9){shift+=.7*(position[(i+5)%6]-other);emphasis+=.8*velocity[(i+3)%6];}
   if(model==10){shift*=.75;emphasis+=.4*average;}
   if(model==11){shift+=soft(x*other-y*own)*1.5;emphasis+=1.2*soft(z*own*5);}
   double centre=clip(centres[i]*std::exp2(clip(shift,-3.,3.)),18.,sr*.22);
   double q=.45+5.5*resonance*resonance*(.55+.45*(1-amount))*(.75+.25*soft(other*3));
   double bandwidth=clip(centre/q,3.,sr*.20);
   double radius=std::exp(-pi*bandwidth/sr),angle=2*pi*centre/sr;
   tx[i]=radius*std::cos(angle);ty[i]=radius*std::sin(angle);targetDrive[i]=std::sqrt(1-radius*radius);
   targetWeight[i]=colours[model][i]*std::exp2(clip(emphasis,-2.5,2.5))*(i>2?.35+.95*amount:.8);
   norm+=targetWeight[i];
  }
  for(int i=0;i<6;++i){targetWeight[i]/=std::max(.01,norm);if(!initialised){cx[i]=tx[i];cy[i]=ty[i];drive[i]=targetDrive[i];weight[i]=targetWeight[i];}}
  initialised=true;
  // Events are a small secondary readout of the medium, never its sustained body.
  if((model==5||model==6||model==7||model==8)&&rng.unit()<std::min(.15,dt*renewal*(.05+2*amount)*(.2+pressure)/(1+4*scale)))detail=std::min(.3,detail+.04+.12*amount);
 }
 double tick(int model,double sampleRate,double /*shared*/){
  if(clock--<=0){clock=interval-1;advance(model);}
  double commonR=rng.bi(),commonI=rng.bi(),body=0;
  for(int i=0;i<6;++i){
   cx[i]+=slew*(tx[i]-cx[i]);cy[i]+=slew*(ty[i]-cy[i]);drive[i]+=slew*(targetDrive[i]-drive[i]);weight[i]+=slew*(targetWeight[i]-weight[i]);
   double nr=(carrierCorrelation*commonR+(1-carrierCorrelation)*rng.bi())*carrierNormal;
   double ni=(carrierCorrelation*commonI+(1-carrierCorrelation)*rng.bi())*carrierNormal;
   double a=real[i],b=imag[i];real[i]=cx[i]*a-cy[i]*b+drive[i]*nr;imag[i]=cy[i]*a+cx[i]*b+drive[i]*ni;
   double e=real[i]*real[i]+imag[i]*imag[i];if(e>16){double loss=1/(1+.002*(e-16));real[i]*=loss;imag[i]*=loss;}
   if(std::abs(real[i])+std::abs(imag[i])<1e-30)real[i]=imag[i]=0;
   body+=weight[i]*real[i];
  }
  detail*=detailLoss;
  double gesture=.40+.75*pressure;
  if(model==1||model==3)gesture=.30+.60*front+.25*recovery;
  if(model==2||model==11)gesture=.40+.45*pressure+.18*std::abs(soft(x-y));
  gesture=(1-gaps)*.8+gaps*clip(gesture,.16,1.4);
  double result=body*gesture*(.35+1.25*energy)+commonR*detail*.15;
  dc+=dcPole*(result-dc);
  return soft((result-dc)*2.2);
 }
 double stateEnergy()const{double e=x*x+y*y+z*z+pressure*pressure+front*front;for(int i=0;i<6;++i)e+=real[i]*real[i]+imag[i]*imag[i]+position[i]*position[i]+velocity[i]*velocity[i]+heat[i]*heat[i];return e;}
};
struct DynamicField {
 static constexpr int capacity=std::min(8,Limits::fieldElements);int count=capacity;
 std::array<double,capacity> x{},y{},cs{},sn{},targetEnergy{};
 double phase=0,inc=.01,scatter=.001,drive=.001,loss=.999,nonlinearity=0,h2=.3,h3=.1,contactStep=.02,readScale=.35,activeRate=0;
 void configure(int model,const Settings& p,double f,double sr,double shared){
  count=std::min(capacity,sr>=64000?6:8);readScale=1/std::sqrt(double(count));
  contactStep=std::min(.12,2*pi*f/sr);inc=std::min(.35,f/sr);scatter=std::min(.08,2*pi*f/sr*(.01+.20*p.fieldSpread)*(1+.1*p.evolve*shared));
  drive=std::min(.03,2*pi*f/sr)*.40;
  h2=.05+.6*p.fieldBrightness*p.fieldBrightness;h3=.01+.5*p.fieldBrightness*p.fieldBrightness*p.fieldBrightness;
  h2*=clip((.40-2*inc)/.1);h3*=clip((.40-3*inc)/.1);
  loss=std::exp(-(8+20*(1-p.fieldModulation))/sr);
  nonlinearity=model==10?(.02+.9*p.fieldShape):0;
  activeRate=(20+100*p.fieldModulation*p.fieldModulation)/sr;
  for(int i=0;i<count;++i){
   double ratio=model==9?(i+1.)*(1+.008*p.fieldShape*i):1+std::floor(i/2.)+(i%2?.05:-.05)*p.fieldShape;
   if(model==11)ratio=std::pow(i+1.,1+.4*p.fieldShape);
   targetEnergy[i]=(.015+.12*p.energy)/std::pow(1+i,1.6-1.4*p.fieldBrightness);
   double w=2*pi*std::min(.35,f*ratio/sr);cs[i]=std::cos(w);sn[i]=std::sin(w);
  }
 }
 Frame tick(int model,double disturbance,double evolution,const Sine& sine,bool powered=true){
  phase+=inc;
  // Boundary source launches energy into the network; Matter is a second boundary.
  x[0]+=drive*(sine(phase)+h2*sine(phase*2)+h3*sine(phase*3))/(1+h2+h3)*(1+.15*evolution)*(powered?1:0)+contactStep*disturbance;
  x[count-1]+=contactStep*disturbance*.7;
  for(int i=0;i<count;++i){
   double q=x[i],p=y[i],nl=nonlinearity*(q*q+p*p)*(.6*contactStep);
   // Cayley phase rotation is bounded and norm-preserving even at large state.
   if(nl>0){double t=std::min(.2,nl),d=1/(1+t*t);double c=(1-t*t)*d,s=2*t*d;x[i]=c*q-s*p;y[i]=s*q+c*p;q=x[i];p=y[i];}
   x[i]=loss*(cs[i]*q-sn[i]*p);y[i]=loss*(sn[i]*q+cs[i]*p);
   if(model==10&&powered){
    double energy=x[i]*x[i]+y[i]*y[i];
    double radial=(1+activeRate)/(1+activeRate*energy/targetEnergy[i]);
    x[i]*=radial;y[i]*=radial;
   }
  }
  // Pairwise unitary scattering: energy is exchanged, not created by routing.
  double t=scatter,c=(1-t*t)/(1+t*t),s=2*t/(1+t*t);
  for(int parity=0;parity<2;++parity)for(int i=parity;i<count-1;i+=2){
   double a=x[i],b=x[i+1],u=y[i],v=y[i+1];
   x[i]=c*a-s*b;x[i+1]=s*a+c*b;y[i]=c*u-s*v;y[i+1]=s*u+c*v;
  }
  double total=0;for(int i=0;i<count;++i)total+=x[i]*x[i]+y[i]*y[i];
  // Distributed amplitude-dependent dissipation caps the internal energy.
  double damping=1/(1+std::max(0.,total-2)*.002);
  Frame out;for(int i=0;i<count;++i){
   x[i]*=damping;y[i]*=damping;
   double read=(model==9?x[i]:x[i]+.35*y[(i+1)%count])*readScale;
   out.l+=read*(i%2?.7:1.3);out.r+=read*(i%2?1.3:.7);
  }
  return out;
 }
 double contactVelocity()const{return x[0]+.7*x[count-1];}
 double stateEnergy()const{double e=0;for(int i=0;i<count;++i)e+=x[i]*x[i]+y[i]*y[i];return e;}
};
class Voice {
 std::array<Mode,Limits::modes> modes{};
 std::array<double,Limits::modes> contact{},read{},ratio{};
 std::array<Mode,4> grains{};
 FiniteParticles finite;
 FieldBank fieldBank;
 DynamicField dynamic;
 PlasmaProcess plasma;
 Interaction routes;
 Random evolutionRng;std::array<double,4> journey{},momentum{},destination{},hold{};
 double matterTravel=0,fieldTravel=0,particleTravel=0,plasmaTravel=0,contactForce=0,contactSpeed=0,contactHardness=.5,eventEnergy=1,eventSpread=.4;
 Random rng;
 double sr=48000,freq=440,env=0,fieldEnv=0,fieldTone=0,pressure=0,tone=0,depth=0,walk=0,walkVelocity=0,walkRate=.12,eventScale=1,target=0,next=0;
 double attackPole=0,releasePole=0,fieldAttackPole=0,fieldReleasePole=0,rate=0,decay=1,pitch=440,particleEnergy=0;
 double lastMatter=0,lastVelocity=0,lastDisplacement=0,step=.02,admittance=.02,storedEnergy=0;
 double strikeAge=0,strikeDuration=.002,scrapeAge=0,scrapeForce=0,forceState=0;
 double exciterMix=1,modelGain=1,fieldGain=1,plasmaGain=1;
 int currentExciter=0,oldExciter=0,currentMatter=-1,currentField=-1,currentPlasma=-1;
 uint32_t counter=0; // Control-rate scheduling must wrap safely in long-held notes.
 Frame lastFrame{},previousFrame{};
 uint64_t releaseFrames=0;bool initialised=false,ringActive=false;
 std::array<double,8> initialPhases{};
public:
 bool active=false,keyDown=false,sustained=false;int note=60,channel=0;double velocity=.7;
 double level()const{return std::max(env,fieldEnv);}
 double frequency()const{return freq;}
 Frame continuation()const{return lastFrame;}Frame previous()const{return previousFrame;}
 double internalEnergy()const{double e=storedEnergy+dynamic.stateEnergy()+plasma.stateEnergy();for(auto& g:grains)e+=g.x*g.x+g.y*g.y;return e;}
 void start(int n,int ch,double v,double rate_,uint32_t seed){
  *this=Voice{};active=keyDown=true;note=n;channel=ch;velocity=v;sr=rate_;freq=440*std::exp2((n-69)/12.);rng.state=seed?seed:1;plasma.rng.state=seed^0x7654321u;evolutionRng.state=seed^0x9137abdu;
  for(auto& phase:initialPhases)phase=rng.unit();
  for(auto& mode:modes)mode.pan=rng.bi()*.75;
  for(auto& grain:grains){grain.pan=rng.bi();grain.tune(freq,sr,.2);}
 }
 void release(){keyDown=sustained=false;}
 void initialiseExcitation(int exciter,const Settings& p){
  strikeAge=0;
  if(exciter==1)for(int i=0;i<Limits::modes;++i)modes[i].y+=.7*p.force*pressure*contact[i]/std::sqrt(std::max(1.,ratio[i]));
  // Tiny displacement seeds active exciters; it is not a sustaining source.
  if(p.force>0&&(exciter==2||exciter==3||exciter==5))modes[0].y+=1e-5;
 }
 void configure(const Settings& p,const Channel& ch,double shared){
  depth=std::sqrt(clip(p.evolve+.35*ch.wheel));
  walkRate=.025*std::exp2(8*p.motion);
  // Independent momentum paths share a small common pressure history, not an LFO.
  auto shape=[](double q){return q*2/(.4+std::abs(q)*2);};
  matterTravel=shape(journey[0]+.12*shared);fieldTravel=shape(journey[1]-.10*journey[0]+.12*shared);
  particleTravel=shape(journey[2]+.12*journey[3]-.08*shared);plasmaTravel=shape(journey[3]-.12*journey[1]+.10*shared);
  walk=matterTravel;
  Settings evolving=p;
  evolving.structure=clip(p.structure+depth*(.65*matterTravel+.12*fieldTravel));
  evolving.fieldStructure=clip(p.fieldStructure+depth*(.55*fieldTravel-.15*plasmaTravel));
  evolving.fieldShape=clip(p.fieldShape+depth*(.75*fieldTravel-.20*particleTravel));
  evolving.fieldBrightness=clip(p.fieldBrightness+depth*(.75*fieldTravel+.20*matterTravel));
  evolving.fieldModulation=clip(p.fieldModulation+depth*(.80*particleTravel-.25*fieldTravel));
  evolving.fieldSpread=clip(p.fieldSpread+depth*(.75*plasmaTravel-.25*fieldTravel));
  evolving.plasmaFlow=clip(p.plasmaFlow+depth*(.70*plasmaTravel-.20*fieldTravel));
  evolving.plasmaPressure=clip(p.plasmaPressure+depth*(.50*plasmaTravel+.20*particleTravel));
  evolving.plasmaTurbulence=clip(p.plasmaTurbulence+depth*(.85*particleTravel-.35*plasmaTravel));
  evolving.plasmaScale=clip(p.plasmaScale+depth*(.65*plasmaTravel+.15*matterTravel));
  evolving.plasmaRate=clip(p.plasmaRate+depth*(.60*particleTravel+.15*plasmaTravel));
  evolving.plasmaCoherence=clip(p.plasmaCoherence+depth*(.65*fieldTravel-.20*plasmaTravel));
  evolving.plasmaIntermittency=clip(p.plasmaIntermittency+depth*(.65*plasmaTravel-.30*particleTravel));
  evolving.plasmaResonance=clip(p.plasmaResonance+depth*(.55*matterTravel-.25*plasmaTravel));
  eventScale=std::exp2(depth*(4*particleTravel+1.2*plasmaTravel));
  eventEnergy=std::exp2(depth*(.9*particleTravel-.4*fieldTravel));eventSpread=clip(p.particleSpread+depth*(.55*plasmaTravel-.3*particleTravel));
  contactForce=p.force*clip(1+depth*(.85*matterTravel-.30*plasmaTravel),.12,1.9);
  contactSpeed=clip(p.speed+.25*ch.wheel+depth*(.65*matterTravel+.25*particleTravel));
  contactHardness=clip(p.hardness+depth*(.6*particleTravel-.35*matterTravel));
  routes=Interaction{};
  auto exchange=[this](double q){return clip(1+depth*q,.35,1.85);};
  routes.fieldMatter*=exchange(.9*fieldTravel-.5*matterTravel);routes.matterField*=exchange(.9*matterTravel-.5*fieldTravel);
  routes.particlesMatter*=exchange(.8*particleTravel+.3*matterTravel);routes.particlesField*=exchange(.8*particleTravel-.3*fieldTravel);
  routes.plasmaMatter*=exchange(.8*plasmaTravel-.3*matterTravel);routes.plasmaField*=exchange(.8*plasmaTravel+.3*fieldTravel);routes.spaceMatter*=exchange(.5*matterTravel-.3*plasmaTravel);
  // Chromatic tuning is anchored; transformation lives in structure and energy.
  freq+=.18*(440*std::exp2((note-69+2*ch.bend)/12.)-freq);
  tone=clip(p.tone+depth*(.60*matterTravel-.25*particleTravel)+routes.particlesField*p.coupling*p.particles*particleEnergy);
  fieldTone=clip(p.fieldTone+depth*(.70*fieldTravel-.25*plasmaTravel)+routes.particlesField*p.coupling*p.particles*particleEnergy);
  pressure=(.25+.75*velocity)*(.25+.75*p.energy)*(1+.65*ch.pressure);
  attackPole=std::exp(-6.907755/(sr*std::max(.01,p.attack)));releasePole=std::exp(-6.907755/(sr*std::max(.1,p.release)));
  fieldAttackPole=std::exp(-6.907755/(sr*std::max(.01,p.fieldAttack)));fieldReleasePole=std::exp(-6.907755/(sr*std::max(.1,p.fieldRelease)));
  rate=(2+90*p.density*p.density)*std::exp2(8*(p.particleRate-.5))*(.3+.7*pressure);
  decay=std::exp2(6*(p.particleDecay-.5)+depth*(1.4*plasmaTravel+.8*particleTravel));pitch=freq*std::exp2(6*(p.particlePitch-.5)+depth*(.30*particleTravel-.15*plasmaTravel));
  auto profile=makeMatterProfile(p.matterModel,evolving.structure,tone,velocity,p.matter);
  static const double original[]={1,2.09,3.91,5.63,7.1,9.72,12.6,15.4};
  step=std::min(.12,2*pi*freq/sr);admittance=0;
  bool freshMatter=currentMatter<0;currentMatter=p.matterModel;
  for(int i=0;i<Limits::modes;++i){
   double n=i+1.,desiredRatio=p.matterModel==0?n+(original[i]-n)*evolving.structure:profile[i].ratio;
   ratio[i]=freshMatter?desiredRatio:ratio[i]+.04*(desiredRatio-ratio[i]);
   double f=freq*ratio[i];
   double seconds=p.matterModel==0?(.6+4*p.matter+3*evolving.structure)/(1+.1*i):profile[i].seconds;
   // Increased physical loss after release retires even high-Q modes naturally.
   if(!keyDown&&!sustained)seconds=std::min(seconds,std::max(.12,p.release));
   double desiredLoss=std::exp(-6.907755/(sr*clip(seconds,.08,12)));
   double previousLoss=modes[i].r;modes[i].tune(f,sr,clip(seconds,.08,12));
   if(!freshMatter)modes[i].r=previousLoss+.04*(desiredLoss-previousLoss);
   double pos=clip(p.position+depth*(.30*matterTravel+.12*particleTravel),.03,.97);
   double desiredContact=std::sin(pi*n*pos)/std::sqrt(n)*(1/(1+.025*(1-contactHardness)*ratio[i]*ratio[i]))*clip((sr*.39-f)/(sr*.10));
   contact[i]=freshMatter?desiredContact:contact[i]+.04*(desiredContact-contact[i]);admittance+=contact[i]*contact[i]*step;
   double redistribution=std::exp2(depth*(1.8*matterTravel*(i/3.5-1)+.8*fieldTravel*(i%2?1:-1)));
   double desiredRead=(p.matterModel==0?1.:profile[i].read)*redistribution/(1+.12*i);
   read[i]=freshMatter?desiredRead:read[i]+.04*(desiredRead-read[i]);
  }
  if(depth>0){
   double t=.12*depth*walkRate*(sr>=64000?128.:64.)/sr*matterTravel;
   double c=(1-t*t)/(1+t*t),q=2*t/(1+t*t);
   for(int i=0;i+1<Limits::modes;i+=2){auto& a=modes[i];auto& b=modes[i+1];double ax=a.x,ay=a.y;a.x=c*ax-q*b.x;a.y=c*ay-q*b.y;b.x=q*ax+c*b.x;b.y=q*ay+c*b.y;}
  }
  strikeDuration=.0004+.012*(1-p.hardness)*(1-p.hardness);
  if(!initialised){currentExciter=oldExciter=p.exciter;initialiseExcitation(p.exciter,p);initialised=true;}
  if(currentExciter!=p.exciter){oldExciter=currentExciter;currentExciter=p.exciter;exciterMix=0;initialiseExcitation(p.exciter,p);}
  int fmodel=std::clamp(p.fieldModel,0,11);
  if(currentField<0){currentField=fmodel;fieldGain=0;fieldBank.initialise(std::min(fmodel,8),initialPhases);}
  fmodel=currentField;
  if(fmodel<9){
   fieldBank.model=fmodel;
   fieldBank.configure(evolving,freq,sr,fieldTone,depth,shared);
   if(fmodel==0){ // Original spectral organisation, now on shared partial renderer.
    double total=0;for(int i=0;i<fieldBank.partialCount();++i){
     double n=i+1.,r=n+clip(evolving.fieldStructure+evolving.fieldShape-.5)*.016*(n*n-n);
     fieldBank.increment[i]=freq*r/sr;fieldBank.weight[i]=clip((.4-fieldBank.increment[i])/.1)/std::pow(n,2.2-1.5*clip(fieldTone+evolving.fieldBrightness-.5));total+=fieldBank.weight[i];
    }
    for(int i=0;i<fieldBank.partialCount();++i)fieldBank.weight[i]/=std::max(1.,total);
   }
  }else dynamic.configure(fmodel,evolving,freq,sr,shared);
  plasma.configure(evolving,sr);
  if(currentPlasma<0){currentPlasma=p.plasmaModel;plasmaGain=0;}
 }
 double excitation(int kind,const Settings& p,const Channel& ch,bool held,double shared){
  if(!held||p.force<=1e-8)return 0;
  double force=contactForce*pressure,velocityContact=lastVelocity;
  double speed=contactSpeed;
  switch(kind){
   case 0:{
    if(strikeAge>=strikeDuration)return 0;
    double t=strikeAge/strikeDuration;
    return 1.0*force*(1-std::cos(2*pi*t))/(sr*strikeDuration*std::max(step,1e-5));
   }
   case 1:return 0;
   case 2:{
    // Friction load-line solve. A sticking solution is used when feasible.
    double wheel=.025+.15*speed,normal=1.35*force,vs=.03+.16*contactHardness;
    double required=(wheel-velocityContact)/std::max(admittance,1e-6);
    if(std::abs(required)<normal*.9)return required;
    auto law=[&](double f){double relative=wheel-velocityContact-admittance*f;
     double u=relative/vs;double mu=.12+.78/(1+u*u);
     return normal*mu*soft(relative/(.001+.01*(1-contactHardness)));};
    double lo=-normal,hi=normal;
    for(int i=0;i<8;++i){double mid=(lo+hi)*.5;if(mid-law(mid)>0)hi=mid;else lo=mid;}
    return (lo+hi)*.5;
   }
   case 3:{
    // Reduced flow-valve negative resistance. Resonator velocity changes the
    // instantaneous conductance; cubic drag closes the energy injection.
    double u=velocityContact/(.025+.15*(1-contactHardness)*(.4+1.4*force));
    double gain=(.35*force)*(.35+.9*speed);
    return gain*(u/(1+u*u)-.045*u)+rng.bi()*.00005*force;
   }
   case 4:{
    double eventRate=30+1200*speed*speed;
    if(rng.unit()<eventRate/sr){scrapeForce+=rng.bi()*force*(.3+.7*contactHardness);scrapeAge=0;}
    double compliance=.0003+.003*(1-contactHardness);
    scrapeForce-=scrapeForce/(1+sr*compliance);
    return scrapeForce*(1+.3*soft(lastDisplacement*4));
   }
   default:{
    double u=velocityContact*8,q=lastDisplacement*20;
    return force*(.5*soft(u*(1.2-std::abs(q)))+.4*soft(q*u)-.15*soft(u*u*u))*(.4+speed);
   }
  }
 }
 Frame tick(const Settings& p,const Channel& ch,double shared,double pulse,double room,const Sine& sine){
  if(!active)return {};
  if(depth>0){
   static constexpr double speeds[]={.65,1.,1.45,.78};
   for(int i=0;i<4;++i){double rate=walkRate*speeds[i];if(hold[i]--<=0){destination[i]=evolutionRng.bi();hold[i]=sr*(.55+1.0*evolutionRng.unit())/rate;}
    momentum[i]+=(6*rate*(destination[i]-journey[i])-4*momentum[i])*rate/sr;
    journey[i]=clip(journey[i]+momentum[i]/sr,-1.,1.);
   }
  }
  if((counter++&(sr>=64000?127:63))==0)configure(p,ch,shared);
  bool held=keyDown||sustained;
  env=(held?1.:0.)+((held?attackPole:releasePole)*(env-(held?1.:0.)));
  fieldEnv=(held?1.:0.)+((held?fieldAttackPole:fieldReleasePole)*(fieldEnv-(held?1.:0.)));
  if(!held)++releaseFrames;
  modelGain=std::min(1.,modelGain+1/(sr*.04));
  if(currentField!=p.fieldModel){
   fieldGain=std::max(0.,fieldGain-1/(sr*.025));
   if(fieldGain<=0){currentField=p.fieldModel;fieldBank.initialise(std::min(currentField,8),initialPhases);configure(p,ch,shared);}
  }else fieldGain=std::min(1.,fieldGain+1/(sr*.04));
  if(currentPlasma!=p.plasmaModel){plasmaGain=std::max(0.,plasmaGain-1/(sr*.025));if(plasmaGain<=0)currentPlasma=p.plasmaModel;}
  else plasmaGain=std::min(1.,plasmaGain+1/(sr*.04));
  exciterMix=std::min(1.,exciterMix+1/(sr*.035));
  Frame particle;
  if(held&&p.particles>1e-8&&rng.unit()<std::max(0.,rate*eventScale*pulse)/sr){
   double amplitude=(.1+.3*rng.unit())*pressure*eventEnergy;
   if(p.particleModel==0){
    ringActive=true;auto& g=grains[int(rng.unit()*4)];
    double r=3+(1+std::floor(rng.unit()*5)-3)*eventSpread*2;
    g.tune(pitch*r,sr,clip((.15+1.2*p.particles+.6*rng.unit())*decay,.005,12));g.x+=amplitude;
   }else finite.trigger(p.particleModel,pitch,decay,eventSpread,amplitude,sr,uint32_t(rng.unit()*4294967295.)|1);
   particleEnergy=std::min(1.,particleEnergy+.4);
  }
  if(ringActive){
   double energy=0;
   for(auto& g:grains){double excess=g.x*g.x+g.y*g.y-1;if(excess>0){double loss=1/(1+step*.08*excess);g.x*=loss;g.y*=loss;}
    double a=g.tick(0);particle.l+=a*(1-.5*g.pan);particle.r+=a*(1+.5*g.pan);energy+=g.x*g.x+g.y*g.y;}
   if(energy<1e-20)ringActive=false;
  }
  auto events=finite.tick(sr,sine);particle.l+=events.l;particle.r+=events.r;particleEnergy*=.9997;
  double turbulent=0;
  if(p.plasma>1e-8&&(p.plasmaSelf>1e-8||p.plasmaDrive>1e-8))turbulent=plasma.tick(currentPlasma,sr,shared)*plasmaGain;
  double disturbance=p.coupling*(routes.matterField*soft(lastMatter*3)*p.matter+routes.particlesField*particleEnergy*p.particles+
                                 routes.plasmaField*turbulent*p.plasma*p.plasmaDrive);
  if(currentField>=9)disturbance=p.coupling*(-routes.fieldMatter*lastVelocity*p.field*p.matter+
                     routes.particlesField*particleEnergy*p.particles+routes.plasmaField*turbulent*p.plasma*p.plasmaDrive);
  Frame field;
  if(p.field>1e-8){
   field=currentField<9?fieldBank.tick(p,disturbance,depth,shared,sine):dynamic.tick(currentField,disturbance,depth*shared,sine,held);
   field.l*=fieldGain;field.r*=fieldGain;
  }
  double own=excitation(currentExciter,p,ch,held,shared);
  if(exciterMix<1)own=own*exciterMix+excitation(oldExciter,p,ch,held,shared)*(1-exciterMix);
  strikeAge+=1/sr;
  // A single force port receives event waveform, independent of audible SELF.
  // No additional generic arrival strike: the model's event spectrum is the force.
  double external=p.coupling*(routes.fieldMatter*(currentField>=9?dynamic.contactVelocity()*p.matter:(field.l+field.r)*.5)*p.field+
                 routes.particlesMatter*(particle.l+particle.r)*.5*p.particles*p.particleImpact+
                 routes.plasmaMatter*turbulent*p.plasma*p.plasmaDrive+
                 routes.spaceMatter*room*p.space);
  double applied=held?own*env*pulse+external*env:0;
  double ml=0,mr=0,contactVelocity=0,contactDisplacement=0,total=0;
  double nonlinearLoss=storedEnergy>1.5?1/(1+step*.4*(storedEnergy-1.5)):1;
  for(int i=0;i<Limits::modes;++i){
   auto& m=modes[i];double a=m.tick(step*contact[i]*applied);
   m.x*=nonlinearLoss;m.y*=nonlinearLoss;
   contactVelocity+=contact[i]*m.x;contactDisplacement+=contact[i]*m.y/std::max(.4,ratio[i]);
   double value=a*read[i]*modelGain;
   ml+=value*(1-.5*m.pan);mr+=value*(1+.5*m.pan);total+=m.x*m.x+m.y*m.y;
  }
  storedEnergy=total;lastVelocity=contactVelocity;lastDisplacement=contactDisplacement;lastMatter=(ml+mr)*.5;
  double fade=held?1:clip((sr*(p.release*3+4)-double(releaseFrames))/(sr*.2));
  ml*=fade;mr*=fade;
  double e=env*pressure;
  Frame out{ml*.18*p.matter+field.l*.10*fieldEnv*pressure*pulse*p.field+particle.l*.16*e*p.particles*p.particleSelf+turbulent*.12*e*p.plasma*p.plasmaSelf,
            mr*.18*p.matter+field.r*.10*fieldEnv*pressure*pulse*p.field+particle.r*.16*e*p.particles*p.particleSelf+turbulent*.12*e*p.plasma*p.plasmaSelf,
            ((particle.l+particle.r)*.5*p.particles*p.particleSelf+turbulent*p.plasma*p.plasmaSelf)*.12*e};
  double lifetimeRelease=std::max(p.release,p.field>1e-8?p.fieldRelease:0.);
  if(!held&&((env<1e-7&&total<1e-10&&(p.field<=1e-8||fieldEnv<1e-7))||releaseFrames>sr*(lifetimeRelease*3+4)))active=false;
  out.matterL=ml*.18*p.matter;out.matterR=mr*.18*p.matter;
  previousFrame=lastFrame;lastFrame=out;return out;
 }
};
