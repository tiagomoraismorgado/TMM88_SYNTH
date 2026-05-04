#include "sub_oscillator.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

SubOscillator::SubOscillator() 
    : phase_(0.0), sampleRate_(44100.0), octave_(1), frequency_(440.0) {}

bool SubOscillator::init() {
    phase_ = 0.0;
    currentBypass_ = targetBypass_;
    return true;
}

void SubOscillator::cleanup() {}

void SubOscillator::setSampleRate(double rate) {
    sampleRate_ = rate;
}

void SubOscillator::setOctave(int octave) {
    // Minilogue XD allows 1 or 2 octaves below
    octave_ = std::clamp(octave, 1, 2);
}

double poly_blep(double t, double dt) {
    if (t < dt) {
        t /= dt;
        return t + t - t * t - 1.0;
    } else if (t > 1.0 - dt) {
        t = (t - 1.0) / dt;
        return t * t + t + t + 1.0;
    }
    return 0.0;
}

double SubOscillator::process(double frequency) {
    currentBypass_ += (targetBypass_ - currentBypass_) * bypassSmoothingFactor_;

    // Calculate frequency for the sub (f / 2^octave)
    double subFreq = frequency / std::pow(2.0, octave_);
    
    // Generate Band-limited Square Wave using PolyBLEP
    double t = phase_ / (2.0 * M_PI);
    double dt = subFreq / sampleRate_;
    double output = (t < 0.5) ? 1.0 : -1.0;
    output += poly_blep(t, dt);
    output -= poly_blep(std::fmod(t + 0.5, 1.0), dt);

    double phaseInc = (2.0 * M_PI * subFreq) / sampleRate_;
    phase_ += phaseInc;
    if (phase_ >= 2.0 * M_PI) phase_ -= 2.0 * M_PI;

    return output * (1.0 - currentBypass_);
}

void SubOscillator::processBuffer(double* input, double* output, size_t size) {
    // SubOscillator is a generator, ignore input
    for (size_t i = 0; i < size; ++i) {
        output[i] = process(frequency_);
    }
}