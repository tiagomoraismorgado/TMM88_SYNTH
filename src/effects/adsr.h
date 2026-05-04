#ifndef ADSR_H
#define ADSR_H

#include "tmm88/framework.h"

enum class ADSRState {
    Idle,
    Attack,
    Hold,
    Decay,
    Sustain,
    Release
};

class ADSR : public AudioComponent {
public:
    ADSR();
    virtual ~ADSR() override = default;

    bool init() override;
    void cleanup() override;

    void setSampleRate(double rate);
    void setAttackTime(double timeMs);
    void setHoldTime(double timeMs);
    void setDecayTime(double timeMs);

    void setTempoSync(double bpm, double attackDiv, double holdDiv, double decayDiv, double releaseDiv);

    void setHoldSlope(double slope); // Change per sample, scaled by velocity
    double getHoldSlope() const { return holdSlope_; }

    void setSustainLevel(double level);
    void setReleaseTime(double timeMs);

    void setAttackCurve(double curve);  // 0.0 = Linear, 1.0 = Exponential
    void setDecayCurve(double curve);   // 0.0 = Linear, 1.0 = Exponential
    void setReleaseCurve(double curve); // 0.0 = Linear, 1.0 = Exponential

    void setLegato(bool enabled);
    bool isLegato() const { return legato_; }

    void setRetriggerThreshold(double threshold);
    double getRetriggerThreshold() const { return retriggerThreshold_; }

    void setLoop(bool enabled);
    bool isLoop() const { return loop_; }

    void setSwing(double swing); // 0.5 = Straight, > 0.5 = Swing feel
    double getSwing() const { return swing_; }

    void setSwingOffset(int offset); // Shifting the even/odd cycle alignment
    int getSwingOffset() const { return swingOffset_; }

    void setSwingSensitivity(double sensitivity);
    double getSwingSensitivity() const { return swingSensitivity_; }

    void triggerSync(); // Restarts the cycle if the gate is active
    
    void setSyncPhase(double phase); // 0.0 (Start of Attack) to 1.0 (End of Attack)
    double getSyncPhase() const { return syncPhase_; }

    void setSyncFadeTime(double timeMs); // Time to glide to sync phase
    double getSyncFadeTime() const { return syncFadeTimeMs_; }

    void setLoopCount(int count); // -1 for infinite, 0 for no loops, >0 for specific count
    int getLoopCount() const { return loopCount_; }

    void setVelocitySensitivity(double sensitivity);
    double getVelocitySensitivity() const { return velocitySensitivity_; }

    void gate(bool on, double velocity = 1.0);
    double process();
    void processBuffer(double* input, double* output, size_t size) override;
    ADSRState getState() const { return state_; }

private:
    void updateInternalTimings();

    ADSRState state_;
    double currentLevel_;
    double sampleRate_;

    double attackCoef_;  // Smoothing coefficient
    double attackLinearStep_;
    double attackCurve_;
    double holdSamples_; // Duration of hold in samples
    double holdSlope_;   // Per-sample increment
    double holdCounter_; // Current sample count in hold state
    double decayLinearStep_;
    double decayCurve_;
    double decayCoef_;   // Smoothing coefficient
    double baseAttackTimeMs_;
    double baseHoldTimeMs_;
    double baseDecayTimeMs_;
    
    double swing_;
    int swingOffset_;
    double swingSensitivity_;
    double lastInputVelocity_;

    double sustainLevel_;
    double releaseTimeMs_;
    double releaseCoef_; // Smoothing coefficient
    double releaseLinearStep_;
    double activeReleaseLinearStep_;
    double releaseCurve_;
    bool legato_;
    double velocity_;
    double velocitySensitivity_;
    double retriggerThreshold_;
    bool loop_;
    int loopCount_;
    int currentLoop_;
    double syncPhase_;
    // Sync Fade parameters
    double syncFadeTimeMs_;
    double syncFadeSmoothingFactor_;
    double targetSyncLevel_;
    bool isSyncFading_;
};

#endif // ADSR_H