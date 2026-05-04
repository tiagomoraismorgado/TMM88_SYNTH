#include "tc_dynamics.h"
#include <cmath>
#include <algorithm>

TCDynamics::TCDynamics() 
    : threshold_(0.5), ratio_(2.0), makeup_(1.0), envFollower_(0.0),
      attackCoef_(0.01), releaseCoef_(0.001) {}

bool TCDynamics::init() { return true; }
void TCDynamics::cleanup() {}

void TCDynamics::setThreshold(double db) { threshold_ = std::pow(10.0, db / 20.0); }
void TCDynamics::setRatio(double ratio) { ratio_ = ratio; }
void TCDynamics::setMakeupGain(double db) { makeup_ = std::pow(10.0, db / 20.0); }

double TCDynamics::process(double input) {
    double absInput = std::abs(input);
    
    // Attack/Release envelope
    double coef = (absInput > envFollower_) ? attackCoef_ : releaseCoef_;
    envFollower_ += (absInput - envFollower_) * coef;

    double gain = 1.0;
    if (envFollower_ > threshold_) {
        // TC Soft-Knee logic
        double excess = envFollower_ - threshold_;
        double reduction = excess * (1.0 - 1.0 / ratio_);
        gain = (envFollower_ - reduction) / envFollower_;
    }

    return input * gain * makeup_;
}