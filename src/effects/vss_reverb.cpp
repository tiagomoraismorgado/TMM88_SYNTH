#include "vss_reverb.h"
#include <algorithm>
#include <cmath>

VSSProcessor::VSSProcessor() 
    : sampleRate_(44100.0), mix_(0.3), decay_(0.5), roomSize_(1.0), 
      absorption_(0.5), m_currentPreset(VSSRoomPreset::MediumHall), 
      erWritePos_(0), highCut_(5000.0),
      filterStateL_(0.0), filterStateR_(0.0), dampStateL_(0.0), dampStateR_(0.0) {
    // Initial buffer size, will be adjusted by setPreset
    erBufferL_.resize(sampleRate_ * 2, 0.0); // Max 2 seconds for ER
    erBufferR_.resize(sampleRate_ * 2, 0.0);
}

bool VSSProcessor::init() {
    std::fill(erBufferL_.begin(), erBufferL_.end(), 0.0);
    std::fill(erBufferR_.begin(), erBufferR_.end(), 0.0);
    dampStateL_ = 0.0;
    dampStateR_ = 0.0;
    currentBypass_ = targetBypass_;
    return true;
}

void VSSProcessor::cleanup() {}

void VSSProcessor::setSampleRate(double rate) { 
    sampleRate_ = rate; 
    // Re-call setPreset to update tap delays based on new sample rate
    setPreset(m_currentPreset);
}
void VSSProcessor::setRoomSize(double size) { 
    roomSize_ = size; 
    // Re-call setPreset to update tap delays based on new room size
    setPreset(m_currentPreset);
}
void VSSProcessor::setDecayTime(double seconds) { decay_ = std::clamp(seconds / 10.0, 0.1, 0.98); }
void VSSProcessor::setHighCut(double freq) { highCut_ = freq; }
void VSSProcessor::setAbsorption(double amount) { absorption_ = std::clamp(amount, 0.0, 1.0); }
void VSSProcessor::setDryWet(double mix) { mix_ = mix; }

void VSSProcessor::setPreset(VSSRoomPreset preset) {
    m_currentPreset = preset;
    
    // Define tap patterns and gains for 15 presets (in milliseconds)
    // These are simplified and illustrative; real VSS3 would be highly tuned.
    std::vector<int> msTaps;
    std::vector<double> gains;

    switch (preset) {
        case VSSRoomPreset::SmallRoom:
            msTaps = {5, 12, 18, 25}; gains = {0.8, 0.6, 0.4, 0.2}; break;
        case VSSRoomPreset::MediumHall:
            msTaps = {15, 30, 45, 60}; gains = {0.7, 0.5, 0.3, 0.2}; break;
        case VSSRoomPreset::LargeHall:
            msTaps = {25, 50, 75, 100}; gains = {0.6, 0.4, 0.3, 0.2}; break;
        case VSSRoomPreset::Plate:
            msTaps = {3, 7, 11, 15}; gains = {0.9, 0.7, 0.5, 0.3}; break;
        case VSSRoomPreset::Cathedral:
            msTaps = {40, 70, 100, 130}; gains = {0.5, 0.4, 0.3, 0.2}; break;
        case VSSRoomPreset::Ambience:
            msTaps = {2, 5, 8, 11}; gains = {0.9, 0.8, 0.7, 0.6}; break;
        case VSSRoomPreset::Chamber:
            msTaps = {10, 20, 30, 40}; gains = {0.75, 0.6, 0.45, 0.3}; break;
        case VSSRoomPreset::Room1:
            msTaps = {7, 14, 21, 28}; gains = {0.8, 0.7, 0.5, 0.3}; break;
        case VSSRoomPreset::Room2:
            msTaps = {9, 18, 27, 36}; gains = {0.7, 0.6, 0.4, 0.2}; break;
        case VSSRoomPreset::Hall1:
            msTaps = {20, 40, 60, 80}; gains = {0.65, 0.5, 0.35, 0.2}; break;
        case VSSRoomPreset::Hall2:
            msTaps = {22, 44, 66, 88}; gains = {0.6, 0.45, 0.3, 0.15}; break;
        case VSSRoomPreset::Studio:
            msTaps = {4, 9, 14, 19}; gains = {0.85, 0.75, 0.6, 0.4}; break;
        case VSSRoomPreset::VocalPlate:
            msTaps = {3, 6, 9, 12}; gains = {0.95, 0.8, 0.65, 0.5}; break;
        case VSSRoomPreset::DrumRoom:
            msTaps = {5, 10, 15, 20}; gains = {0.9, 0.7, 0.5, 0.3}; break;
        case VSSRoomPreset::ConcertHall:
            msTaps = {30, 60, 90, 120}; gains = {0.4, 0.3, 0.2, 0.1}; break;
        default: // Fallback
            msTaps = {15, 30, 45, 60}; gains = {0.7, 0.5, 0.3, 0.2}; break;
    }

    m_taps.clear();
    m_tapGains.clear();
    int maxTapDelaySamples = 0;

    for (size_t i = 0; i < msTaps.size(); ++i) {
        // Apply roomSize scaling to tap delays
        int tapSamples = (int)((double)msTaps[i] / 1000.0 * sampleRate_ * roomSize_);
        m_taps.push_back(tapSamples);
        m_tapGains.push_back(gains[i]);
        if (tapSamples > maxTapDelaySamples) {
            maxTapDelaySamples = tapSamples;
        }
    }

    // Ensure ER buffer is large enough for the longest tap
    // Add some margin (e.g., 10% or 100 samples) to prevent read/write collisions near buffer end
    size_t requiredBufferSize = (size_t)(maxTapDelaySamples + sampleRate_ * 0.1); 
    if (requiredBufferSize < sampleRate_ * 0.1) requiredBufferSize = sampleRate_ * 0.1; // Minimum 100ms
    
    if (erBufferL_.size() < requiredBufferSize) {
        erBufferL_.resize(requiredBufferSize, 0.0);
        erBufferR_.resize(requiredBufferSize, 0.0);
        // Clear new parts of buffer to prevent noise
        std::fill(erBufferL_.begin(), erBufferL_.end(), 0.0);
        std::fill(erBufferR_.begin(), erBufferR_.end(), 0.0);
    }
    // Reset write position if buffer size changed significantly
    if (erWritePos_ >= erBufferL_.size()) {
        erWritePos_ = 0;
    }
}

void VSSProcessor::process(double inputL, double inputR, double& outL, double& outR) {
    currentBypass_ += (targetBypass_ - currentBypass_) * bypassSmoothingFactor_;

    // 1. Multi-Tap Early Reflections (TC VSS Logic)
    erBufferL_[erWritePos_] = inputL;
    erBufferR_[erWritePos_] = inputR;

    double erL = 0.0, erR = 0.0;
    for(size_t i=0; i<m_taps.size(); ++i) {
        size_t readPos = (erWritePos_ + erBufferL_.size() - m_taps[i]) % erBufferL_.size();
        erL += erBufferL_[readPos] * m_tapGains[i];
        erR += erBufferR_[readPos] * m_tapGains[i];
    }

    // 2. High Frequency Absorption (Pristine TC Filter)
    double alpha = highCut_ / sampleRate_;
    filterStateL_ += (erL - filterStateL_) * alpha;
    filterStateR_ += (erR - filterStateR_) * alpha;

    // 3. Diffuse Tail with VSS3 Absorption (Frequency-Dependent Decay)
    double tailL = filterStateL_, tailR = filterStateR_;
    
    // Apply Absorption damping (one-pole LPF in the "virtual" feedback loop)
    double damp = absorption_ * 0.8; // Max damp factor
    dampStateL_ = (tailL * (1.0 - damp)) + (dampStateL_ * damp);
    dampStateR_ = (tailR * (1.0 - damp)) + (dampStateR_ * damp);

    double wetL = erL + dampStateL_ * decay_;
    double wetR = erR + dampStateR_ * decay_;

    erWritePos_ = (erWritePos_ + 1) % erBufferL_.size();

    outL = (inputL * (1.0 - mix_)) + (wetL * mix_);
    outR = (inputR * (1.0 - mix_)) + (wetR * mix_);
    
    // Bypass logic
    outL = outL + currentBypass_ * (inputL - outL);
    outR = outR + currentBypass_ * (inputR - outR);
}