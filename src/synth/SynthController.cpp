#include "SynthController.h"
#include <iostream>
#include <fstream>
#include <QCoreApplication> // For platform detection
#include <QFileInfo>

SynthController::SynthController(QObject *parent) : QObject(parent) { // Assuming this file is now in src/app/
    const double defaultSampleRate = 44100.0;
    m_filter.setSampleRate(defaultSampleRate);
    m_reverb.setSampleRate(defaultSampleRate);
    m_crystals.setSampleRate(defaultSampleRate);
    
    m_filter.init();
    m_reverb.init();
    m_crystals.init();

    // Initialize per-voice DSP components
    for (int i = 0; i < MAX_POLYPHONY; ++i) {
        m_voices[i].osc.setSampleRate(defaultSampleRate);
        m_voices[i].adsr.setSampleRate(defaultSampleRate);
        m_voices[i].vca.setSmoothingTime(0.0, defaultSampleRate); // Instant VCA response for ADSR

        m_voices[i].osc.init();
        m_voices[i].adsr.init();
        m_voices[i].vca.init();
        m_voices[i].active = false;
        m_voices[i].midiNote = 0;
        m_voices[i].velocity = 0.0;
    }

    // Initialize audio backend based on platform
#ifdef Q_OS_WIN
    m_audioBackendType = ASIO;
    m_currentAudioDevice = std::make_unique<AsioAudioDevice>();
    if (!m_currentAudioDevice->init("")) { // Empty string for default ASIO driver
        std::cerr << "Failed to initialize default ASIO device." << std::endl;
        m_currentAudioDevice.reset(); // Clear device if init fails
        m_audioBackendType = None;
    }
#elif defined(Q_OS_MACOS)
    m_audioBackendType = COREAUDIO;
    m_currentAudioDevice = std::make_unique<CoreAudioDevice>();
    if (!m_currentAudioDevice->init("")) {
        std::cerr << "Failed to initialize CoreAudio device." << std::endl;
        m_currentAudioDevice.reset();
        m_audioBackendType = None;
    }
#elif defined(Q_OS_LINUX)
    // On Linux, try JACK first, then ALSA
    m_audioBackendType = JACK;
    m_currentAudioDevice = std::make_unique<JackAudioDevice>();
    if (!m_currentAudioDevice->init("DSPFramework")) {
        std::cerr << "Failed to initialize JACK device. Trying ALSA..." << std::endl;
        m_audioBackendType = ALSA;
        m_currentAudioDevice = std::make_unique<AlsaAudioDevice>();
        if (!m_currentAudioDevice->init("default")) {
            std::cerr << "Failed to initialize ALSA device. No audio backend available." << std::endl;
            m_currentAudioDevice.reset();
            m_audioBackendType = None;
        }
    }
#else
    m_audioBackendType = None;
    std::cerr << "Unsupported operating system for audio backend." << std::endl;
#endif

    // Start the audio thread if a device was successfully initialized
    if (m_currentAudioDevice) {
        // Update DSP components with actual sample rate from the device
        const double fs = m_currentAudioDevice->getSampleRate();
        m_filter.setSampleRate(fs);
        m_reverb.setSampleRate(fs);
        m_crystals.setSampleRate(fs);
        for (int i = 0; i < MAX_POLYPHONY; ++i) {
            m_voices[i].osc.setSampleRate(fs); m_voices[i].adsr.setSampleRate(fs); m_voices[i].vca.setSmoothingTime(0.0, fs);
        }

        m_audioThread = std::thread(&SynthController::audioThreadLoop, this);
    } else {
        std::cerr << "No audio device initialized. Audio playback will not function." << std::endl;
    }
}

SynthController::~SynthController() {
    m_running = false;
    if (m_audioThread.joinable()) m_audioThread.join();
    if (m_currentAudioDevice) {
        m_currentAudioDevice->stop();
    }
}

void SynthController::setCutoff(double v) {
    if (m_cutoff != v) {
        m_cutoff = v;
        emit cutoffChanged();
    }
}

void SynthController::setFrequency(double v) {
    if (m_frequency != v) {
        m_frequency = v;
        emit frequencyChanged();
    }
}

void SynthController::setGain(double v) {
    if (m_gain != v) {
        m_gain = v;
        emit gainChanged();
    }
}

void SynthController::setDrive(double v) {
    if (m_drive != v) {
        m_drive = v;
        emit driveChanged();
    }
}

void SynthController::setMorph(double v) {
    if (m_morph != v) {
        m_morph = v;
        emit morphChanged();
    }
}

void SynthController::setAbsorption(double v) {
    if (m_absorption != v) {
        m_absorption = v;
        emit absorptionChanged();
    }
}

void SynthController::setRoomPreset(int v) {
    if (m_roomPreset != v) {
        m_roomPreset = v;
        emit roomPresetChanged();
    }
}

void SynthController::setCrystalsPitch(double v) {
    if (m_crystalsPitch != v) {
        m_crystalsPitch = v;
        emit crystalsPitchChanged();
    }
}

void SynthController::setCrystalsDelay(double v) {
    if (m_crystalsDelay != v) {
        m_crystalsDelay = v;
        emit crystalsDelayChanged();
    }
}

void SynthController::setCrystalsFeedback(double v) {
    if (m_crystalsFeedback != v) {
        m_crystalsFeedback = v;
        emit crystalsFeedbackChanged();
    }
}

void SynthController::setCrystalsRandom(double v) {
    if (m_crystalsRandom != v) {
        m_crystalsRandom = v;
        emit crystalsRandomChanged();
    }
}

void SynthController::setCrystalsReverse(bool v) {
    if (m_crystalsReverse != v) {
        m_crystalsReverse = v;
        emit crystalsReverseChanged();
    }
}

void SynthController::setCrystalsMix(double v) {
    if (m_crystalsMix != v) {
        m_crystalsMix = v;
        emit crystalsMixChanged();
    }
}

void SynthController::setLfoRate(double v) {
    if (m_lfoRate != v) {
        m_lfoRate = v;
        emit lfoRateChanged();
    }
}

void SynthController::setLfoToCutoff(double v) {
    if (m_lfoToCutoff != v) {
        m_lfoToCutoff = v;
        emit lfoToCutoffChanged();
    }
}

void SynthController::setLfoToScan(double v) {
    if (m_lfoToScan != v) {
        m_lfoToScan = v;
        emit lfoToScanChanged();
    }
}

void SynthController::noteOn(int midiNote, double velocity) {
    // Convert MIDI note to frequency
    double freq = 440.0 * std::pow(2.0, (midiNote - 69) / 12.0);

    // Find an inactive voice or steal one
    int voiceIdx = -1;
    for (int i = 0; i < MAX_POLYPHONY; ++i) {
        if (!m_voices[i].active || m_voices[i].adsr.getState() == ADSRState::Idle) {
            voiceIdx = i;
            break;
        }
    }

    // If all voices are active, steal the oldest one (or one in release phase)
    if (voiceIdx == -1) {
        // Simple stealing: find the first voice that is not in attack/decay
        for (int i = 0; i < MAX_POLYPHONY; ++i) {
            if (m_voices[i].adsr.getState() == ADSRState::Release || m_voices[i].adsr.getState() == ADSRState::Sustain) {
                voiceIdx = i;
                break;
            }
        }
        if (voiceIdx == -1) voiceIdx = 0; // Fallback to stealing voice 0
    }

    m_voices[voiceIdx].active = true;
    m_voices[voiceIdx].midiNote = midiNote;
    m_voices[voiceIdx].velocity = velocity;
    m_voices[voiceIdx].osc.setFrequency(freq);
    m_voices[voiceIdx].adsr.gate(true, velocity);
    m_voices[voiceIdx].vca.setGain(1.0); // Reset VCA gain for new note
}

void SynthController::noteOff(int midiNote) {
    for (int i = 0; i < MAX_POLYPHONY; ++i) {
        if (m_voices[i].active && m_voices[i].midiNote == midiNote) {
            m_voices[i].adsr.gate(false);
            // Voice remains active until ADSR enters Idle state
            break;
        }
    }
}

QVariantList SynthController::modulationMatrix() const {
    QVariantList list;
    for (const auto& conn : m_modMatrix) {
        QVariantMap map;
        map["source"] = static_cast<int>(conn.source);
        map["destination"] = static_cast<int>(conn.destination);
        map["amount"] = conn.amount;
        list.append(map);
    }
    return list;
}

void SynthController::addModulationConnection(int source, int destination, double amount) {
    // In a real application, you'd want to check for duplicates or allow editing existing connections.
    // For simplicity, we just add it.
    m_modMatrix.push_back({static_cast<ModulationSource>(source), 
                           static_cast<ModulationDestination>(destination), 
                           amount});
    emit modulationMatrixChanged();
}

void SynthController::removeModulationConnection(int index) {
    if (index >= 0 && index < static_cast<int>(m_modMatrix.size())) {
        m_modMatrix.erase(m_modMatrix.begin() + index);
        emit modulationMatrixChanged();
    }
}

void SynthController::audioThreadLoop() {
    if (m_currentAudioDevice) {
        m_currentAudioDevice->start([this](float** input, float** output, long numSamples) {
            this->audioProcessingCallback(input, output, numSamples);
        });
    }

    // The audio device's internal thread will call audioProcessingCallback.
    // This thread (audioThreadLoop) now just keeps the SynthController alive
    // and can be used for other non-realtime tasks if needed, or simply join.
    while (m_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void SynthController::audioProcessingCallback(float** input, float** output, long numSamples) {
    if (!m_currentAudioDevice) return;

    const double sampleRate = m_currentAudioDevice->getSampleRate();

    for (long i = 0; i < numSamples; ++i) {
        // Global LFO phase update
        m_lfoPhase += (2.0 * M_PI * m_lfoRate.load()) / sampleRate;
        if (m_lfoPhase >= 2.0 * M_PI) m_lfoPhase -= 2.0 * M_PI;
        double lfoVal = std::sin(m_lfoPhase);
        __m256d lfoVal_v = _mm256_set1_pd(lfoVal);

        // Prepare vectorized inputs for EventideFilter::process_simd
        __m256d osc_outputs_v = _mm256_setzero_pd();
        __m256d per_voice_mod_signals_v = _mm256_setzero_pd();
        __m256d vca_gains_v = _mm256_setzero_pd();
        __m256d target_cutoff_v = _mm256_set1_pd(m_cutoff.load());
        __m256d target_resonance_v = _mm256_set1_pd(m_filter.getResonance()); // Assuming filter resonance is global

        double morph = m_morph.load();
        __m256d morph_v = _mm256_set1_pd(morph);
        __m256d lfoToScan_v = _mm256_set1_pd(m_lfoToScan.load());
        __m256d global_gain_v = _mm256_set1_pd(m_gain.load());
        __m256d lfoToCutoff_v = _mm256_set1_pd(m_lfoToCutoff.load());

        // Process each voice
        for (int j = 0; j < MAX_POLYPHONY; ++j) {
            if (m_voices[j].active) {
                // Process Oscillator and ADSR for this voice
                double osc_out = m_voices[j].osc.process();
                double adsr_out = m_voices[j].adsr.process();

                // If ADSR is idle, deactivate the voice
                if (m_voices[j].adsr.getState() == ADSRState::Idle) {
                    m_voices[j].active = false;
                    // Ensure this voice's lane is zeroed out in vectors
                    ((double*)&osc_outputs_v)[j] = 0.0;
                    ((double*)&per_voice_mod_signals_v)[j] = 0.0;
                    ((double*)&vca_gains_v)[j] = 0.0;
                    continue;
                }

                // Calculate per-voice modulation signal for the filter cutoff
                double per_voice_mod_signal = 0.0;
                for (const auto& conn : m_modMatrix) {
                    if (conn.destination == ModulationDestination::EventideFilterCutoff) {
                        double srcVal = (conn.source == ModulationSource::ADSR) ? adsr_out : lfoVal;
                        per_voice_mod_signal += srcVal * conn.amount;
                    }
                }
                // Add direct LFO to cutoff (if still used outside mod matrix)
                per_voice_mod_signal += lfoVal * m_lfoToCutoff.load();

                // Store per-voice outputs and mod signals into SIMD vectors
                ((double*)&osc_outputs_v)[j] = osc_out;
                ((double*)&per_voice_mod_signals_v)[j] = per_voice_mod_signal;

                // Set per-voice VCA gain (ADSR output * global gain)
                ((double*)&vca_gains_v)[j] = adsr_out * m_gain.load();

                // Update per-voice oscillator shape (wavetable scan)
                m_voices[j].osc.setShape(morph + (lfoVal * m_lfoToScan.load()));
            } else {
                // Inactive voice, ensure its lane is zeroed
                ((double*)&osc_outputs_v)[j] = 0.0;
                ((double*)&per_voice_mod_signals_v)[j] = 0.0;
                ((double*)&vca_gains_v)[j] = 0.0;
            }
        }

        // Process the EventideFilter for all 4 voices in parallel
        // The filter's targetCutoff and targetResonance are now global for all voices,
        // but the modSignal_v allows per-voice modulation.
        __m256d filtered_voices_v = m_filter.process_simd(osc_outputs_v, target_cutoff_v, target_resonance_v, per_voice_mod_signals_v);

        // Apply per-voice VCA and sum all voices to a single mono output
        double mixed_poly_output_scalar = 0.0;
        for (int j = 0; j < MAX_POLYPHONY; ++j) {
            double vca_out = ((double*)&filtered_voices_v)[j] * ((double*)&vca_gains_v)[j];
            mixed_poly_output_scalar += vca_out;
        }

        // Update global effects parameters (these are still scalar)
        m_filter.setDrive(m_drive.load());
        m_reverb.setPreset(static_cast<VSSRoomPreset>(m_roomPreset.load()));
        m_reverb.setAbsorption(m_absorption.load());
        m_crystals.setRandomization(m_crystalsRandom.load());
        m_crystals.setReverse(m_crystalsReverse.load());
        m_crystals.setDryWet(m_crystalsMix.load());
        m_crystals.setPitch(m_crystalsPitch.load());
        m_crystals.setDelayTime(m_crystalsDelay.load());
        m_crystals.setFeedback(m_crystalsFeedback.load());

        // TC M3000 Hybrid: Parallel Engine Routing
        // Engine 1: The Synth Path
        double engine1 = mixed_poly_output_scalar; // This is the summed output of all voices
        
        // Engine 2: VSS3 Space Simulation
        double revL, revR;
        m_reverb.process(engine1, engine1, revL, revR);

        // Engine 3: Crystals Delay
        double crystL, crystR;
        m_crystals.process(engine1, engine1, crystL, crystR);

        // Mix reverb and crystals (simple sum for now)
        double finalL = engine1 + revL + crystL;
        double finalR = engine1 + revR + crystR;

        // Final output
        output[0][i] = static_cast<float>(finalL);
        output[1][i] = static_cast<float>(finalR);
    }
}

QStringList SynthController::getAvailableAudioBackends() const {
    QStringList backends;
#ifdef Q_OS_WIN
    backends << "ASIO";
#elif defined(Q_OS_LINUX)
    backends << "ALSA" << "JACK";
#elif defined(Q_OS_MACOS)
    backends << "CoreAudio";
#endif
    return backends;
}

void SynthController::setAudioBackendType(AudioBackendType type) {
    if (m_audioBackendType.load() == type) return;

    if (m_currentAudioDevice) {
        m_currentAudioDevice->stop();
        m_currentAudioDevice.reset();
    }

    const double fs = 44100.0; // Default sample rate for new device
    if (type == ASIO) {
        m_currentAudioDevice = std::make_unique<AsioAudioDevice>();
        m_currentAudioDevice->init("");
    } else if (type == COREAUDIO) {
        m_currentAudioDevice = std::make_unique<CoreAudioDevice>();
        m_currentAudioDevice->init("");
    } else if (type == ALSA) {
        m_currentAudioDevice = std::make_unique<AlsaAudioDevice>();
        m_currentAudioDevice->init("default");
    } else if (type == JACK) {
        m_currentAudioDevice = std::make_unique<JackAudioDevice>();
        m_currentAudioDevice->init("DSPFramework");
    }

    if (m_currentAudioDevice) {
        m_filter.setSampleRate(m_currentAudioDevice->getSampleRate());
        m_reverb.setSampleRate(m_currentAudioDevice->getSampleRate());
        m_crystals.setSampleRate(m_currentAudioDevice->getSampleRate());
        for (int i = 0; i < MAX_POLYPHONY; ++i) {
            m_voices[i].osc.setSampleRate(m_currentAudioDevice->getSampleRate()); m_voices[i].adsr.setSampleRate(m_currentAudioDevice->getSampleRate()); m_voices[i].vca.setSmoothingTime(0.0, m_currentAudioDevice->getSampleRate());
        }
        m_currentAudioDevice->start([this](float** input, float** output, long numSamples) {
            this->audioProcessingCallback(input, output, numSamples);
        });
        m_audioBackendType = type;
        emit audioBackendTypeChanged();
    } else {
        std::cerr << "Failed to initialize selected audio backend." << std::endl;
        m_audioBackendType = None;
        emit audioBackendTypeChanged();
    }
}

void SynthController::loadWavetable(const QUrl &fileUrl) {
    QString path = fileUrl.toLocalFile();
#ifdef Q_OS_WIN
    // Handle potential QUrl formatting issues on Windows
    if (path.startsWith("/")) path.remove(0, 1);
#endif

    std::ifstream file(path.toStdString(), std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open WAV file: " << path.toStdString() << std::endl;
        return;
    }

    // Basic WAV Header Parsing
    char header[44];
    file.read(header, 44);
    
    // Check for RIFF and WAVE tags
    if (std::string(header, 4) != "RIFF" || std::string(header + 8, 4) != "WAVE") {
        std::cerr << "Invalid WAV format." << std::endl;
        return;
    }

    uint16_t channels = *reinterpret_cast<uint16_t*>(header + 22);
    uint16_t bitsPerSample = *reinterpret_cast<uint16_t*>(header + 34);
    uint16_t formatTag = *reinterpret_cast<uint16_t*>(header + 20); // 1 = PCM, 3 = Float

    // Find data chunk
    file.seekg(12); // Skip RIFF header
    while (file.good()) {
        char chunkId[4];
        uint32_t chunkSize;
        file.read(chunkId, 4);
        file.read(reinterpret_cast<char*>(&chunkSize), 4);

        if (std::string(chunkId, 4) == "data") {
            std::vector<double> table;
            size_t numSamples = chunkSize / (bitsPerSample / 8);
            table.reserve(numSamples / channels);

            if (formatTag == 1) { // PCM Integer
                for (size_t i = 0; i < numSamples; i += channels) {
                    int32_t sample = 0;
                    if (bitsPerSample == 16) {
                        int16_t s16;
                        file.read(reinterpret_cast<char*>(&s16), 2);
                        sample = s16;
                    } else if (bitsPerSample == 24) {
                        uint8_t bytes[3];
                        file.read(reinterpret_cast<char*>(bytes), 3);
                        sample = (bytes[0] << 8) | (bytes[1] << 16) | (bytes[2] << 24);
                        sample >>= 8;
                    }
                    // Sum to mono if needed and normalize
                    table.push_back(static_cast<double>(sample) / (std::pow(2, bitsPerSample - 1)));
                    
                    // Skip remaining channels
                    if (channels > 1) file.seekg((channels - 1) * (bitsPerSample / 8), std::ios::cur);
                }
            } else if (formatTag == 3) { // IEEE Float
                for (size_t i = 0; i < numSamples; i += channels) {
                    float s32;
                    file.read(reinterpret_cast<char*>(&s32), 4);
                    table.push_back(static_cast<double>(s32));
                    if (channels > 1) file.seekg((channels - 1) * 4, std::ios::cur);
                }
            }

            // Update the oscillator
            m_osc.setWavetable(table);
            m_osc.setType(WaveformType::Wavetable);
            
            std::cout << "Wavetable loaded: " << table.size() << " samples from " << path.toStdString() << std::endl;
            return;
        } else {
            file.seekg(chunkSize, std::ios::cur);
        }
    }

    file.close();
}