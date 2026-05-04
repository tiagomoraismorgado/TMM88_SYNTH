#include "midi_handler.h"

bool MidiHandler::parseMessage(const uint8_t* bytes, MidiMessage& outMsg) {
    if (!bytes) return false;

    uint8_t status = bytes[0];
    uint8_t type = status & 0xF0;
    
    outMsg.channel = status & 0x0F;
    outMsg.data1 = bytes[1];
    outMsg.data2 = bytes[2];

    switch (type) {
        case (uint8_t)MidiStatus::NoteOn:
            // MIDI Convention: Note On with Velocity 0 is actually Note Off
            outMsg.type = (outMsg.data2 == 0) ? MidiStatus::NoteOff : MidiStatus::NoteOn;
            return true;
        case (uint8_t)MidiStatus::NoteOff:
        case (uint8_t)MidiStatus::ControlChange:
        case (uint8_t)MidiStatus::PitchBend:
            outMsg.type = (MidiStatus)type;
            return true;
    }
    return false;
}