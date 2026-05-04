#include "crystals_delay.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

CrystalsDelay::CrystalsDelay() 
    : sampleRate_(44100.0), writePos_(0), maxDelaySamples_(176400),
      targetDelaySamples_(22050.0), pitchRatio_(2.0), 
      grainSizeSamples_(4410.0), feedback_(0.5), reverse_(false), mix_(0.5),
      randomizationAmount_(0.0), rng_(std::random_device{}()), dist_(-1.0, 1.0) {
    bufferL_.resize(maxDelaySamples_, 0.0);
    bufferR_.resize(maxDelaySamples_, 0.0);
    initWindowLut();
}

bool CrystalsDelay::init() {
    std::fill(bufferL_.begin(), bufferL_.end(), 0.0);
    std::fill(bufferR_.begin(), bufferR_.end(), 0.0);
    
    for(int i=0; i<2; ++i) {
        grainsL[i] = {0.0, (double)i * 0.5, 0.0, grainSizeSamples_};
        grainsR[i] = {0.0, (double)i * 0.5, 0.0, grainSizeSamples_};
    }

    currentBypass_ = targetBypass_;
    return true;
}

void CrystalsDelay::cleanup() {}

void CrystalsDelay::setSampleRate(double rate) {
    sampleRate_ = rate;
    maxDelaySamples_ = (size_t)(rate * 4.0); // 4 seconds max
    bufferL_.assign(maxDelaySamples_, 0.0);
    bufferR_.assign(maxDelaySamples_, 0.0);
}

void CrystalsDelay::setPitch(double ratio) { pitchRatio_ = ratio; }
void CrystalsDelay::setDelayTime(double timeMs) { targetDelaySamples_ = (timeMs / 1000.0) * sampleRate_; }
void CrystalsDelay::setGrainSize(double sizeMs) { grainSizeSamples_ = (sizeMs / 1000.0) * sampleRate_; }
void CrystalsDelay::setFeedback(double feedback) { feedback_ = std::clamp(feedback, 0.0, 0.95); }
void CrystalsDelay::setRandomization(double amount) { randomizationAmount_ = std::clamp(amount, 0.0, 1.0); }
void CrystalsDelay::setReverse(bool reverse) { reverse_ = reverse; }
void CrystalsDelay::setDryWet(double mix) { mix_ = std::clamp(mix, 0.0, 1.0); }

void CrystalsDelay::initWindowLut() {
    windowLut_.resize(kWindowLutSize);
    for (size_t i = 0; i < kWindowLutSize; ++i) {
        windowLut_[i] = std::sin(((double)i / (double)(kWindowLutSize - 1)) * M_PI);
    }
}

double CrystalsDelay::readBuffer(const std::vector<double>& buffer, double pos) {
    while (pos < 0) pos += maxDelaySamples_;
    while (pos >= maxDelaySamples_) pos -= maxDelaySamples_;
    
    long i0 = (long)std::floor(pos);
    long i1 = (i0 + 1) % maxDelaySamples_;
    double frac = pos - i0;
    return buffer[i0] + frac * (buffer[i1] - buffer[i0]);
}

void CrystalsDelay::updateGrain(Grain& g, double baseDelay) {
    // Move the read head. If reverse is enabled, move backwards.
    double speed = reverse_ ? -pitchRatio_ : pitchRatio_;
    g.pos += speed;

    // Advance grain progress
    g.progress += 1.0 / g.sizeSamples;

    if (g.progress >= 1.0) {
        g.progress = 0.0; 
        // Calculate randomization jitter
        double jitter = dist_(rng_) * randomizationAmount_;
        g.sizeSamples = grainSizeSamples_ * (1.0 + jitter * 0.5);
        
        // Jitter the start position (up to +/- 50ms relative to base delay)
        double posJitter = dist_(rng_) * randomizationAmount_ * (sampleRate_ * 0.05);
        g.pos = (double)writePos_ - (baseDelay + posJitter);
    }

    // Sinusoidal windowing for smooth cross-fading
    double lutPos = g.progress * (double)(kWindowLutSize - 1);
    size_t i0 = (size_t)lutPos;
    size_t i1 = std::min(i0 + 1, kWindowLutSize - 1);
    double frac = lutPos - i0;
    g.amp = windowLut_[i0] + frac * (windowLut_[i1] - windowLut_[i0]);
}

void CrystalsDelay::process(double inputL, double inputR, double& outL, double& outR) {
    currentBypass_ += (targetBypass_ - currentBypass_) * bypassSmoothingFactor_;

    double wetL = 0.0;
    double wetR = 0.0;

    // Process dual grains to allow for overlap/pitch shifting
    for (int i = 0; i < 2; ++i) {
        updateGrain(grainsL[i], targetDelaySamples_);
        updateGrain(grainsR[i], targetDelaySamples_);

        wetL += readBuffer(bufferL_, grainsL[i].pos) * grainsL[i].amp;
        wetR += readBuffer(bufferR_, grainsR[i].pos) * grainsR[i].amp;
    }

    // Write to buffer with feedback
    bufferL_[writePos_] = inputL + wetL * feedback_;
    bufferR_[writePos_] = inputR + wetR * feedback_;

    writePos_ = (writePos_ + 1) % maxDelaySamples_;

    // Final Mix and Bypass crossfade
    double finalWetL = wetL * mix_;
    double finalWetR = wetR * mix_;
    
    outL = (inputL * (1.0 - mix_)) + finalWetL;
    outR = (inputR * (1.0 - mix_)) + finalWetR;
    
    outL = outL + currentBypass_ * (inputL - outL);
    outR = outR + currentBypass_ * (inputR - outR);
}