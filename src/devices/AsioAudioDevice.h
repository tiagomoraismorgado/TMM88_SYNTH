#ifndef ASIO_AUDIO_DEVICE_H
#define ASIO_AUDIO_DEVICE_H

#include <string>
#include <vector>
#include <functional>
#include "common/asiosys.h" // ASIO SDK specific
#include "common/asio.h"     // ASIO SDK specific
#include "IAudioDevice.h"    // Include the interface

class AsioAudioDevice : public IAudioDevice { // Inherit from IAudioDevice
public:
    AsioAudioDevice();
    ~AsioAudioDevice();

    bool init(const std::string& driverName = "") override; // Default to empty string for driver selection
    void start(AudioCallback callback) override;
    void stop() override;

    static void bufferSwitch(long index, ASIOBool processNow);
    static ASIOTime* bufferSwitchTimeInfo(ASIOTime* timeInfo, long index, ASIOBool processNow);
    static void sampleRateChanged(ASIOSampleRate sRate);
    static long asioMessages(long selector, long value, void* message, double* opt);

private:
    bool m_initialized;
    long m_bufferSize;
    long m_inputChannels;
    long m_outputChannels;
    std::vector<ASIOBufferInfo> m_bufferInfos;
    std::vector<ASIOChannelInfo> m_channelInfos;
    static AsioAudioDevice* s_instance;
    AudioCallback m_callback; // Use the AudioCallback from IAudioDevice
    
    float* m_outputs[2];
    float* m_inputs[2];
    unsigned int m_sampleRate; // Store sample rate
};

#endif // ASIO_AUDIO_DEVICE_H