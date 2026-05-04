#include "overdrive.h"
#include <cmath>
#include <algorithm>


Overdrive::Overdrive() : drive_(1.0), mix_(1.0), outputGain_(1.0) {}

bool Overdrive::init() {
    currentBypass_ = targetBypass_;
    return true;
}

void Overdrive::cleanup() {
}

void Overdrive::setDrive(double drive) {
    drive_ = std::max(1.0, drive);
}

void Overdrive::setMix(double mix) {
    mix_ = std::clamp(mix, 0.0, 1.0);
}

void Overdrive::setOutputGain(double gain) {
    outputGain_ = std::max(0.0, gain);
}

double Overdrive::process(double input) {
    currentBypass_ += (targetBypass_ - currentBypass_) * bypassSmoothingFactor_;

    double wet = fast_tanh(input * drive_);
    double saturated = wet * mix_ + input * (1.0 - mix_);
    double output = saturated * outputGain_;
    return output + currentBypass_ * (input - output);
}

void Overdrive::processBuffer(double* input, double* output, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        output[i] = process(input[i]);
    }
}