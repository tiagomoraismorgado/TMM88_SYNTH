#include "CoreAudioDevice.h"
#include <iostream>

CoreAudioDevice::CoreAudioDevice() 
    : m_audioUnit(nullptr), m_sampleRate(44100), m_bufferSize(512), 
      m_initialized(false), m_running(false) {}

CoreAudioDevice::~CoreAudioDevice() {
    stop();
    if (m_audioUnit) {
        AudioComponentInstanceDispose(m_audioUnit);
    }
}

bool CoreAudioDevice::init(const std::string& deviceName) {
    AudioComponentDescription desc;
    desc.componentType = kAudioUnitType_Output;
    desc.componentSubType = kAudioUnitSubType_DefaultOutput;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    desc.componentFlags = 0;
    desc.componentFlagsMask = 0;

    AudioComponent comp = AudioComponentFindNext(NULL, &desc);
    if (comp == NULL) return false;

    OSStatus status = AudioComponentInstanceNew(comp, &m_audioUnit);
    if (status != noErr) return false;

    // Configure the stream format to match the DSP framework (Stereo, Float32, Non-Interleaved)
    AudioStreamBasicDescription asbd;
    asbd.mSampleRate = m_sampleRate;
    asbd.mFormatID = kAudioFormatLinearPCM;
    asbd.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsNonInterleaved | kAudioFormatFlagIsPacked;
    asbd.mBitsPerChannel = 32;
    asbd.mChannelsPerFrame = 2;
    asbd.mFramesPerPacket = 1;
    asbd.mBytesPerFrame = 4;
    asbd.mBytesPerPacket = 4;

    status = AudioUnitSetProperty(m_audioUnit,
                                  kAudioUnitProperty_StreamFormat,
                                  kAudioUnitScope_Input,
                                  0,
                                  &asbd,
                                  sizeof(asbd));
    if (status != noErr) return false;

    // Sync with hardware sample rate
    UInt32 size = sizeof(asbd);
    AudioUnitGetProperty(m_audioUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &asbd, &size);
    m_sampleRate = (unsigned int)asbd.mSampleRate;

    // Query the maximum buffer size allowed by the system
    size = sizeof(m_bufferSize);
    AudioUnitGetProperty(m_audioUnit, kAudioUnitProperty_MaximumFramesPerSlice, kAudioUnitScope_Global, 0, &m_bufferSize, &size);

    m_initialized = true;
    return true;
}

void CoreAudioDevice::start(AudioCallback callback) {
    if (!m_initialized || m_running) return;
    m_callback = callback;

    AURenderCallbackStruct callbackStruct;
    callbackStruct.inputProc = CoreAudioDevice::renderCallback;
    callbackStruct.inputProcRefCon = this;

    OSStatus status = AudioUnitSetProperty(m_audioUnit,
                                           kAudioUnitProperty_SetRenderCallback,
                                           kAudioUnitScope_Input,
                                           0,
                                           &callbackStruct,
                                           sizeof(callbackStruct));
    if (status != noErr) return;

    status = AudioUnitInitialize(m_audioUnit);
    if (status != noErr) return;

    status = AudioOutputUnitStart(m_audioUnit);
    if (status == noErr) {
        m_running = true;
    }
}

void CoreAudioDevice::stop() {
    if (m_running) {
        AudioOutputUnitStop(m_audioUnit);
        AudioUnitUninitialize(m_audioUnit);
        m_running = false;
    }
}

OSStatus CoreAudioDevice::renderCallback(void* inRefCon,
                                         AudioUnitRenderActionFlags* ioActionFlags,
                                         const AudioTimeStamp* inTimeStamp,
                                         UInt32 inBusNumber,
                                         UInt32 inNumberFrames,
                                         AudioBufferList* ioData) {
    auto* device = static_cast<CoreAudioDevice*>(inRefCon);
    if (!device->m_callback) return noErr;

    // Map Core Audio de-interleaved buffers to IAudioDevice float** format
    float* outputs[2];
    outputs[0] = static_cast<float*>(ioData->mBuffers[0].mData);
    outputs[1] = static_cast<float*>(ioData->mBuffers[1].mData);

    // Pass null for inputs as the framework currently handles synthesis
    float* inputs[2] = { nullptr, nullptr };

    device->m_callback(inputs, outputs, (long)inNumberFrames);

    return noErr;
}