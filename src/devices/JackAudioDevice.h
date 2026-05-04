#ifndef JACK_AUDIO_DEVICE_H
#define JACK_AUDIO_DEVICE_H

#include <jack/jack.h>
#include <string>
#include <functional>
#include "IAudioDevice.h" // Include the interface

class JackAudioDevice : public IAudioDevice { // Inherit from IAudioDevice
public:
    JackAudioDevice();
    ~JackAudioDevice();

    bool init(const std::string& clientName = "DSPFramework") override;
    void start(AudioCallback callback) override;
    void stop() override;

    unsigned int getSampleRate() const override;
    int getBufferSize() const override;

private:
    static int processCallback(jack_nframes_t nframes, void* arg);
    static void shutdownCallback(void* arg);

    jack_client_t* m_client;
    jack_port_t* m_inputPorts[2];
    jack_port_t* m_outputPorts[2];
    
    AudioCallback m_callback;
};

#endif // JACK_AUDIO_DEVICE_H