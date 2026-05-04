#ifndef OSCILLATOR_H
#define OSCILLATOR_H

#include "tmm88/framework.h"
#include <immintrin.h>
#include <vector>

enum class WaveformType {
    Sine,
    Sawtooth,
    Square,
    Wavetable
};

class Oscillator : public AudioComponent {
public:
    Oscillator(WaveformType type = WaveformType::Sawtooth);
    virtual ~Oscillator() override = default;

    bool init() override;
    void cleanup() override;

    void setType(WaveformType type) { type_ = type; }
    WaveformType getType() const { return type_; }

    void setFrequency(double freq);
    void setSampleRate(double rate);
    void setUnison(int numVoices, double detune); // detune in 0.0 to 1.0 range
    void setShape(double shape);
    void setFMDepth(double depth);
    void setWavetable(const std::vector<double>& table);

    double process(double fmSignal = 0.0);
    void processBuffer(double* input, double* output, size_t size) override;

private:
    WaveformType type_;
    double baseFrequency_;
    double sampleRate_;
    double shape_;
    double fmDepth_;

    int numVoices_;
    double detuneAmount_;

    std::vector<double> wavetable_;
    double wavetableSize_;

    // Aligned buffers for AVX processing (max 8 voices supported in this block)
    alignas(32) double phases_[8];
    alignas(32) double increments_[8];
};

#endif // OSCILLATOR_H