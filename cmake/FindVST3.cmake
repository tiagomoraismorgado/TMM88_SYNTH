# FindVST3.cmake
# Locates the Steinberg VST3 SDK
# Sets:
#  VST3_FOUND        - True if VST3 is found
#  VST3_INCLUDE_DIRS - Include directories for VST3

find_path(VST3_ROOT_DIR
    NAMES pluginterfaces/vst/vsttypes.h
    PATHS
        "$ENV{VST3_SDK_DIR}"
        "C:/SDKs/VST_SDK/VST3_SDK"
        "${CMAKE_CURRENT_SOURCE_DIR}/vst3sdk"
    DOC "Path to the VST3 SDK root directory"
)

if(VST3_ROOT_DIR)
    set(VST3_FOUND TRUE)
    set(VST3_INCLUDE_DIRS 
        "${VST3_ROOT_DIR}"
        "${VST3_ROOT_DIR}/pluginterfaces"
        "${VST3_ROOT_DIR}/public.sdk"
    )
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(VST3
    REQUIRED_VARS VST3_ROOT_DIR VST3_INCLUDE_DIRS
)

mark_as_advanced(VST3_ROOT_DIR)