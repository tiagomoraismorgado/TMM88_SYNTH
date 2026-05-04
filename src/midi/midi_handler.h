#ifndef MIDI_HANDLER_H
#define MIDI_HANDLER_H

#include <cstdint>
#include <cmath>

enum class MidiStatus : uint8_t {
    NoteOff = 0x80,
    NoteOn = 0x90,
    ControlChange = 0xB0,
    PitchBend = 0xE0
};

struct MidiMessage {
    MidiStatus type;
    uint8_t channel;
    uint8_t data1;
    uint8_t data2;

    float getVelocityNormalized() const { return data2 / 127.0f; }
    float getCCValueNormalized() const { return data2 / 127.0f; }
};

class MidiHandler {
public:
    MidiHandler() = default;

    // Converts MIDI note number (0-127) to frequency (Hz)
    static double midiNoteToFrequency(uint8_t noteNumber) {
        return 440.0 * std::pow(2.0, (noteNumber - 69.0) / 12.0);
    }

    // Normalizes Pitch Bend (-8192 to 8191) to range [-1.0, 1.0]
    static float normalizePitchBend(uint8_t lsb, uint8_t msb) {
        int value = (msb << 7) | lsb;
        return (value - 8192) / 8192.0f;
    }

    bool parseMessage(const uint8_t* bytes, MidiMessage& outMsg);
};

#endif // MIDI_HANDLER_H