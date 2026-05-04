#include "JackAudioDevice.h"
#include <iostream>

JackAudioDevice::JackAudioDevice() : m_client(nullptr) {
    m_inputPorts[0] = m_inputPorts[1] = nullptr;
    m_outputPorts[0] = m_outputPorts[1] = nullptr;
}

JackAudioDevice::~JackAudioDevice() {
    stop();
}

bool JackAudioDevice::init(const std::string& clientName) {
    jack_status_t status;
    // Use JackNoStartServer to prevent JACK from trying to start a server if none is running
    m_client = jack_client_open(clientName.c_str(), JackNullOption, &status);
    if (!m_client) {
        std::cerr << "JACK: Failed to open client. Status: " << status << std::endl;
        return false;
    }

    // Register stereo ports
    m_inputPorts[0] = jack_port_register(m_client, "input_L", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
    m_inputPorts[1] = jack_port_register(m_client, "input_R", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
    m_outputPorts[0] = jack_port_register(m_client, "output_L", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
    m_outputPorts[1] = jack_port_register(m_client, "output_R", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

    if (!m_inputPorts[0] || !m_inputPorts[1] || !m_outputPorts[0] || !m_outputPorts[1]) {
        std::cerr << "JACK: Failed to register ports" << std::endl;
        return false;
    }

    jack_set_process_callback(m_client, JackAudioDevice::processCallback, this);
    jack_on_shutdown(m_client, JackAudioDevice::shutdownCallback, this);

    return true;
}

void JackAudioDevice::start(AudioCallback callback) {
    if (!m_client) return;
    m_callback = callback;
    if (jack_activate(m_client) != 0) {
        std::cerr << "JACK: Failed to activate client" << std::endl;
    }
}

void JackAudioDevice::stop() {
    if (m_client) {
        jack_client_close(m_client);
        m_client = nullptr;
    }
}

unsigned int JackAudioDevice::getSampleRate() const {
    return m_client ? jack_get_sample_rate(m_client) : 44100;
}

int JackAudioDevice::getBufferSize() const {
    return m_client ? jack_get_buffer_size(m_client) : 512;
}

int JackAudioDevice::processCallback(jack_nframes_t nframes, void* arg) {
    auto* device = static_cast<JackAudioDevice*>(arg);
    if (!device->m_callback) return 0;

    float* in[2];
    float* out[2];

    in[0] = static_cast<float*>(jack_port_get_buffer(device->m_inputPorts[0], nframes));
    in[1] = static_cast<float*>(jack_port_get_buffer(device->m_inputPorts[1], nframes));
    out[0] = static_cast<float*>(jack_port_get_buffer(device->m_outputPorts[0], nframes));
    out[1] = static_cast<float*>(jack_port_get_buffer(device->m_outputPorts[1], nframes));

    device->m_callback(in, out, static_cast<long>(nframes));

    return 0;
}

void JackAudioDevice::shutdownCallback(void* arg) {
    auto* device = static_cast<JackAudioDevice*>(arg);
    device->m_client = nullptr;
}