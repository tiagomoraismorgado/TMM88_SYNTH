set(SYNTHESIS_SOURCES
    oscillator.cpp
    vca.cpp
    stereo_panner.cpp
    sub_oscillator.cpp
)
set(DSP_LIBRARY_SOURCES ${DSP_LIBRARY_SOURCES} ${SYNTHESIS_SOURCES} PARENT_SCOPE)