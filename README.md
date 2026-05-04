# TMM88 Synth

A high-performance, cross-platform DSP synthesizer framework written in modern C++17.

## Features

- **Native C++ Implementation**: No external hardware acceleration dependencies
- **Modular Architecture**: Polymorphic audio components with clean interfaces
- **Cross-Platform**: Windows, macOS, and Linux support
- **SIMD Optimization**: AVX2 intrinsics for high-performance oscillators
- **Multi-Platform Audio**: ASIO (Windows), CoreAudio (macOS), ALSA/JACK (Linux)
- **Qt6 UI Framework**: Optional graphical interface
- **Comprehensive DSP**: Oscillators, filters, effects, reverb, delay, ADSR

## Components

### Synthesis
- **Oscillator**: Sine, sawtooth, square waves with SIMD optimization
- **SubOscillator**: Octave-down generator with polyBLEP anti-aliasing
- **VCA**: Voltage-controlled amplifier with smooth parameter changes

### Filters
- **ResonantLPF**: State-variable filter with saturation modeling
- **EventideFilter**: 4-pole ladder filter with multiple modes

### Effects
- **Overdrive**: CPU-based saturation with bypass smoothing
- **ADSR**: Advanced envelope generator with looping and swing
- **CrystalsDelay**: Pitch-shifting delay with reverse and randomization
- **TC Dynamics**: Compressor/expander with RMS detection
- **VSS Reverb**: Algorithmic reverb with early reflections

### MIDI & Control
- **MIDI Handler**: Note on/off, CC, pitch bend processing
- **Spectral Delay**: FFT-based delay with frequency domain processing
- **ModMatrix**: Flexible modulation routing (framework ready)

## Building

### Prerequisites

- **C++17 Compiler**: GCC 8+, Clang 7+, MSVC 2019+
- **CMake 3.18+**
- **Qt6** (optional, for GUI): Install from qt.io or use `aqtinstall`

### Quick Start

```bash
# Clone repository
git clone https://github.com/your-repo/TMM88_SYNTH.git
cd TMM88_SYNTH

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake .. -G "MinGW Makefiles"  # Windows
# or
cmake .. -G "Unix Makefiles"    # Linux/macOS

# Build
make

# Run example
./dsp_example
```

### Qt6 GUI (Optional)

To enable the Qt6 graphical interface:

1. Install Qt6:
   ```bash
   # Using aqtinstall (recommended)
   winget install miurahr.aqtinstall
   aqt install-qt windows desktop 6.5.0 gcc_64 --outputdir C:/Qt
   ```

2. Set CMake prefix path:
   ```bash
   export CMAKE_PREFIX_PATH="/path/to/qt6"
   ```

3. Rebuild - Qt6 will be automatically detected

### Platform-Specific Audio Backends

- **Windows**: ASIO (requires Steinberg SDK), WASAPI
- **macOS**: CoreAudio
- **Linux**: ALSA, JACK

For ASIO support on Windows:
1. Download ASIO SDK from Steinberg
2. Extract to a folder
3. Set environment variable: `ASIO_SDK_PATH=/path/to/asio/sdk`

## Usage Example

The following snippet demonstrates how to set up a basic FM synthesis chain with a filter sweep using the framework:

```cpp
// Create components
Oscillator carrier(WaveformType::Sine);
Oscillator modulator(WaveformType::Sine);
ResonantLPF filter;
ADSR adsr;

// Configuration
carrier.setFrequency(440.0);
modulator.setFrequency(880.0);
filter.setCutoff(5000.0);
adsr.setAttackTime(10.0);

// Initialize
carrier.init();
modulator.init();
filter.init();
adsr.init();

// Process
double env = adsr.process();
double out = carrier.process(modulator.process());
filter.process(out, env);
```
