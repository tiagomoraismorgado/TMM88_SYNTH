#ifndef STEREO_PANNER_H
#define STEREO_PANNER_H

#include "tmm88/framework.h"

class StereoPanner : public AudioComponent {
public:
    StereoPanner();
    virtual ~StereoPanner() override = default;

    bool init() override;
    void cleanup() override;

    void setPan(double pan); // -1.0 (Left) to 1.0 (Right)
    double getPan() const;
    void setSmoothingTime(double timeMs, double sampleRate);
    void process(double input, double& left, double& right);

private:
    double targetPan_;
    double currentPan_;
    double smoothingFactor_;
};

#endif // STEREO_PANNER_H