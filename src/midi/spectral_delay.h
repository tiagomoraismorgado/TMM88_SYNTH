#ifndef SPECTRAL_DELAY_H
#define SPECTRAL_DELAY_H

#include "tmm88/framework.h"
#include <vector>

class SpectralDelay : public AudioComponent {
public:
    SpectralDelay();
    virtual ~SpectralDelay() override = default;

    bool init() override;
    void cleanup() override;

    void setSampleRate(double rate);
    void setDelayTime(double timeMs);
    void setFeedback(double feedback);
    void setTone(double tone); // 0.0 (LPF) -> 0.5 (Flat) -> 1.0 (HPF)
    void setModulation(double depth, double rate);
    void setHazarai(double smear); // Controls diffusion/multi-tap density
    void setDryWet(double mix);

    void process(double inputL, double inputR, double& outL, double& outR);

private:
    double interpolate(const std::vector<double>& buffer, double readPos);

    double sampleRate_;
    size_t bufferSize_;
    std::vector<double> bufferL_;
    std::vector<double> bufferR_;
    size_t writePos_;

    double targetDelaySamples_;
    double currentDelaySamples_;
    double feedback_;
    double tone_;
    double smear_;
    double mix_;

    // Modulation
    double lfoPhase_;
    double modDepth_;
    double modRate_;

    // Internal Filtering (simple one-pole)
    double filterStateL_;
    double filterStateR_;
};

#endif // SPECTRAL_DELAY_H