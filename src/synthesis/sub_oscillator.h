#ifndef SUB_OSCILLATOR_H
#define SUB_OSCILLATOR_H

#include "tmm88/framework.h"

class SubOscillator : public AudioComponent {
public:
    SubOscillator();
    virtual ~SubOscillator() override = default;

    bool init() override;
    void cleanup() override;

    void setSampleRate(double rate);
    void setOctave(int octave); // 1 or 2 octaves below
    int getOctave() const { return octave_; }

    void setFrequency(double frequency) { frequency_ = frequency; }
    double getFrequency() const { return frequency_; }

    double process(double frequency);
    void processBuffer(double* input, double* output, size_t size) override;

private:
    double phase_;
    double sampleRate_;
    int octave_;
    double frequency_;
};

#endif // SUB_OSCILLATOR_H