#include "lpf.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

ResonantLPF::ResonantLPF() 
    : targetCutoff_(1000.0), currentCutoff_(1000.0), 
      targetResonance_(0.707), currentResonance_(0.707),
      cutoffModDepth_(0.0), keyTrackAmount_(0.0), currentKeyFreq_(0.0), 
      drive_(1.0), satModel_(FilterSaturationModel::Soft), 
      mode_(FilterMode::LPF), invertMod_(false), 
      lpfOutput_(0.0), bpfOutput_(0.0), hpfOutput_(0.0),
      sampleRate_(44100.0), 
      smoothingFactor_(0.001),
      m_satFunc([](double x, double d){ return fast_tanh(x * d); }),
      s1_(0.0), s2_(0.0),
      lastEffectiveCutoff_(-1.0), lastResonance_(-1.0),
      g_coeff_(0.0), k_coeff_(0.0), a1_coeff_(0.0), a2_coeff_(0.0), a3_coeff_(0.0) {}

bool ResonantLPF::init() {
    currentCutoff_ = targetCutoff_;
    currentResonance_ = targetResonance_;
    
    // Force coefficient update on first process
    lastEffectiveCutoff_ = -1.0;
    lastResonance_ = -1.0;

    s1_ = 0.0;
    s2_ = 0.0;
    return true;
}

void ResonantLPF::cleanup() {}

void ResonantLPF::setCutoff(double cutoffHz) {
    targetCutoff_ = std::clamp(cutoffHz, 10.0, sampleRate_ * 0.45);
}

void ResonantLPF::setResonance(double q) {
    targetResonance_ = std::max(0.01, q);
}

void ResonantLPF::setSampleRate(double sampleRate) {
    sampleRate_ = sampleRate;
}

void ResonantLPF::setSmoothingTime(double timeMs, double sampleRate) {
    sampleRate_ = sampleRate;
    if (timeMs <= 0.0) {
        smoothingFactor_ = 1.0;
    } else {
        smoothingFactor_ = 1.0 - std::exp(-1.0 / (sampleRate * (timeMs / 1000.0)));
    }
}

void ResonantLPF::setCutoffModDepth(double depthHz) {
    cutoffModDepth_ = depthHz;
}

void ResonantLPF::setInvertMod(bool invert) {
    invertMod_ = invert;
}

void ResonantLPF::setKeyTrack(double amount) {
    keyTrackAmount_ = std::clamp(amount, 0.0, 1.0);
}

void ResonantLPF::setKeyTrackFreq(double freqHz) {
    currentKeyFreq_ = freqHz;
}

void ResonantLPF::setDrive(double drive) {
    drive_ = std::max(1.0, drive);
}

void ResonantLPF::setMode(FilterMode mode) {
    mode_ = mode;
}

void ResonantLPF::setSaturationModel(FilterSaturationModel model) {
    satModel_ = model;
    // Professional tip: Assign saturation function here, not in the process loop
    switch(model) {
        case FilterSaturationModel::Soft: m_satFunc = [](double x, double d){ return fast_tanh(x * d); }; break;
        case FilterSaturationModel::Hard: m_satFunc = [](double x, double d){ return std::clamp(x * d, -1.0, 1.0); }; break;
        default: m_satFunc = [](double x, double d){ return x * d; }; break;
    }
}

double ResonantLPF::process(double input, double modSignal) {
    // Efficient Parameter Smoothing
    currentCutoff_ += (targetCutoff_ - currentCutoff_) * smoothingFactor_;
    currentResonance_ += (targetResonance_ - currentResonance_) * smoothingFactor_;

    // Calculate effective cutoff with modulation and Minilogue-style Keytracking
    double modulation = modSignal * cutoffModDepth_;
    double keyTrackOffset = currentKeyFreq_ * keyTrackAmount_;
    double effectiveCutoff = currentCutoff_ + keyTrackOffset + (invertMod_ ? -modulation : modulation);
    effectiveCutoff = std::clamp(effectiveCutoff, 10.0, sampleRate_ * 0.45);

    // Dirty-flag check: Only recalculate if effective cutoff or resonance changed
    if (effectiveCutoff != lastEffectiveCutoff_ || currentResonance_ != lastResonance_) {
        g_coeff_ = std::tan(M_PI * effectiveCutoff / sampleRate_);
        k_coeff_ = 1.0 / currentResonance_;
        a1_coeff_ = 1.0 / (1.0 + g_coeff_ * (g_coeff_ + k_coeff_));
        a2_coeff_ = g_coeff_ * a1_coeff_;
        a3_coeff_ = g_coeff_ * a2_coeff_;

        lastEffectiveCutoff_ = effectiveCutoff;
        lastResonance_ = currentResonance_;
    }

    const double v3 = input - s2_;
    const double v1 = a1_coeff_ * s1_ + a2_coeff_ * v3;
    const double v2 = s2_ + a2_coeff_ * s1_ + a3_coeff_ * v3;

    s1_ = 2.0 * v1 - s1_;
    s2_ = 2.0 * v2 - s2_;

    // Call pre-assigned saturation function (Branchless)
    lpfOutput_ = m_satFunc(v2, drive_);
    bpfOutput_ = m_satFunc(v1, drive_);
    hpfOutput_ = m_satFunc(input - k_coeff_ * v1 - v2, drive_);

    switch (mode_) {
        case FilterMode::BPF:
            return bpfOutput_;
        case FilterMode::HPF:
            return hpfOutput_;
        case FilterMode::Notch:
            return m_satFunc(lpfOutput_ + hpfOutput_, 1.0);
        case FilterMode::Peak:
            return m_satFunc(lpfOutput_ - hpfOutput_, 1.0);
        default:
            return lpfOutput_;
    }
}