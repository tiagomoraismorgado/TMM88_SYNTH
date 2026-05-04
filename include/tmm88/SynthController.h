#ifndef SYNTH_CONTROLLER_H
#define SYNTH_CONTROLLER_H

#include <QObject>
#include <atomic>
#include <thread>
#include <memory>
#include <QStringList>
#include <QVariantList>
#include <QUrl>
#include "dsp.h"
#include "IAudioDevice.h"

static constexpr int MAX_POLYPHONY = 4; // Number of voices for SIMD processing

class SynthController : public QObject {
    Q_OBJECT
    Q_PROPERTY(double cutoff READ cutoff WRITE setCutoff NOTIFY cutoffChanged)
    Q_PROPERTY(double frequency READ frequency WRITE setFrequency NOTIFY frequencyChanged)
    Q_PROPERTY(double gain READ gain WRITE setGain NOTIFY gainChanged)
    Q_PROPERTY(double drive READ drive WRITE setDrive NOTIFY driveChanged)
    Q_PROPERTY(double morph READ morph WRITE setMorph NOTIFY morphChanged)
    Q_PROPERTY(double absorption READ absorption WRITE setAbsorption NOTIFY absorptionChanged)
    Q_PROPERTY(int roomPreset READ roomPreset WRITE setRoomPreset NOTIFY roomPresetChanged)
    Q_PROPERTY(double crystalsPitch READ crystalsPitch WRITE setCrystalsPitch NOTIFY crystalsPitchChanged)
    Q_PROPERTY(double crystalsDelay READ crystalsDelay WRITE setCrystalsDelay NOTIFY crystalsDelayChanged)
    Q_PROPERTY(double crystalsFeedback READ crystalsFeedback WRITE setCrystalsFeedback NOTIFY crystalsFeedbackChanged)
    Q_PROPERTY(double crystalsRandom READ crystalsRandom WRITE setCrystalsRandom NOTIFY crystalsRandomChanged)
    Q_PROPERTY(bool crystalsReverse READ crystalsReverse WRITE setCrystalsReverse NOTIFY crystalsReverseChanged)
    Q_PROPERTY(double crystalsMix READ crystalsMix WRITE setCrystalsMix NOTIFY crystalsMixChanged)
    
    enum AudioBackendType {
        None = 0,
        ASIO,
        COREAUDIO,
        ALSA,
        JACK
    };
    Q_PROPERTY(AudioBackendType audioBackendType READ audioBackendType WRITE setAudioBackendType NOTIFY audioBackendTypeChanged)
    Q_PROPERTY(double lfoRate READ lfoRate WRITE setLfoRate NOTIFY lfoRateChanged)
    Q_PROPERTY(double lfoToCutoff READ lfoToCutoff WRITE setLfoToCutoff NOTIFY lfoToCutoffChanged)
    Q_PROPERTY(double lfoToScan READ lfoToScan WRITE setLfoToScan NOTIFY lfoToScanChanged)
    Q_PROPERTY(QVariantList modulationMatrix READ modulationMatrix NOTIFY modulationMatrixChanged)

public:
    explicit SynthController(QObject *parent = nullptr);
    ~SynthController();

    enum ModSource {
        SourceADSR = static_cast<int>(ModulationSource::ADSR),
        SourceLFO = static_cast<int>(ModulationSource::LFO)
    };
    Q_ENUM(ModSource)

    enum ModDest {
        DestFilterCutoff = static_cast<int>(ModulationDestination::EventideFilterCutoff)
    };
    Q_ENUM(ModDest)

    Q_ENUM(AudioBackendType)
    Q_INVOKABLE QStringList getAvailableAudioBackends() const;

    double cutoff() const { return m_cutoff.load(); }
    void setCutoff(double v);

    double frequency() const { return m_frequency.load(); }
    void setFrequency(double v);

    double gain() const { return m_gain.load(); }
    void setGain(double v);

    double drive() const { return m_drive.load(); }
    void setDrive(double v);

    double morph() const { return m_morph.load(); }
    void setMorph(double v);

    double absorption() const { return m_absorption.load(); }
    void setAbsorption(double v);

    int roomPreset() const { return m_roomPreset.load(); }
    void setRoomPreset(int v);

    double crystalsPitch() const { return m_crystalsPitch.load(); }
    void setCrystalsPitch(double v);

    double crystalsDelay() const { return m_crystalsDelay.load(); }
    void setCrystalsDelay(double v);

    double crystalsFeedback() const { return m_crystalsFeedback.load(); }
    void setCrystalsFeedback(double v);

    double crystalsRandom() const { return m_crystalsRandom.load(); }
    void setCrystalsRandom(double v);

    bool crystalsReverse() const { return m_crystalsReverse.load(); }
    void setCrystalsReverse(bool v);

    double crystalsMix() const { return m_crystalsMix.load(); }
    void setCrystalsMix(double v);

    double lfoRate() const { return m_lfoRate.load(); }
    void setLfoRate(double v);

    double lfoToCutoff() const { return m_lfoToCutoff.load(); }
    void setLfoToCutoff(double v);

    double lfoToScan() const { return m_lfoToScan.load(); }
    void setLfoToScan(double v);

    AudioBackendType audioBackendType() const { return m_audioBackendType.load(); }

    QVariantList modulationMatrix() const;
    Q_INVOKABLE void addModulationConnection(int source, int destination, double amount);
    Q_INVOKABLE void removeModulationConnection(int index);
    Q_INVOKABLE void setAudioBackendType(AudioBackendType type);

    Q_INVOKABLE void loadWavetable(const QUrl &fileUrl);

    Q_INVOKABLE void noteOn(int midiNote, double velocity);
    Q_INVOKABLE void noteOff(int midiNote);

signals:
    void cutoffChanged();
    void frequencyChanged();
    void gainChanged();
    void driveChanged();
    void morphChanged();
    void absorptionChanged();
    void roomPresetChanged();
    void crystalsPitchChanged();
    void crystalsDelayChanged();
    void crystalsFeedbackChanged();
    void crystalsRandomChanged();
    void crystalsReverseChanged();
    void crystalsMixChanged();
    void audioBackendTypeChanged();
    void lfoRateChanged();
    void lfoToCutoffChanged();
    void modulationMatrixChanged();
    void lfoToScanChanged();

private:
    void audioThreadLoop(); // Renamed to avoid confusion with the callback
    void audioProcessingCallback(float** input, float** output, long numSamples);
    
    std::atomic<bool> m_running{true};
    std::thread m_audioThread;

    // Voice management
    struct Voice {
        Oscillator osc{WaveformType::Sawtooth};
        ADSR adsr;
        VCA vca;
        bool active = false;
        int midiNote = 0;
        double velocity = 0.0;
        // Add other per-voice parameters here if needed
    };
    std::array<Voice, MAX_POLYPHONY> m_voices;
    std::atomic<int> m_activeVoicesCount{0};

    std::unique_ptr<IAudioDevice> m_currentAudioDevice;

    // Global DSP Components (or those that process vectorized data)
    EventideFilter m_filter;
    VSSProcessor m_reverb;
    CrystalsDelay m_crystals;

    // Atomic parameters for thread-safety
    std::atomic<double> m_cutoff{1000.0};
    std::atomic<double> m_frequency{440.0};
    std::atomic<double> m_gain{0.7};
    std::atomic<double> m_drive{1.0};
    std::atomic<double> m_morph{0.0};
    std::atomic<double> m_absorption{0.5};
    std::atomic<int> m_roomPreset{static_cast<int>(VSSRoomPreset::MediumHall)};

    std::atomic<AudioBackendType> m_audioBackendType{AudioBackendType::None};
    std::atomic<double> m_lfoRate{1.0};
    std::atomic<double> m_lfoToCutoff{0.0};
    std::atomic<double> m_lfoToScan{0.0};
    double m_lfoPhase{0.0};

    std::vector<ModulationConnection> m_modMatrix;

    std::atomic<double> m_crystalsPitch{2.0};
    std::atomic<double> m_crystalsDelay{200.0};
    std::atomic<double> m_crystalsFeedback{0.5};
    std::atomic<double> m_crystalsRandom{0.5};
    std::atomic<bool> m_crystalsReverse{false};
    std::atomic<double> m_crystalsMix{0.5};
};

#endif // SYNTH_CONTROLLER_H