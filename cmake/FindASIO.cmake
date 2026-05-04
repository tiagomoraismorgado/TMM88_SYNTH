# FindASIO.cmake
# Locates the Steinberg ASIO SDK
# Sets:
#  ASIO_FOUND        - True if ASIO is found
#  ASIO_INCLUDE_DIRS - Include directories for ASIO
#  ASIO_SOURCE_FILE  - Path to common/asio.cpp

find_path(ASIO_ROOT_DIR
    NAMES common/asio.h
    PATHS
        "$ENV{ASIO_SDK_DIR}"
        "C:/SDKs/ASIOSDK2.3"
        "C:/SDKs/ASIO"
        "/usr/local/include/asio"
    DOC "Path to the ASIO SDK root directory"
)

if(ASIO_ROOT_DIR)
    set(ASIO_INCLUDE_DIRS "${ASIO_ROOT_DIR}/common")
    
    find_file(ASIO_SOURCE_FILE
        NAMES asio.cpp
        PATHS "${ASIO_ROOT_DIR}/common"
        NO_DEFAULT_PATH
    )

    if(ASIO_SOURCE_FILE)
        set(ASIO_FOUND TRUE)
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ASIO
    REQUIRED_VARS ASIO_ROOT_DIR ASIO_INCLUDE_DIRS ASIO_SOURCE_FILE
)

mark_as_advanced(ASIO_ROOT_DIR ASIO_SOURCE_FILE)