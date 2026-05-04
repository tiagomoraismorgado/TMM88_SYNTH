set(DEVICE_SOURCES)

if(WIN32)
    if(DEFINED ENV{ASIO_SDK_PATH}) # Check for ASIO SDK path here, but include directory is handled in main CMakeLists.txt
        list(APPEND DEVICE_SOURCES AsioAudioDevice.cpp)
    endif()
elseif(APPLE)
    list(APPEND DEVICE_SOURCES CoreAudioDevice.cpp)
elseif(UNIX)
    list(APPEND DEVICE_SOURCES JackAudioDevice.cpp AlsaAudioDevice.cpp)
endif()

set(DSP_LIBRARY_SOURCES ${DSP_LIBRARY_SOURCES} ${DEVICE_SOURCES} PARENT_SCOPE)