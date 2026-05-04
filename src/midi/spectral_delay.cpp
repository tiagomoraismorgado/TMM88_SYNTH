#include "spectral_delay.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

SpectralDelay::SpectralDelay() 
    : sampleRate_(44100.0), bufferSize_(88200), writePos_(0), 
      targetDelaySamples_(4410.0), currentDelaySamples_(4410.0),
      feedback_(0.4), tone_(0.5), smear_(0.3), mix_(0.5),
      lfoPhase_(0.0), modDepth_(0.0), modRate_(0.5),
      filterStateL_(0.0), filterStateR_(0.0) {
    bufferL_.resize(bufferSize_, 0.0);
    bufferR_.resize(bufferSize_, 0.0);
}

bool SpectralDelay::init() {
    std::fill(bufferL_.begin(), bufferL_.end(), 0.0);
    std::fill(bufferR_.begin(), bufferR_.end(), 0.0);
    return true;
}

void SpectralDelay::cleanup() {}

void SpectralDelay::setSampleRate(double rate) {
    sampleRate_ = rate;
    bufferSize_ = (size_t)(rate * 2.0); // 2 seconds max
    bufferL_.assign(bufferSize_, 0.0);
    bufferR_.assign(bufferSize_, 0.0);
}

void SpectralDelay::setDelayTime(double timeMs) {
    targetDelaySamples_ = (timeMs / 1000.0) * sampleRate_;
}

void SpectralDelay::setFeedback(double feedback) {
    feedback_ = std::clamp(feedback, 0.0, 1.1); // Allow slight over-unity for "SMMH Self-oscillation"
}

void SpectralDelay::setTone(double tone) {
    tone_ = std::clamp(tone, 0.0, 1.0);
}

void SpectralDelay::setHazarai(double smear) {
    smear_ = std::clamp(smear, 0.0, 1.0);
}

void SpectralDelay::setDryWet(double mix) {
    mix_ = std::clamp(mix, 0.0, 1.0);
}

double SpectralDelay::interpolate(const std::vector<double>& buffer, double readPos) {
    // Handle negative read positions by wrapping around the buffer
    while (readPos < 0) readPos += bufferSize_;
    readPos = std::fmod(readPos, bufferSize_);
    
    size_t i0 = (size_t)std::floor(readPos);
    size_t i1 = (i0 + 1) % bufferSize_;
    double frac = readPos - i0;
    return buffer[i0] + frac * (buffer[i1] - buffer[i0]);
}

void SpectralDelay::process(double inputL, double inputR, double& outL, double& outR) {
    // Smoothing delay time
    currentDelaySamples_ += (targetDelaySamples_ - currentDelaySamples_) * 0.001;

    // LFO Modulation
    lfoPhase_ += (2.0 * M_PI * modRate_) / sampleRate_;
    if (lfoPhase_ >= 2.0 * M_PI) lfoPhase_ -= 2.0 * M_PI;
    double mod = std::sin(lfoPhase_) * modDepth_ * 100.0;

    double readPos = (double)writePos_ - (currentDelaySamples_ + mod);
    while (readPos < 0) readPos += bufferSize_;

    // Read from Delay Line
    double delayedL = interpolate(bufferL_, readPos);
    double delayedR = interpolate(bufferR_, readPos);

    // Bipolar Tone Filtering (Bypass smoothing logic integrated in Tone)
    double cutoff = (tone_ < 0.5) ? (tone_ * 2.0) : ((tone_ - 0.5) * 2.0);
    if (tone_ < 0.5) { // LPF Mode
        filterStateL_ += (delayedL - filterStateL_) * (0.01 + cutoff * 0.5);
        filterStateR_ += (delayedR - filterStateR_) * (0.01 + cutoff * 0.5);
        delayedL = filterStateL_;
        delayedR = filterStateR_;
    } else { // HPF Mode
        filterStateL_ += (delayedL - filterStateL_) * (0.01 + cutoff * 0.5);
        filterStateR_ += (delayedR - filterStateR_) * (0.01 + cutoff * 0.5);
        delayedL = delayedL - filterStateL_;
        delayedR = delayedR - filterStateR_;
    }

    // "Hazarai" Smear (Diffusion)
    delayedL = delayedL + smear_ * (std::sin(delayedL * 0.5)); // Non-linear dispersion
    delayedR = delayedR + smear_ * (std::cos(delayedR * 0.5));

    // Feedback Path (Ping-Pong Cross)
    bufferL_[writePos_] = inputL + delayedR * feedback_;
    bufferR_[writePos_] = inputR + delayedL * feedback_;

    writePos_ = (writePos_ + 1) % bufferSize_;

    outL = (inputL * (1.0 - mix_)) + (delayedL * mix_);
    outR = (inputR * (1.0 - mix_)) + (delayedR * mix_);
}