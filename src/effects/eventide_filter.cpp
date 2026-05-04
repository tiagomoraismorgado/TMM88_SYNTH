#include "eventide_filter.h"
#include <cmath>
#include <algorithm>
#include <array> // For std::array
#include <immintrin.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

EventideFilter::EventideFilter() 
    : sampleRate_(44100.0), drive_(1.0), mode_(EventideFilterMode::LPF4),
      smoothingFactor_(0.001) {
    for(int i=0; i<4; ++i) {
        for(int j=0; j<4; ++j) s_stages[i][j] = 0.0;
    }
    currentCutoff_v_ = _mm256_set1_pd(1000.0);
    currentResonance_v_ = _mm256_set1_pd(0.0);
    lastEffectiveCutoff_v_ = _mm256_set1_pd(-1.0);
    lastResonance_v_ = _mm256_set1_pd(-1.0);
}

bool EventideFilter::init() {
    for(int i=0; i<4; ++i) {
        for(int j=0; j<4; ++j) s_stages[i][j] = 0.0;
    }
    currentCutoff_v_ = _mm256_set1_pd(1000.0); // Default cutoff
    currentResonance_v_ = _mm256_set1_pd(0.0); // Default resonance
    // Initialize vectorized dirty flags
    lastEffectiveCutoff_v_ = _mm256_set1_pd(-1.0);
    lastResonance_v_ = _mm256_set1_pd(-1.0);
    currentBypass_ = targetBypass_;
    return true;
}

// Scalar setters are now for the *default* values, or for single-voice processing.
// For polyphonic processing, parameters are passed directly to process_simd.
void EventideFilter::setSampleRate(double rate) { sampleRate_ = rate; }
void EventideFilter::setCutoff(double freqHz) { /* Not used directly in process_simd */ }
void EventideFilter::setResonance(double q) { /* Not used directly in process_simd */ }

void EventideFilter::setSmoothingTime(double timeMs, double sampleRate) {
    sampleRate_ = sampleRate;
    smoothingFactor_ = (timeMs <= 0.0) ? 1.0 : 1.0 - std::exp(-1.0 / (sampleRate * (timeMs / 1000.0)));
}

void EventideFilter::setDrive(double drive) {
    drive_ = std::max(1.0, drive);
}

void EventideFilter::setMode(EventideFilterMode mode) {
    mode_ = mode;
}

double EventideFilter::process(double input, double modSignal) {
    currentBypass_ += (targetBypass_ - currentBypass_) * bypassSmoothingFactor_;

    // 1. Parameter Smoothing
    currentCutoff_ += (targetCutoff_ - currentCutoff_) * smoothingFactor_;
    currentResonance_ += (targetResonance_ - currentResonance_) * smoothingFactor_;

    // 2. Dirty-flag optimization
    double effectiveCutoff = std::clamp(currentCutoff_ + modSignal, 10.0, sampleRate_ * 0.45);
    if (effectiveCutoff != lastEffectiveCutoff_ || currentResonance_ != lastResonance_) {
        g_ = std::tan(M_PI * effectiveCutoff / sampleRate_);
        h_ = g_ / (1.0 + g_);
        
        double resFeedback = currentResonance_ * 4.0;
        invDenom_ = 1.0 / (1.0 + resFeedback * g_ * g_ * g_ * g_);
        
        lastEffectiveCutoff_ = effectiveCutoff;
        lastResonance_ = currentResonance_;
    }

    double resFeedback = currentResonance_ * 4.0;
    
    // 3. SIMD Optimized State Summation (Sigma)
    // Calculates (((s0*g + s1)*g + s2)*g + s3) using FMA
    __m256d vS = _mm256_load_pd(s);
    __m256d vG_powers = _mm256_set_pd(1.0, g_, g_ * g_, g_ * g_ * g_);
    __m256d vProd = _mm256_mul_pd(vS, vG_powers);
    
    // Horizontal sum of the weighted states
    __m128d xlow = _mm256_extractf128_pd(vProd, 0);
    __m128d xhigh = _mm256_extractf128_pd(vProd, 1);
    xlow = _mm_add_pd(xlow, xhigh);
    double sigma = _mm_cvtsd_ss(_mm_hadd_ps(_mm_castpd_ps(xlow), _mm_castpd_ps(xlow)));
    // Correcting for double precision horizontal add
    sigma = ((double*)&xlow)[0] + ((double*)&xlow)[1];

    double inputWithFeedback = (input * drive_ - resFeedback * sigma) * invDenom_;
    inputWithFeedback = fast_tanh(inputWithFeedback);

    // 4. Parallel Stage Expansion (SIMD Ladder Update)
    // This computes the 4 serial stages in parallel by unrolling the recurrence:
    // y[i] = h*y[i-1] + (1-h)*s[i]
    const double h = h_;
    const double b = 1.0 - h;

    // Precompute coefficients for the input contribution [h, h^2, h^3, h^4]
    __m256d vH_poly = _mm256_set_pd(h*h*h*h, h*h*h, h*h, h);
    __m256d vVin = _mm256_set1_pd(inputWithFeedback);
    
    // State contribution matrix (Lower Triangular Expansion)
    // y0 = b*s0 + h*vin
    // y1 = h*b*s0 + b*s1 + h^2*vin ... etc
    __m256d vS0 = _mm256_set1_pd(s[0]);
    __m256d vS1 = _mm256_set1_pd(s[1]);
    __m256d vS2 = _mm256_set1_pd(s[2]);
    __m256d vS3 = _mm256_set1_pd(s[3]);

    __m256d vOut = _mm256_mul_pd(vVin, vH_poly);
    vOut = _mm256_fmadd_pd(vS0, _mm256_set_pd(h*h*h*b, h*h*b, h*b, b), vOut);
    vOut = _mm256_fmadd_pd(vS1, _mm256_set_pd(h*h*b, h*b, b, 0.0), vOut);
    vOut = _mm256_fmadd_pd(vS2, _mm256_set_pd(h*b, b, 0.0, 0.0), vOut);
    vOut = _mm256_fmadd_pd(vS3, _mm256_set_pd(b, 0.0, 0.0, 0.0), vOut);

    // Update states: s_new = s + 2*v = s + 2*h*(vin_stage - s) = s(1-2h) + 2h*vin_stage
    // In ZDF TPT: s_new = 2*y - s
    __m256d vS_new = _mm256_fmsub_pd(vOut, _mm256_set1_pd(2.0), vS);
    _mm256_store_pd(s, vS_new);

    alignas(32) double y_final[4];
    _mm256_store_pd(y_final, vOut);

    double output = y_final[3];  // Default to LPF (output of 4th stage)
    
    switch (mode_) {
        case EventideFilterMode::LPF4:  output = y_final[3]; break;
        case EventideFilterMode::HPF4:  output = input - y_final[3]; break;
        case EventideFilterMode::BPF4:  output = 6.75 * (y_final[2] - y_final[3]) * g_; break;
        case EventideFilterMode::Notch: output = input - (6.75 * (y_final[2] - y_final[3]) * g_); break;
    }

    // Crossfade for Bypass

void EventideFilter::processBuffer(double* input, double* output, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        output[i] = process(input[i], 0.0);
    }
}
    return output + currentBypass_ * (input - output);
}