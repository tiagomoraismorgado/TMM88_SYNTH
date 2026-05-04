#include "vca.h"
#include <algorithm>
#include <cmath>

VCA::VCA() 
    : targetGain_(1.0), currentGain_(1.0), 
      targetMorph_(0.0), currentMorph_(0.0),
      targetDryWet_(1.0), currentDryWet_(1.0),
      smoothingFactor_(0.001) {}

bool VCA::init() {
    // Reset current values to targets to prevent gliding on startup
    currentGain_ = targetGain_;
    currentMorph_ = targetMorph_;
    currentDryWet_ = targetDryWet_;
    currentBypass_ = targetBypass_;
    return true;
}

void VCA::cleanup() {}

void VCA::setGain(double gain) {
    targetGain_ = gain;
}

double VCA::getGain() const {
    return targetGain_;
}

void VCA::setMorph(double morph) {
    targetMorph_ = std::clamp(morph, 0.0, 1.0);
}

double VCA::getMorph() const {
    return targetMorph_;
}

void VCA::setDryWet(double dryWet) {
    targetDryWet_ = std::clamp(dryWet, 0.0, 1.0);
}

double VCA::getDryWet() const {
    return targetDryWet_;
}

void VCA::setSmoothingTime(double timeMs, double sampleRate) {
    if (timeMs <= 0.0) {
        smoothingFactor_ = 1.0; // Instant transition
    } else {
        // Calculate coefficient for exponential smoothing
        // factor = 1 - exp(-1 / (sampleRate * timeInSeconds))
        smoothingFactor_ = 1.0 - std::exp(-1.0 / (sampleRate * (timeMs / 1000.0)));
    }
}

double VCA::process(double input) {
    currentBypass_ += (targetBypass_ - currentBypass_) * bypassSmoothingFactor_;

    // Smoothly interpolate current gain towards target
    currentGain_ += (targetGain_ - currentGain_) * smoothingFactor_;
    
    double processed = input * currentGain_;
    // Crossfade with dry input
    return processed + currentBypass_ * (input - processed);
}

double VCA::process(double inputA, double inputB) {
    currentBypass_ += (targetBypass_ - currentBypass_) * bypassSmoothingFactor_;

    // Smoothly interpolate both parameters
    currentGain_ += (targetGain_ - currentGain_) * smoothingFactor_;
    currentMorph_ += (targetMorph_ - currentMorph_) * smoothingFactor_;
    currentDryWet_ += (targetDryWet_ - currentDryWet_) * smoothingFactor_;
    
    // Linear crossfade: inputA * (1 - dryWet) + inputB * dryWet
    double mixed = inputA + currentDryWet_ * (inputB - inputA);
    double result = mixed * currentGain_;

    // Crossfade with Dry signal (inputA)
    return result + currentBypass_ * (inputA - result);
}