#include "adsr.h"
#include <algorithm>
#include <cmath>

ADSR::ADSR() 
    : state_(ADSRState::Idle), currentLevel_(0.0), sampleRate_(44100.0),
      attackCoef_(0.001), attackLinearStep_(0.001), attackCurve_(1.0), 
      holdSamples_(0.0), holdSlope_(0.0), holdCounter_(0.0), decayLinearStep_(0.001), decayCurve_(1.0), 
      decayCoef_(0.001), baseAttackTimeMs_(0.0), baseHoldTimeMs_(0.0), baseDecayTimeMs_(0.0),
      swing_(0.5), swingOffset_(0), swingSensitivity_(0.0), lastInputVelocity_(1.0),
      sustainLevel_(0.7), releaseTimeMs_(0.0), 
      releaseCoef_(0.001), releaseLinearStep_(0.001), activeReleaseLinearStep_(0.001), releaseCurve_(1.0),
      legato_(false), velocity_(1.0), velocitySensitivity_(1.0), retriggerThreshold_(0.0), loop_(false), loopCount_(-1), currentLoop_(0), syncPhase_(0.0), syncFadeTimeMs_(0.0), syncFadeSmoothingFactor_(1.0), targetSyncLevel_(0.0), isSyncFading_(false) {}

bool ADSR::init() {
    state_ = ADSRState::Idle;
    currentLevel_ = 0.0;
    return true;
}

void ADSR::cleanup() {}

void ADSR::setSampleRate(double rate) {
    sampleRate_ = rate;
}

void ADSR::setAttackTime(double timeMs) {
    baseAttackTimeMs_ = timeMs;
    updateInternalTimings();
}

void ADSR::setHoldTime(double timeMs) {
    baseHoldTimeMs_ = timeMs;
    updateInternalTimings();
}

void ADSR::setDecayTime(double timeMs) {
    baseDecayTimeMs_ = timeMs;
    updateInternalTimings();
}

void ADSR::updateInternalTimings() {
    // Calculate effective swing modulated by velocity and sensitivity
    double modulation = (1.0 - swingSensitivity_) + (lastInputVelocity_ * swingSensitivity_);
    double effectiveSwing = 0.5 + (swing_ - 0.5) * modulation;

    // Calculate swing multiplier: even loops use swing, odd loops use 1-swing
    // Multiplied by 2.0 so that 0.5 results in 1.0 (straight timing)
    double multiplier = ((currentLoop_ + swingOffset_) % 2 == 0) ? (effectiveSwing * 2.0) : ((1.0 - effectiveSwing) * 2.0);

    // Temporarily apply multiplier to stages
    double adjAttack = baseAttackTimeMs_ * multiplier;
    double adjHold = baseHoldTimeMs_ * multiplier;
    double adjDecay = baseDecayTimeMs_ * multiplier;

    // Update coefficients (Attack)
    if (adjAttack <= 0.0) {
        attackCoef_ = 1.0;
    } else {
        attackCoef_ = 1.0 - std::exp(-1.0 / (sampleRate_ * (adjAttack / 1000.0)));
    }
    double aSamples = (adjAttack / 1000.0) * sampleRate_;
    attackLinearStep_ = (aSamples > 0.0) ? (1.0 / aSamples) : 1.0;

    // Update Hold
    holdSamples_ = (adjHold / 1000.0) * sampleRate_;

    // Update Decay
    if (adjDecay <= 0.0) {
        decayCoef_ = 1.0;
    } else {
        decayCoef_ = 1.0 - std::exp(-1.0 / (sampleRate_ * (adjDecay / 1000.0)));
    }
    double dSamples = (adjDecay / 1000.0) * sampleRate_;
    decayLinearStep_ = (dSamples > 0.0) ? ((1.0 - sustainLevel_) / dSamples) : 1.0;
}

void ADSR::setSwing(double swing) {
    swing_ = std::clamp(swing, 0.0, 1.0);
    updateInternalTimings();
}

void ADSR::setSwingOffset(int offset) {
    swingOffset_ = offset;
    updateInternalTimings();
}

void ADSR::setSwingSensitivity(double sensitivity) {
    swingSensitivity_ = std::clamp(sensitivity, 0.0, 1.0);
}

void ADSR::setTempoSync(double bpm, double attackDiv, double holdDiv, double decayDiv, double releaseDiv) {
    if (bpm <= 0.0) return;

    // Calculate the duration of one beat (quarter note) in milliseconds
    double beatMs = 60000.0 / bpm;

    setAttackTime(beatMs * attackDiv);
    setHoldTime(beatMs * holdDiv);
    setDecayTime(beatMs * decayDiv);
    setReleaseTime(beatMs * releaseDiv);
}

void ADSR::setHoldSlope(double slope) {
    holdSlope_ = slope;
}

void ADSR::setSustainLevel(double level) {
    sustainLevel_ = std::clamp(level, 0.0, 1.0);
    // Recalculate timings to account for new sustain travel distance
    updateInternalTimings();
}

void ADSR::setReleaseTime(double timeMs) {
    releaseTimeMs_ = timeMs;
    if (timeMs <= 0.0) {
        releaseCoef_ = 1.0;
    } else {
        releaseCoef_ = 1.0 - std::exp(-1.0 / (sampleRate_ * (timeMs / 1000.0)));
    }
    double samples = (timeMs / 1000.0) * sampleRate_;
    releaseLinearStep_ = (samples > 0.0) ? (1.0 / samples) : 1.0;
}

void ADSR::setAttackCurve(double curve) {
    attackCurve_ = std::clamp(curve, 0.0, 1.0);
}

void ADSR::setDecayCurve(double curve) {
    decayCurve_ = std::clamp(curve, 0.0, 1.0);
}

void ADSR::setReleaseCurve(double curve) {
    releaseCurve_ = std::clamp(curve, 0.0, 1.0);
}

void ADSR::setVelocitySensitivity(double sensitivity) {
    velocitySensitivity_ = std::clamp(sensitivity, 0.0, 1.0);
}

void ADSR::setLegato(bool enabled) {
    legato_ = enabled;
}

void ADSR::setRetriggerThreshold(double threshold) {
    retriggerThreshold_ = std::clamp(threshold, 0.0, 1.0);
}

void ADSR::setLoop(bool enabled) {
    loop_ = enabled;
}

void ADSR::setSyncPhase(double phase) {
    syncPhase_ = std::clamp(phase, 0.0, 1.0);
}

void ADSR::setSyncFadeTime(double timeMs) {
    syncFadeTimeMs_ = timeMs;
    if (timeMs <= 0.0) {
        syncFadeSmoothingFactor_ = 1.0; // Instant transition
    } else {
        syncFadeSmoothingFactor_ = 1.0 - std::exp(-1.0 / (sampleRate_ * (timeMs / 1000.0)));
    }
}

void ADSR::setLoopCount(int count) {
    loopCount_ = count;
}

void ADSR::triggerSync() {
    // Sync restarts the rhythmic cycle if a note is currently being held or released
    if (state_ != ADSRState::Idle) {
        state_ = ADSRState::Attack;
        holdCounter_ = 0.0;
        currentLoop_ = 0; // Reset the loop budget for the new synced sequence
        updateInternalTimings();
        
        targetSyncLevel_ = syncPhase_ * velocity_;

        // Only fade if there's a significant difference and fade time is set
        if (syncFadeTimeMs_ > 0.0 && std::abs(currentLevel_ - targetSyncLevel_) > 0.0001) {
            isSyncFading_ = true;
        } else {
            currentLevel_ = targetSyncLevel_; // Instant jump if no fade time or already close
            isSyncFading_ = false;
        }
    }
}

void ADSR::gate(bool on, double velocity) {
    if (on) {
        // Check if we should force a retrigger even in legato mode
        bool forceRetrigger = (legato_ && state_ != ADSRState::Idle && currentLevel_ <= retriggerThreshold_);

        // Legato Logic: If a note is already active, we handle the re-trigger gracefully
        if (legato_ && state_ != ADSRState::Idle && !forceRetrigger) {
            // If we were in the middle of a release, return to Sustain
            if (state_ == ADSRState::Release) {
                state_ = ADSRState::Sustain;
            }
            // Otherwise, ignore the trigger to let the current envelope shape continue
            return;
        }

        // Calculate effective velocity based on sensitivity
        // Sensitivity 0.0 = Always 1.0 (ignore input)
        // Sensitivity 1.0 = Follow input exactly
        double inputVelocity = std::clamp(velocity, 0.0, 1.0);
        lastInputVelocity_ = inputVelocity;
        velocity_ = (1.0 - velocitySensitivity_) + (inputVelocity * velocitySensitivity_);

        // Standard/Retrigger Logic:
        // Reset to zero if we are not in legato mode, or if the retrigger threshold was met
        if (!legato_ || forceRetrigger) {
            currentLevel_ = 0.0; 
        }
        
        currentLoop_ = 0;
        holdCounter_ = 0.0;
        updateInternalTimings();
        isSyncFading_ = false; // Ensure sync fade is off when a new gate starts
        state_ = ADSRState::Attack;
    } else if (state_ != ADSRState::Idle) {
        // Calculate dynamic linear step to ensure time consistency
        // We want to travel from currentLevel_ to 0.0 in exactly releaseTimeMs_
        double samples = (releaseTimeMs_ / 1000.0) * sampleRate_;
        activeReleaseLinearStep_ = (samples > 0.0) ? (currentLevel_ / samples) : currentLevel_;
        
        isSyncFading_ = false; // Ensure sync fade is off when releasing
        state_ = ADSRState::Release;
    }
}

double ADSR::process() {
    if (isSyncFading_) {
        currentLevel_ += (targetSyncLevel_ - currentLevel_) * syncFadeSmoothingFactor_;
        if (std::abs(currentLevel_ - targetSyncLevel_) < 1e-5) {
            currentLevel_ = targetSyncLevel_;
            isSyncFading_ = false;
        }
        return currentLevel_;
    }

    switch (state_) {
        case ADSRState::Idle: currentLevel_ = 0.0; break;
        case ADSRState::Attack:
            currentLevel_ += ((velocity_ * 1.01 - currentLevel_) * attackCoef_ * attackCurve_) + 
                             (attackLinearStep_ * velocity_ * (1.0 - attackCurve_));
            if (currentLevel_ >= velocity_) {
                currentLevel_ = velocity_; holdCounter_ = 0.0;
                state_ = ADSRState::Hold;
            }
            break;
        case ADSRState::Hold:
            currentLevel_ += holdSlope_ * velocity_;
            currentLevel_ = std::max(0.0, currentLevel_);
            holdCounter_ += 1.0; 
            if (holdCounter_ >= holdSamples_) {
                state_ = ADSRState::Decay;
            }
            break;
        case ADSRState::Decay:
        {
            const double target = velocity_ * sustainLevel_;
            currentLevel_ += ((target - currentLevel_) * decayCoef_ * decayCurve_) - 
                             (decayLinearStep_ * velocity_ * (1.0 - decayCurve_));
            if (currentLevel_ <= target) {
                if (loop_ && (loopCount_ < 0 || currentLoop_ < loopCount_)) {
                    holdCounter_ = 0.0; currentLoop_++;
                    updateInternalTimings();
                    state_ = ADSRState::Attack;
                } else {
                    currentLevel_ = target;
                    state_ = ADSRState::Sustain;
                }
            }
            break;
        }

        case ADSRState::Sustain:
            currentLevel_ = velocity_ * sustainLevel_;
            break;

        case ADSRState::Release:
            {
                double expDelta = (currentLevel_ - (-0.01)) * releaseCoef_;
                double linDelta = activeReleaseLinearStep_;
                currentLevel_ -= (expDelta * releaseCurve_) + (linDelta * (1.0 - releaseCurve_));
            }
            if (currentLevel_ <= 0.0) {
                currentLevel_ = 0.0;
                state_ = ADSRState::Idle;
            }
            break;
    }

    return currentLevel_;
}

void ADSR::processBuffer(double* input, double* output, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        output[i] = process();
    }
}