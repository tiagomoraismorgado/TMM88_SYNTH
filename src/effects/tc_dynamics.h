#ifndef TC_DYNAMICS_H
#define TC_DYNAMICS_H

#include "tmm88/framework.h"

class TCDynamics : public AudioComponent {
public:
    TCDynamics();
    virtual ~TCDynamics() override = default;

    bool init() override;
    void cleanup() override;

    void setThreshold(double db);
    void setRatio(double ratio); // 1.0 to 20.0
    void setMakeupGain(double db);

    double process(double input);

private:
    double threshold_;
    double ratio_;
    double makeup_;
    double envFollower_;
    double attackCoef_;
    double releaseCoef_;
};

#endif // TC_DYNAMICS_H