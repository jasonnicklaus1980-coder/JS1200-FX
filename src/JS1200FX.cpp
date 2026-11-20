#include "mpc_plugin.h"
#include "engine/analog/AnalogOutput.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace {
enum { kMix, kRate, kBits, kPath, kCutoff, kResonance, kDrive, kOutput, kParams };
float gain(float db) { return std::pow(10.0f,db/20.0f); }
}
class JS1200FX final : public mpc::Plugin {
public:
 JS1200FX():mpc::Plugin(infoForPlugin()) {
  addParam(mpc::floatParam("mix","Mix",0,100,100,"%",0));
  addParam(mpc::logParam("rate","Rate",2000,44100,26041.667f,"Hz",0));
  addParam(mpc::intParam("bits","Bits",4,12,12,"bit"));
  addParam(mpc::choiceParam("path","Path",{"Dyn 1","Dyn 2","Fixed 3","Fixed 4","Fixed 5","Fixed 6","Raw 7","Raw 8"},0));
  addParam(mpc::logParam("cutoff","Cutoff",40,8000,1000,"Hz",0));
  addParam(mpc::floatParam("res","Resonance",0,1,0,"",2));
  addParam(mpc::floatParam("drive","Drive",0,18,0,"dB",1));
  addParam(mpc::floatParam("output","Output",-24,0,-6,"dB",1));
  addFactoryPrograms();
 }
 void prepare(double rate,int) override { rate_=rate>1000?rate:44100; for(auto& f:filters_)f.prepare(rate_);reset(); }
 void reset() override { for(auto& f:filters_)f.reset(); held_[0]=held_[1]=0; phase_=1; level_=0; gate_=false; mix_=get(kMix)/100.f; out_=gain(get(kOutput)); }
 void process(mpc::ProcessContext& c) override {
  const int ch=mpc::clamp(c.numOutputs,0,2), path=mpc::clamp(getInt(kPath),0,7), bits=mpc::clamp(getInt(kBits),4,12);
  const float rate=std::min((float)rate_,get(kRate)), step=rate/(float)rate_, levels=(float)(1<<(bits-1));
  const float mixTarget=get(kMix)*.01f, outputTarget=gain(get(kOutput)), drive=gain(get(kDrive));
  const float cutoff=get(kCutoff), res=get(kResonance);
  for(int k=0;k<ch;k++){filters_[k].setDynamic(0,cutoff,res);filters_[k].setDynamic(1,cutoff,res);}
  for(int i=0;i<c.numFrames;i++){
   float input[2]={0,0}; for(int k=0;k<ch;k++)input[k]=c.inputs && k<c.numInputs && c.inputs[k]?c.inputs[k][i]:0;
   float detector=std::max(std::fabs(input[0]),std::fabs(input[1])); level_+=0.01f*(detector-level_);
   if(!gate_ && level_>.12f){for(int k=0;k<ch;k++)filters_[k].trigger(path);gate_=true;} else if(gate_ && level_<.06f)gate_=false;
   phase_+=step; if(phase_>=1.f){phase_-=std::floor(phase_);for(int k=0;k<ch;k++){float v=std::clamp(input[k]*drive,-1.f,1.f);held_[k]=(std::clamp(std::floor((v+1.f)*levels),0.f,2.f*levels-1.f)-levels)/levels;}}
   mix_+=.002f*(mixTarget-mix_);out_+=.002f*(outputTarget-out_);
   for(int k=0;k<ch;k++){float wet=filters_[k].process(held_[k],path);float y=(input[k]*(1.f-mix_)+wet*mix_)*out_;c.outputs[k][i]=std::clamp(y,-1.f,1.f);}
  }
 }
private:
 static mpc::PluginInfo infoForPlugin(){mpc::PluginInfo i;i.name="JS1200 FX";i.vendor="Shannon Instruments";i.uniqueId=MPC_FOURCC('S','X','F','1');i.version=1000;i.isInstrument=false;i.numInputs=2;i.numOutputs=2;return i;}
 void addFactoryPrograms();
 js1200::AnalogOutput filters_[2];double rate_=44100.;float held_[2]={},phase_=1,level_=0,mix_=1,out_=.5f;bool gate_=false;
};

void JS1200FX::addFactoryPrograms() {
 // Values: mix %, sample rate Hz, bits, output channel, cutoff Hz,
 // resonance 0..1, input drive dB, output dB. Eight named groups in order.
 struct Preset { const char* name; float mix,rate,bits,path,cutoff,res,drive,out; };
 static const Preset p[] = {
 {"SP Classic",100,26041,12,0,1000,.00,0,-6},
 {"Original Crunch",100,26041,12,2,1000,.00,4,-8},
 {"Raw 12-Bit",100,26041,12,6,1000,.00,0,-6},
 {"26K Character",85,26041,12,4,1400,.00,2,-6},
 {"Vintage Converter",90,24000,11,3,1200,.05,3,-7},
 {"Warm Output",75,26041,12,0,850,.08,2,-6},
 {"Dirty DAC",100,19000,10,6,1000,.00,8,-10},
 {"Full Character",100,22000,11,1,1500,.15,6,-10},
 {"Boom Bap 90",90,22000,11,2,1200,.00,5,-9},
 {"Dusty Kicks",85,19000,11,0,650,.08,5,-9},
 {"Snare Knock",85,26041,12,4,2300,.00,6,-9},
 {"Dirty Breaks",100,17000,10,3,1500,.00,8,-10},
 {"Drum Bus Crunch",65,24000,11,2,1300,.00,7,-9},
 {"Old School Punch",80,23000,12,5,1800,.00,4,-8},
 {"Hard Hats",90,18000,10,7,1500,.00,4,-10},
 {"Underground Drums",95,14500,10,4,1200,.00,7,-10},
 {"Velvet Soul",50,26041,12,0,800,.04,1,-6},
 {"Warm Rhodes",60,23000,12,1,1400,.05,2,-7},
 {"Neo Soul Dust",55,19000,11,3,1600,.00,2,-7},
 {"Soulful Keys",65,26041,12,4,1700,.00,1,-6},
 {"Satin Strings",45,30000,12,1,2200,.03,0,-6},
 {"90s R&B",60,22000,11,5,1400,.00,3,-7},
 {"Smooth Sample",55,26041,12,2,1200,.00,1,-6},
 {"Midnight Groove",70,18000,11,0,900,.12,3,-8},
 {"SSM Warmth",85,26041,12,0,900,.06,2,-7},
 {"Four-Pole Silk",100,32000,12,0,2400,.02,0,-6},
 {"Deep Lowpass",100,30000,12,1,350,.04,0,-7},
 {"Resonant Sweep",100,26041,12,0,1600,.65,0,-10},
 {"Dark Ladder",95,22000,12,1,500,.20,2,-8},
 {"Filtered Drums",90,24000,12,0,2000,.18,4,-9},
 {"Vintage Sweep",95,26041,11,1,3000,.35,2,-9},
 {"SSM Drive",100,26041,12,0,1200,.25,9,-12},
 {"Aliasing Machine",100,6500,8,6,1000,.00,3,-9},
 {"Clock Down",100,9000,12,7,1000,.00,0,-8},
 {"Bit Damage",100,22000,5,6,1000,.00,4,-10},
 {"Digital Dust",75,12000,8,4,1300,.00,3,-8},
 {"Crushed Transients",90,15000,6,7,1000,.00,8,-12},
 {"Broken Converter",100,7800,7,6,1000,.00,6,-12},
 {"Extreme 12-Bit",100,4200,12,7,1000,.00,8,-12},
 {"Lo-Fi Chaos",100,3000,4,5,1600,.00,12,-15},
 {"Dirty Bass",90,17000,9,0,750,.08,6,-10},
 {"Sub Warmth",55,26041,12,1,400,.05,2,-7},
 {"Low-End Weight",60,24000,12,2,650,.00,3,-8},
 {"Vintage 808",75,20000,11,0,500,.08,5,-9},
 {"Bass Crunch",80,14000,8,3,900,.00,8,-12},
 {"Deep Analog",85,32000,12,1,300,.10,1,-7},
 {"Filtered Sub",100,26041,12,0,250,.03,0,-8},
 {"Heavy Lowpass",100,18000,10,1,550,.25,6,-11},
 {"Tape-Like Warmth",50,30000,12,3,1500,.00,3,-7},
 {"Old Sampler",90,22000,11,2,1000,.00,2,-8},
 {"Vintage Vinyl",60,16000,10,4,1200,.00,2,-8},
 {"Soft Converter",40,26041,12,5,1400,.00,1,-6},
 {"Dark Master",45,24000,12,0,750,.04,1,-8},
 {"Retro Studio",60,26041,11,3,1500,.00,3,-8},
 {"Warm Saturation",70,32000,12,1,1600,.06,6,-10},
 {"Analog Dust",65,18000,10,0,1100,.08,3,-8},
 {"Subtle 12-Bit",20,26041,12,6,1000,.00,0,-6},
 {"Drum Glue",35,26041,12,2,1500,.00,3,-7},
 {"Parallel Crunch",30,14000,8,6,1000,.00,6,-9},
 {"Gentle Color",25,30000,12,4,1600,.00,1,-6},
 {"Sample Bus",40,22000,11,3,1300,.00,2,-7},
 {"Warm Mix",30,26041,12,0,1800,.03,1,-7},
 {"Transient Edge",25,20000,10,7,1000,.00,4,-8},
 {"Clean Vintage",15,26041,12,5,1200,.00,0,-6}
 };
 static_assert(sizeof(p)/sizeof(p[0])==64,"Expected 64 factory presets");
 for(const auto& v:p) addProgram(v.name,{v.mix,v.rate,v.bits,v.path,v.cutoff,v.res,v.drive,v.out});
}
MPC_PLUGIN_ENTRY(JS1200FX)
