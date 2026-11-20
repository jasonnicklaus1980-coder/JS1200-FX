#pragma once
#include "md12/internal.h"
namespace js1200 {
// Eight independent output circuits; calibration and resonance are provisional.
class AnalogOutput {
public:
    AnalogOutput();
    void prepare(double sampleRate);
    void reset();
    void trigger(int channel);
    void setDynamic(int channel, float cutoffHz, float resonance);
    float process(float input, int channel);
private:
    md12_dyn dynamic_[2] = {}, initialDynamic_[2] = {};
    md12_iir fixed_[4] = {}, initialFixed_[4] = {};
    float resonance_[2] = {}, feedback_[2] = {}, cutoff_[2] = {};
    int gate_[2] = {};
    int gateLength_ = 1;
    double sampleRate_ = 44100.0;
};
}
