#ifndef CORE_AUDIO_DEVICE_H
#define CORE_AUDIO_DEVICE_H

#include <AudioUnit/AudioUnit.h>
#include <string>
#include "IAudioDevice.h"

class CoreAudioDevice : public IAudioDevice {
public:
    CoreAudioDevice();
    ~CoreAudioDevice();

    bool init(const std::string& deviceName = "") override;
    void start(AudioCallback callback) override;
    void stop() override;

    unsigned int getSampleRate() const override { return m_sampleRate; }
    int getBufferSize() const override { return (int)m_bufferSize; }

private:
    static OSStatus renderCallback(void* inRefCon,
                                   AudioUnitRenderActionFlags* ioActionFlags,
                                   const AudioTimeStamp* inTimeStamp,
                                   UInt32 inBusNumber,
                                   UInt32 inNumberFrames,
                                   AudioBufferList* ioData);

    AudioComponentInstance m_audioUnit;
    AudioCallback m_callback;
    unsigned int m_sampleRate;
    UInt32 m_bufferSize;
    bool m_initialized;
    bool m_running;
};

#endif // CORE_AUDIO_DEVICE_H