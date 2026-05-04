#ifndef VCA_H
#define VCA_H

#include "tmm88/framework.h"

class VCA : public AudioComponent {
public:
    VCA();
    virtual ~VCA() override = default;

    bool init() override;
    void cleanup() override;

    void setGain(double gain);
    double getGain() const;

    void setMorph(double morph); // 0.0 (Input A) to 1.0 (Input B)
    double getMorph() const;

    void setSmoothingTime(double timeMs, double sampleRate);

    void setDryWet(double dryWet); // 0.0 (Dry/InputA) to 1.0 (Wet/InputB)
    double getDryWet() const;

    double process(double input);
    double process(double inputA, double inputB);

private:
    double targetGain_;
    double currentGain_;
    double targetMorph_;
    double currentMorph_;
    double targetDryWet_;
    double currentDryWet_;
    
    double smoothingFactor_;
};

#endif // VCA_H