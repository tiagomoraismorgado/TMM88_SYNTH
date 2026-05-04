set(EFFECTS_SOURCES
    adsr.cpp
    overdrive.cpp
    crystals_delay.cpp
    eventide_filter.cpp
    tc_dynamics.cpp
    vss_reverb.cpp
)
set(DSP_LIBRARY_SOURCES ${DSP_LIBRARY_SOURCES} ${EFFECTS_SOURCES} PARENT_SCOPE)