#include "AnalogOutput.h"
#include <algorithm>
#include <cmath>
namespace js1200 {
AnalogOutput::AnalogOutput() { prepare(44100.0); }
void AnalogOutput::prepare(double rate) {
    sampleRate_ = rate > 1000.0 ? rate : 44100.0;
    md12_calib c{}; md12_calib_sp1200(&c);
    // Use the drawn SP-1200 network, not the measured SP-12 alternative.
    c.dyn_pin13_ohms = MD12_PIN13_DRAWN;
    const md12_proto* proto[] = {&md12_proto_fixed3,&md12_proto_fixed4,&md12_proto_fixed5,&md12_proto_fixed6};
    for (int i=0;i<4;++i) md12_iir_design(&initialFixed_[i],proto[i],1.0,sampleRate_);
    for (int i=0;i<2;++i) { md12_dyn_init(&initialDynamic_[i],&c,i,sampleRate_); cutoff_[i]=1000.0f; resonance_[i]=0.0f; }
    // Eight 1.2288 ms OS housekeeping ticks. Trigger phase is approximate.
    gateLength_ = std::max(1, static_cast<int>(std::lround(sampleRate_ * (8.0 * 0.0012288))));
    reset();
}
void AnalogOutput::reset() {
    for(int i=0;i<4;++i) fixed_[i]=initialFixed_[i];
    for(int i=0;i<2;++i) { dynamic_[i]=initialDynamic_[i]; gate_[i]=0; feedback_[i]=0.0f; cutoff_[i]=1000.0f; }
}
void AnalogOutput::trigger(int channel) {
    if(channel>=0 && channel<2) { gate_[channel]=gateLength_; dynamic_[channel].gate_low=1; }
}
void AnalogOutput::setDynamic(int channel,float cutoffHz,float resonance) {
    if(channel<0 || channel>=2) return;
    cutoffHz=std::max(40.0f,std::min(8000.0f,cutoffHz));
    resonance_[channel]=std::max(0.0f,std::min(1.0f,resonance));
    if(cutoff_[channel]!=cutoffHz) {
        // Preserve envelope offset while changing the calibrated rest point.
        dynamic_[channel].lr0=static_cast<float>(std::log2(cutoffHz/sampleRate_));
        cutoff_[channel]=cutoffHz;
    }
}
float AnalogOutput::process(float input,int channel) {
    if(channel<0 || channel>=8) return 0.0f;
    if(channel>=6) return input;
    if(channel>=2) return md12_iir_run(&fixed_[channel-2],input);
    auto& d=dynamic_[channel];
    if(gate_[channel]>0 && --gate_[channel]==0) d.gate_low=0;
    // External feedback is a bounded, uncalibrated resonance approximation,
    // not the SSM2044's measured internal feedback transfer function.
    const float driven=input - 2.0f*resonance_[channel]*feedback_[channel];
    const float output=md12_dyn_run(&d,driven);
    feedback_[channel]=std::max(-4.0f,std::min(4.0f,output));
    return output;
}
}
