#ifndef I_AUDIO_DEVICE_H
#define I_AUDIO_DEVICE_H

#include <string>
#include <functional>

/**
 * @brief Interface for audio device implementations (ASIO, ALSA, JACK, CoreAudio).
 * This allows the SynthController to manage different audio backends polymorphically.
 */
class IAudioDevice {
public:
    // Callback type for audio processing: (input_buffers, output_buffers, num_samples)
    using AudioCallback = std::function<void(float** input, float** output, long numSamples)>;

    virtual ~IAudioDevice() = default;

    // Initializes the audio device with an optional device name/identifier.
    virtual bool init(const std::string& deviceName = "") = 0;

    // Starts the audio processing, providing a callback function for audio data.
    virtual void start(AudioCallback callback) = 0;

    // Stops the audio processing.
    virtual void stop() = 0;

    virtual int getNumInputChannels() const = 0;
    virtual int getNumOutputChannels() const = 0;

    virtual unsigned int getSampleRate() const = 0;
    virtual int getBufferSize() const = 0;
};

#endif // I_AUDIO_DEVICE_H