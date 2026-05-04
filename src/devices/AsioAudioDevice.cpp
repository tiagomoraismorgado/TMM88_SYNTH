#include "AsioAudioDevice.h"
#include <algorithm>

AsioAudioDevice* AsioAudioDevice::s_instance = nullptr;

AsioAudioDevice::AsioAudioDevice() : m_initialized(false), m_bufferSize(0) {
    s_instance = this;
}

AsioAudioDevice::~AsioAudioDevice() {
    stop();
    if (m_initialized) {
        ASIOExit();
    }
}

bool AsioAudioDevice::init(const std::string& driverName) {
    // Load the specified driver, or the default if driverName is empty
    if (ASIOInit(driverName.empty() ? nullptr : (LPTSTR)driverName.c_str()) != ASE_OK) return false;
    
    ASIOGetChannels(&m_inputChannels, &m_outputChannels);
    long minSize, maxSize, preferredSize, granularity;
    ASIOGetBufferSize(&minSize, &maxSize, &preferredSize, &granularity);
    m_bufferSize = preferredSize;

    ASIOSampleRate currentRate;
    ASIOGetSampleRate(&currentRate);
    m_sampleRate = static_cast<unsigned int>(currentRate);

    for (int i = 0; i < 2; i++) {
        ASIOBufferInfo info;
        info.isInput = ASIOFalse;
        info.channelNum = i;
        info.buffers[0] = info.buffers[1] = nullptr;
        m_bufferInfos.push_back(info);
    }

    static ASIOCallbacks callbacks;
    callbacks.bufferSwitch = &AsioAudioDevice::bufferSwitch;
    callbacks.sampleRateDidChange = &AsioAudioDevice::sampleRateChanged;
    callbacks.asioMessage = &AsioAudioDevice::asioMessages;
    callbacks.bufferSwitchTimeInfo = &AsioAudioDevice::bufferSwitchTimeInfo;

    if (ASIOCreateBuffers(m_bufferInfos.data(), 2, m_bufferSize, &callbacks) != ASE_OK) return false;
    m_initialized = true;
    return true;
}

void AsioAudioDevice::start(AudioCallback callback) {
    m_callback = callback;
    ASIOStart();
}

void AsioAudioDevice::stop() {
    ASIOStop();
}

void AsioAudioDevice::bufferSwitch(long index, ASIOBool processNow) {
    if (!s_instance || !s_instance->m_callback) return;

    // Prepare output pointers for the callback
    for (int i = 0; i < 2; i++) {
        s_instance->m_outputs[i] = (float*)s_instance->m_bufferInfos[i].buffers[index];
    }

    // For ASIO, we assume 2 input and 2 output channels for simplicity in this example
    // In a real application, you'd check m_inputChannels and m_outputChannels
    // and allocate/map buffers accordingly.
    // For now, we pass null for inputs as the DSP framework doesn't currently use them.
    s_instance->m_callback(s_instance->m_inputs, s_instance->m_outputs, s_instance->m_bufferSize);
    ASIOOutputReady();
}

// Remaining ASIO callback stubs...
ASIOTime* AsioAudioDevice::bufferSwitchTimeInfo(ASIOTime* timeInfo, long index, ASIOBool processNow) {
    bufferSwitch(index, processNow);
    return timeInfo;
}
void AsioAudioDevice::sampleRateChanged(ASIOSampleRate sRate) {
    // Handle sample rate change, e.g., reinitialize buffers, notify DSP components
    s_instance->m_sampleRate = static_cast<unsigned int>(sRate);
}
long AsioAudioDevice::asioMessages(long selector, long value, void* message, double* opt) { return 0; }

unsigned int AsioAudioDevice::getSampleRate() const {
    return m_sampleRate;
}

int AsioAudioDevice::getBufferSize() const {
    return m_bufferSize;
}