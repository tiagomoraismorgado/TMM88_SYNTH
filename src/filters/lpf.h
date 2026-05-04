#ifndef LPF_H
#define LPF_H

#include "tmm88/framework.h"
#include <functional>

enum class FilterSaturationModel {
    Off,
    Soft,    // tanh
    Hard,    // Clipping
    Foldback, // Wavefolding
    Nord     // Punchy asymmetrical clipping
};

enum class FilterMode {
    LPF,
    BPF,
    HPF,
    Notch,
    Peak
};

class ResonantLPF : public AudioComponent {
public:
    ResonantLPF();
    virtual ~ResonantLPF() override = default;

    bool init() override;
    void cleanup() override;

    void setCutoff(double cutoffHz);
    void setResonance(double q); // Q factor: 0.707 is flat, > 1.0 is resonant
    void setSampleRate(double sampleRate);
    void setSmoothingTime(double timeMs, double sampleRate);

    void setCutoffModDepth(double depthHz);
    double getCutoffModDepth() const { return cutoffModDepth_; }

    void setKeyTrack(double amount); // 0.0 to 1.0
    void setKeyTrackFreq(double freqHz);

    void setSaturationModel(FilterSaturationModel model);

    void setMode(FilterMode mode);
    FilterMode getMode() const { return mode_; }

    void setDrive(double drive);
    double getDrive() const { return drive_; }

    void setInvertMod(bool invert);
    bool getInvertMod() const { return invertMod_; }

    double getLPFOutput() const { return lpfOutput_; }
    double getBPFOutput() const { return bpfOutput_; }
    double getHPFOutput() const { return hpfOutput_; }

    double process(double input, double modSignal = 0.0);
    void processBuffer(double* input, double* output, size_t size) override;

private:
    double targetCutoff_;
    double currentCutoff_;
    double targetResonance_;
    double currentResonance_;
    double cutoffModDepth_;
    double keyTrackAmount_;
    double currentKeyFreq_;
    double drive_;
    FilterSaturationModel satModel_;
    FilterMode mode_;
    bool invertMod_;
    double lpfOutput_;
    double bpfOutput_;
    double hpfOutput_;
    
    double sampleRate_;
    double smoothingFactor_;

    // Saturation function
    std::function<double(double, double)> m_satFunc;

    // State variables
    double s1_, s2_;

    // Cached coefficients and state for dirty-checking
    double lastEffectiveCutoff_;
    double lastResonance_;
    double g_coeff_;
    double k_coeff_;
    double a1_coeff_;
    double a2_coeff_;
    double a3_coeff_;
};

#endif // LPF_H