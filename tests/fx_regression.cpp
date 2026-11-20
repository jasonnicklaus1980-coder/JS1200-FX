#include "../src/JS1200FX.cpp"
#include <array>
#include <set>
#include <vector>
#include <cstdint>
#include <cstdio>
#include <cmath>
#include <cstring>
int main(){
 JS1200FX p; p.prepare(44100,128); if(p.numPrograms()!=64||p.numParams()!=8)return 1;
 std::set<std::array<float,8>> settings; std::set<uint64_t> hashes;
 for(int n=0;n<64;n++){
  p.setProgram(n); std::array<float,8> a; for(int j=0;j<8;j++)a[j]=p.get(j); settings.insert(a);
  auto state=p.internalSaveState(); p.set(0,0); if(!p.internalLoadState(state.data(),state.size()))return 2; for(int j=0;j<8;j++)if(std::fabs(p.get(j)-a[j])>0.01f)return 3;
  p.reset(); float in0[128],in1[128],out0[128],out1[128]; const float* ins[]={in0,in1}; float* outs[]={out0,out1}; mpc::ProcessContext c; c.inputs=ins;c.outputs=outs;c.numInputs=2;c.numOutputs=2;c.numFrames=128;c.sampleRate=44100;
  uint64_t hash=1469598103934665603ULL;
  for(int b=0;b<48;b++){for(int i=0;i<128;i++){int t=b*128+i; in0[i]=.25f*std::sin(t*.037f)+.14f*std::sin(t*.97f)+(t%641==0?.5f:0);in1[i]=.21f*std::sin(t*.061f)+.1f*std::sin(t*.51f);} p.process(c);for(int i=0;i<128;i++){if(!std::isfinite(out0[i])||!std::isfinite(out1[i])||std::fabs(out0[i])>1||std::fabs(out1[i])>1)return 4;uint32_t v;std::memcpy(&v,&out0[i],4);hash=(hash^v)*1099511628211ULL;}}
  hashes.insert(hash);
 }
 std::printf("programs=%d unique_parameter_sets=%zu unique_audio_renders=%zu; state, finite and output bounds PASS\n",p.numPrograms(),settings.size(),hashes.size());
 return settings.size()==64&&hashes.size()==64?0:5;
}
