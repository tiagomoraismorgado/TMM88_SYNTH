#include <gtest/gtest.h>
#include "../include/tmm88/dsp.h"
#include <cmath>
#include <algorithm>

/**
 * @brief Regression test converting the original complex signal chain into an automated suite.
 * Verifies stability across FM modulation, filtering, and high-CPU shimmer effects.
 */
class IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        fs = 44100.0;
    }
    double fs;
};

TEST_F(IntegrationTest, FMChainStabilityAndLevelSafety) {
    Oscillator carrier(WaveformType::Sine);
    Oscillator modulator(WaveformType::Sine);
    VCA vca;
    StereoPanner panner;
    ResonantLPF filter;
    Overdrive drive;
    CrystalsDelay crystals;
    ADSR adsr;

    // Mirror configuration from main.cpp
    carrier.setFrequency(440.0);
    carrier.setSampleRate(fs);
    carrier.setFMDepth(500.0);
    modulator.setFrequency(880.0);
    modulator.setSampleRate(fs);
    vca.setSmoothingTime(0.0, fs);
    vca.setDryWet(0.5);
    filter.setSmoothingTime(0.0, fs);
    filter.setCutoff(5000.0);
    filter.setCutoffModDepth(4000.0);
    filter.setInvertMod(true);
    filter.setResonance(2.0);
    filter.setDrive(1.0);
    filter.setSampleRate(fs);
    drive.setDrive(8.0);
    drive.setMix(1.0);
    drive.setOutputGain(1.5);
    drive.setBypassSmoothingTime(500.0, fs);
    crystals.setSampleRate(fs);
    crystals.setPitch(2.0);
    crystals.setDelayTime(200.0);
    crystals.setFeedback(0.7);
    crystals.setReverse(true);
    crystals.setRandomization(0.8);
    crystals.setDryWet(0.5);
    adsr.setSampleRate(fs);
    adsr.setAttackTime(10.0);
    adsr.setDecayTime(50.0);
    adsr.setSustainLevel(0.7);
    adsr.setReleaseTime(100.0);

    ASSERT_TRUE(carrier.init());
    ASSERT_TRUE(modulator.init());
    ASSERT_TRUE(drive.init());
    ASSERT_TRUE(vca.init());
    ASSERT_TRUE(panner.init());
    ASSERT_TRUE(filter.init());
    ASSERT_TRUE(adsr.init());
    ASSERT_TRUE(crystals.init());

    adsr.gate(true, 1.0);

    double maxLevel = 0.0;
    for (int i = 0; i < 5000; i++) {
        if (i == 1000) drive.setBypass(true);

        double env = adsr.process();
        double modOut = modulator.process();
        double carrierOut = carrier.process(modOut);
        double saturated = drive.process(carrierOut);
        filter.process(saturated, env);
        double lpfOut = filter.getLPFOutput();
        vca.setGain(env);
        double vcaOut = vca.process(carrierOut, lpfOut);
        double left, right;
        panner.process(vcaOut, left, right);
        double outL, outR;
        crystals.process(left, right, outL, outR);

        maxLevel = std::max({maxLevel, std::abs(outL), std::abs(outR)});
    }

    EXPECT_GT(maxLevel, 0.001) << "The signal chain appears to be silent.";
    EXPECT_LT(maxLevel, 10.0) << "The signal chain produced an excessive peak level.";
}