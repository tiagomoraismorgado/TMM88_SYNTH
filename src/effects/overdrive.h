#ifndef OVERDRIVE_H
#define OVERDRIVE_H

#include "tmm88/framework.h"

class Overdrive : public AudioComponent {
public:
    Overdrive();
    virtual ~Overdrive() override = default;

    bool init() override;
    void cleanup() override;

    void setDrive(double drive); // Gain multiplier before saturation
    double getDrive() const { return drive_; }

    void setMix(double mix); // 0.0 (Dry) to 1.0 (Wet)
    double getMix() const { return mix_; }

    void setOutputGain(double gain); // Compensation gain
    double getOutputGain() const { return outputGain_; }

    double process(double input);
    void processBuffer(double* input, double* output, size_t size) override;

private:
    double drive_;
    double mix_;
    double outputGain_;
};

#endif // OVERDRIVE_H