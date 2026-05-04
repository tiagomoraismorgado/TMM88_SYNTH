#include <gtest/gtest.h>
#include <memory>
#include "../include/tmm88/dsp.h"

// Test fixture for DSP components
class DSPTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize with standard sample rate
        sampleRate = 44100.0;
    }

    double sampleRate;
};

// Test oscillator basic functionality
TEST_F(DSPTest, OscillatorBasic) {
    Oscillator osc(WaveformType::Sine);
    osc.init();
    osc.setSampleRate(sampleRate);
    osc.setFrequency(440.0); // A4 note

    double output[64];
    osc.processBuffer(nullptr, output, 64);

    // Check that output is within reasonable bounds
    for (int i = 0; i < 64; ++i) {
        EXPECT_GE(output[i], -1.1);
        EXPECT_LE(output[i], 1.1);
    }
}

// Test ADSR envelope
TEST_F(DSPTest, ADSREnvelope) {
    ADSR adsr;
    adsr.init();
    adsr.setSampleRate(sampleRate);
    adsr.setAttackTime(0.1);   // 100ms
    adsr.setDecayTime(0.2);    // 200ms
    adsr.setSustainLevel(0.7);  // 70%
    adsr.setReleaseTime(0.3);  // 300ms

    // Test note on
    adsr.gate(true, 1.0);
    double output[1024];
    adsr.processBuffer(nullptr, output, 1024);

    // Should start at 0 and rise during attack
    EXPECT_LT(output[0], output[100]); // Rising during attack

    // Test note off
    adsr.gate(false, 0.0);
    adsr.processBuffer(nullptr, output, 1024);

    // Should decay after note off
    EXPECT_GT(output[0], output[500]); // Falling during release
}

// Test low-pass filter
TEST_F(DSPTest, LowPassFilter) {
    ResonantLPF lpf;
    lpf.init();
    lpf.setSampleRate(sampleRate);
    lpf.setCutoff(1000.0); // 1kHz cutoff
    lpf.setResonance(0.5);

    double input[1024];
    double output[1024];

    // Generate test signal (mix of frequencies)
    for (int i = 0; i < 1024; ++i) {
        input[i] = sin(2.0 * M_PI * 440.0 * i / sampleRate) + // 440Hz
                  0.5 * sin(2.0 * M_PI * 2000.0 * i / sampleRate); // 2kHz
    }

    lpf.processBuffer(input, output, 1024);

    // High frequency component should be attenuated
    // This is a basic sanity check - more sophisticated tests would use FFT
    double inputRMS = 0.0, outputRMS = 0.0;
    for (int i = 0; i < 1024; ++i) {
        inputRMS += input[i] * input[i];
        outputRMS += output[i] * output[i];
    }
    inputRMS = sqrt(inputRMS / 1024);
    outputRMS = sqrt(outputRMS / 1024);

    // Output should be attenuated compared to input
    EXPECT_LT(outputRMS, inputRMS);
}

// Test overdrive effect
TEST_F(DSPTest, OverdriveEffect) {
    Overdrive overdrive;
    overdrive.init();
    overdrive.setDrive(2.0);
    overdrive.setOutputGain(1.0);

    double input[1024];
    double output[1024];

    // Generate sine wave input
    for (int i = 0; i < 1024; ++i) {
        input[i] = 0.8 * sin(2.0 * M_PI * 440.0 * i / sampleRate);
    }

    overdrive.processBuffer(input, output, 1024);

    // Check for saturation (output should be more compressed)
    double maxInput = 0.0, maxOutput = 0.0;
    for (int i = 0; i < 1024; ++i) {
        maxInput = std::max(maxInput, std::abs(input[i]));
        maxOutput = std::max(maxOutput, std::abs(output[i]));
    }

    // With drive=2.0, we expect some saturation
    EXPECT_GE(maxOutput, maxInput * 0.9); // Should not be much less
}

// Test stereo panner
TEST_F(DSPTest, StereoPanner) {
    StereoPanner panner;
    panner.init();
    panner.setPan(0.0); // Center
    panner.setSmoothingTime(1.0, sampleRate); // 1ms smoothing

    double input[1024];
    double output[2048]; // Stereo output

    // Generate mono input
    for (int i = 0; i < 1024; ++i) {
        input[i] = sin(2.0 * M_PI * 440.0 * i / sampleRate);
    }

    panner.processBuffer(input, output, 1024);

    // Check stereo output
    double leftRMS = 0.0, rightRMS = 0.0;
    for (int i = 0; i < 1024; ++i) {
        leftRMS += output[i * 2] * output[i * 2];
        rightRMS += output[i * 2 + 1] * output[i * 2 + 1];
    }
    leftRMS = sqrt(leftRMS / 1024);
    rightRMS = sqrt(rightRMS / 1024);

    // Center pan should have equal levels
    EXPECT_NEAR(leftRMS, rightRMS, 0.01);
}

// Test VCA (Voltage Controlled Amplifier)
TEST_F(DSPTest, VCA) {
    VCA vca;
    vca.init();
    vca.setGain(0.5); // Half level
    vca.setSmoothingTime(1.0, sampleRate); // 1ms smoothing

    double input[1024];
    double output[1024];

    // Generate input signal
    for (int i = 0; i < 1024; ++i) {
        input[i] = sin(2.0 * M_PI * 440.0 * i / sampleRate);
    }

    vca.processBuffer(input, output, 1024);

    // Check attenuation
    double inputRMS = 0.0, outputRMS = 0.0;
    for (int i = 0; i < 1024; ++i) {
        inputRMS += input[i] * input[i];
        outputRMS += output[i] * output[i];
    }
    inputRMS = sqrt(inputRMS / 1024);
    outputRMS = sqrt(outputRMS / 1024);

    // Should be approximately half level
    EXPECT_NEAR(outputRMS, inputRMS * 0.5, 0.01);
}