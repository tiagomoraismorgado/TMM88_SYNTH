set(MIDI_SOURCES
    midi_handler.cpp
    spectral_delay.cpp
)
set(DSP_LIBRARY_SOURCES ${DSP_LIBRARY_SOURCES} ${MIDI_SOURCES} PARENT_SCOPE)