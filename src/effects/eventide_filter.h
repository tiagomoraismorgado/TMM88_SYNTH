#ifndef EVENTIDE_FILTER_H
#define EVENTIDE_FILTER_H

#include "tmm88/framework.h"

enum class EventideFilterMode {
    LPF4, // 24dB Low Pass
    HPF4, // 24dB High Pass
    BPF4, // 24dB Band Pass
    Notch
};

class EventideFilter : public AudioComponent {
public:
    EventideFilter();
    virtual ~EventideFilter() override = default;

    bool init() override;
    void cleanup() override;

    void setSampleRate(double rate);
    void setCutoff(double freqHz);
    void setResonance(double q); // 0.0 to 1.0 (highly resonant)
    void setSmoothingTime(double timeMs, double sampleRate);
    void setDrive(double drive);   // 1.0 to 10.0
    void setMode(EventideFilterMode mode);

    double process(double input, double modSignal = 0.0);
    __m256d process_simd(__m256d input, __m256d targetCutoff_v, __m256d targetResonance_v, __m256d modSignal_v = _mm256_setzero_pd());
    void processBuffer(double* input, double* output, size_t size) override; // This will call process_simd

private:
    double sampleRate_;
    // These are now per-voice (vectorized)
    __m256d currentCutoff_v_;
    __m256d currentResonance_v_;

    double drive_;
    EventideFilterMode mode_;

    alignas(32) double s_stages[4][4]; // s_stages[stage][voice]
    
    // Coefficients
    __m256d g_coeffs_;
    __m256d h_coeffs_;
    __m256d invDenom_coeffs_; // Cached pre-calculated denominator for feedback

    // Dirty-flag state
    __m256d lastEffectiveCutoff_v_; // Stores the last effective cutoff for each voice
    __m256d lastResonance_v_;       // Stores the last resonance for each voice
    double smoothingFactor_;

};

#endif // EVENTIDE_FILTER_H