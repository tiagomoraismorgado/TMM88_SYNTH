#include "stereo_panner.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

StereoPanner::StereoPanner() 
    : targetPan_(0.0), currentPan_(0.0), smoothingFactor_(0.001) {}

bool StereoPanner::init() {
    currentPan_ = targetPan_;
    currentBypass_ = targetBypass_;
    return true;
}

void StereoPanner::cleanup() {}

void StereoPanner::setPan(double pan) {
    targetPan_ = std::clamp(pan, -1.0, 1.0);
}

double StereoPanner::getPan() const {
    return targetPan_;
}

void StereoPanner::setSmoothingTime(double timeMs, double sampleRate) {
    if (timeMs <= 0.0) {
        smoothingFactor_ = 1.0; // Instant transition
    } else {
        // Calculate coefficient for exponential smoothing
        smoothingFactor_ = 1.0 - std::exp(-1.0 / (sampleRate * (timeMs / 1000.0)));
    }
}

void StereoPanner::process(double input, double& left, double& right) {
    // Smoothly update bypass state
    currentBypass_ += (targetBypass_ - currentBypass_) * bypassSmoothingFactor_;

    // Smoothly interpolate current pan towards target
    currentPan_ += (targetPan_ - currentPan_) * smoothingFactor_;

    // Constant power panning algorithm
    // Maps pan [-1, 1] to angle [0, PI/2]
    double angle = (currentPan_ + 1.0) * (M_PI / 4.0);
    
    double pannedLeft = input * std::cos(angle);
    double pannedRight = input * std::sin(angle);

    // Crossfade with dry input (mono pass-through)
    left = pannedLeft + currentBypass_ * (input - pannedLeft);
    right = pannedRight + currentBypass_ * (input - pannedRight);
}