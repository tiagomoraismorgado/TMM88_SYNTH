#ifndef ALSA_AUDIO_DEVICE_H
#define ALSA_AUDIO_DEVICE_H

#include <alsa/asoundlib.h>
#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <vector> // Keep for internal buffer
#include "IAudioDevice.h" // Include the interface

class AlsaAudioDevice : public IAudioDevice { // Inherit from IAudioDevice
public:
    AlsaAudioDevice();
    ~AlsaAudioDevice();

    bool init(const std::string& deviceName = "default") override;
    void start(AudioCallback callback) override;
    void stop() override;

    unsigned int getSampleRate() const override { return m_sampleRate; }
    int getBufferSize() const override { return m_periodSize; } // ALSA period size is the effective buffer size per callback

private:
    void audioThreadLoop();

    snd_pcm_t* m_pcmHandle;
    std::string m_deviceName;
    unsigned int m_sampleRate;
    snd_pcm_uframes_t m_bufferSize;
    snd_pcm_uframes_t m_periodSize;

    std::thread m_thread;
    std::atomic<bool> m_running;
    AudioCallback m_callback;

    std::vector<float> m_interleavedBuffer;
};

#endif // ALSA_AUDIO_DEVICE_H