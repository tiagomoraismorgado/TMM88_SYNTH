#ifndef DSP_FRAMEWORK_H
#define DSP_FRAMEWORK_H

#include <cmath>

/**
 * DSP Framework - Audio Synthesis Building Blocks
 * 
 * Components:
 *   - Oscillator (sin/cos)
 *   - VCA (Voltage Controlled Amplifier)
 *   - StereoPanner (dual VCA)
 */

enum class AccelerationMode {
    CPU,
    CUDA,
    OpenCL
};

// Base component interface
class AudioComponent {
public:
    virtual ~AudioComponent() = default;
    virtual bool init() = 0;
    virtual void cleanup() = 0;

    virtual void setBypass(bool bypassed) { targetBypass_ = bypassed ? 1.0 : 0.0; }
    virtual bool isBypassed() const { return targetBypass_ > 0.5; }

    virtual void setBypassSmoothingTime(double timeMs, double sampleRate) {
        if (timeMs <= 0.0) {
            bypassSmoothingFactor_ = 1.0;
        } else {
            // Calculate coefficient for exponential smoothing: 1 - exp(-1 / (fs * tau))
            bypassSmoothingFactor_ = 1.0 - std::exp(-1.0 / (sampleRate * (timeMs / 1000.0)));
        }
    }

    virtual void setAccelerationMode(AccelerationMode mode) { accelMode_ = mode; }
    
    // Block processing interface for GPU/SIMD optimization
    virtual void processBuffer(double* input, double* output, size_t size) {
        for (size_t i = 0; i < size; ++i) output[i] = input[i]; 
    }

protected:
    double targetBypass_ = 0.0;  // 0.0 = Active, 1.0 = Bypassed
    double currentBypass_ = 0.0;
    double bypassSmoothingFactor_ = 0.001; // Default smoothing speed
    AccelerationMode accelMode_ = AccelerationMode::CPU;
};

/**
 * @brief Enumeration of available modulation sources.
 */
enum class ModulationSource {
    ADSR,
    LFO
};

/**
 * @brief Enumeration of available modulation destinations.
 */
enum class ModulationDestination {
    EventideFilterCutoff
};

/**
 * @brief Fast rational approximation of tanh.
 * Approximately 4-5x faster than std::tanh by avoiding exponentials.
 * Accurate for musical saturation in the range [-3.0, 3.0].
 */
inline double fast_tanh(double x) {
    if (x <= -3.0) return -1.0;
    if (x >= 3.0) return 1.0;
    
    double x2 = x * x;
    // Padé approximant: x * (27 + x^2) / (27 + 9 * x^2)
    return x * (27.0 + x2) / (27.0 + 9.0 * x2);
}

/**
 * @brief Represents a single modulation connection in the matrix.
 */
struct ModulationConnection {
    ModulationSource source;
    ModulationDestination destination;
    double amount; // The intensity of the modulation
};


// Forward declarations
class Oscillator;
class VCA;
class StereoPanner;
class ResonantLPF;
class ADSR;
class Overdrive;
class SpectralDelay;
class SubOscillator;
class EventideFilter;
class CrystalsDelay;
class VSSProcessor;
class TCDynamics;

#endif // DSP_FRAMEWORK_H