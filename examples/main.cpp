#include "tmm88/dsp.h"
#include <iostream>

int main() {
    std::cout << "=== DSP Framework Example ===" << std::endl;
    
    // Create components
    Oscillator carrier(WaveformType::Sine);
    Oscillator modulator(WaveformType::Sine);
    VCA vca;
    StereoPanner panner;
    ResonantLPF filter;
    Overdrive drive;
    CrystalsDelay crystals;
    ADSR adsr;
    
    // Configure parameters BEFORE init to snap to initial values
    const double fs = 44100.0;
    
    // Carrier Configuration (The audible pitch)
    carrier.setFrequency(440.0);
    carrier.setSampleRate(fs);
    carrier.setFMDepth(500.0); // How much the modulator affects the carrier

    // Modulator Configuration (Determines the timbre)
    modulator.setFrequency(880.0); // 1:2 Ratio for harmonic overtones
    modulator.setSampleRate(fs);
    
    // For ADSR modulation, we set VCA smoothing to 0 to respond immediately to the envelope
    vca.setSmoothingTime(0.0, fs);
    vca.setDryWet(0.5); // 50/50 mix of Dry and Wet signals
    panner.setPan(0.0);
    filter.setSmoothingTime(0.0, fs);
    filter.setCutoff(5000.0);   // High base cutoff for downward sweep
    filter.setCutoffModDepth(4000.0); // Sweep depth
    filter.setInvertMod(true);  // Sweep DOWN from 5000Hz to 1000Hz
    filter.setResonance(2.0);   // Higher resonance to make the sweep more audible
    filter.setDrive(1.0);       // Neutral drive for internal filter saturation
    filter.setSampleRate(fs);

    // Overdrive Configuration (Pre-filter saturation)
    drive.setDrive(8.0);
    drive.setMix(1.0); // Fully wet to demonstrate the effect
    drive.setOutputGain(1.5); // Boost volume after heavy saturation
    drive.setBypassSmoothingTime(500.0, fs); // 500ms long crossfade for bypass

    // Crystals Configuration (Cloud-like Reverse Shimmer)
    crystals.setSampleRate(fs);
    crystals.setPitch(2.0);       // Octave up
    crystals.setDelayTime(200.0); // 200ms base delay
    crystals.setFeedback(0.7);    // High feedback for climbing shimmer
    crystals.setReverse(true);    // Enable reverse pitch-shifting
    crystals.setRandomization(0.8); // High randomization for cloud-like wash
    crystals.setDryWet(0.5);      // 50/50 Dry/Wet mix

    adsr.setSampleRate(fs);
    adsr.setAttackTime(10.0);
    adsr.setDecayTime(50.0);
    adsr.setSustainLevel(0.7);
    adsr.setReleaseTime(100.0);
    
    // Initialize all (this snaps current values to target values)
    if (!carrier.init() || !modulator.init() || !drive.init() || !vca.init() || 
        !panner.init() || !filter.init() || !adsr.init() || !crystals.init()) {
        std::cerr << "Failed to initialize components" << std::endl;
        return 1;
    }
    
    std::cout << "\n--- Demonstrating FM Synthesis with Filter Sweep and Cloud Shimmer ---" << std::endl;
    std::cout << "\nConfiguration:" << std::endl;
    std::cout << "  Carrier: 440Hz Sine" << std::endl;
    std::cout << "  Modulator: 880Hz Sine" << std::endl;
    std::cout << "  FM Depth: 500Hz" << std::endl;
    std::cout << "  Crystals Delay: Pitch 2.0, Reverse: ON, Randomization: 0.8 (Cloud), Feedback 0.7" << std::endl;
    std::cout << "  Overdrive (Pre-Filter): Drive 8.0, Output Gain 1.5, Bypass Fade: 500ms" << std::endl;
    std::cout << "  Filter Base Cutoff: 5000Hz, Mod Depth: 4000Hz (Inverted)" << std::endl;
    std::cout << "  Mode: Low-Pass Output, VCA Dry/Wet: 0.5" << std::endl;
    std::cout << "  Sample Rate: " << fs << " Hz" << std::endl;

    adsr.gate(true, 1.0);

    // Process enough samples to witness the 500ms fade (~22,050 samples)
    for (int i = 0; i < 25000; i++) {
        if (i == 1000) {
            std::cout << "\n--- Starting 500ms Overdrive Bypass Fade ---" << std::endl;
            drive.setBypass(true);
        }

        // 1. Process ADSR for modulation
        double env = adsr.process();

        // 2. Generate Modulator signal
        double modOut = modulator.process();

        // 3. Generate Carrier signal, modulated by modOut
        double carrierOut = carrier.process(modOut);

        // 4. Saturate the carrier signal before it enters the filter
        double saturated = drive.process(carrierOut);

        // 5. Pass the saturated signal through the Filter
        filter.process(saturated, env);
        double lpfOut = filter.getLPFOutput();

        vca.setGain(env);
        double vcaOut = vca.process(carrierOut, lpfOut);

        // 5. Panning
        double left, right;
        panner.process(vcaOut, left, right);

        // 6. Crystals Delay (Apply shimmer to panned signal)
        double outL, outR;
        crystals.process(left, right, outL, outR);

        // Print every 3000 samples or at the start of the fade to show progression
        if (i % 3000 == 0 || i == 1000) {
            std::cout << "Sample " << i << ": Mod: " << modOut 
                      << " -> Carrier: " << carrierOut 
                      << (drive.isBypassed() ? " -> [BYPASS] " : " -> Sat: ") << saturated 
                      << " -> Cloud Shimmer Out L/R: [" << outL << ", " << outR << "]" << std::endl;
        }
    }

    // Cleanup
    carrier.cleanup();
    modulator.cleanup();
    vca.cleanup();
    drive.cleanup();
    panner.cleanup();
    filter.cleanup();
    adsr.cleanup();
    crystals.cleanup();
    
    std::cout << "\nDone." << std::endl;
    
    return 0;
}