#ifndef CRYSTALS_DELAY_H
#define CRYSTALS_DELAY_H

#include "tmm88/framework.h"
#include <vector>
#include <random>

struct Grain {
    double pos;      // Current read position in samples
    double progress; // 0.0 to 1.0 within the grain window
    double amp;      // Current window amplitude
    double sizeSamples; // Per-grain size to support randomization
};

class CrystalsDelay : public AudioComponent {
public:
    CrystalsDelay();
    virtual ~CrystalsDelay() override = default;

    bool init() override;
    void cleanup() override;

    void setSampleRate(double rate);
    void setDelayTime(double timeMs);
    void setPitch(double ratio); // 0.5 (octave down) to 2.0 (octave up)
    void setGrainSize(double sizeMs);
    void setFeedback(double feedback);
    
    void setRandomization(double amount); // 0.0 (Strict) to 1.0 (Cloud)
    double getRandomization() const { return randomizationAmount_; }
    
    void setReverse(bool reverse);
    bool getReverse() const { return reverse_; }
    void setDryWet(double mix);

    void process(double inputL, double inputR, double& outL, double& outR);

private:
    double sampleRate_;
    std::vector<double> bufferL_;
    std::vector<double> bufferR_;
    size_t writePos_;
    size_t maxDelaySamples_;

    // Dual grains per channel for cross-fading
    Grain grainsL[2];
    Grain grainsR[2];

    double targetDelaySamples_;
    double pitchRatio_;
    double grainSizeSamples_;
    double feedback_;
    bool reverse_;
    double mix_;

    // Randomization engine
    double randomizationAmount_;
    std::mt19937 rng_;
    std::uniform_real_distribution<double> dist_;

    static constexpr size_t kWindowLutSize = 2048;
    std::vector<double> windowLut_;
    void initWindowLut();

    void updateGrain(Grain& g, double baseDelay);
    double readBuffer(const std::vector<double>& buffer, double pos);
};

#endif // CRYSTALS_DELAY_H