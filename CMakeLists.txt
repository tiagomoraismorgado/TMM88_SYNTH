cmake_minimum_required(VERSION 3.18)

project(TMM88Synth LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported OUTPUT error)
if(ipo_supported)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
endif()

# Enable code coverage instrumentation for Debug or Coverage builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "Coverage")
    message(STATUS "Enabling code coverage instrumentation.")
    # For GCC/Clang (MSVC uses a different coverage mechanism not covered here)
    if(NOT MSVC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage -fno-inline -fno-elide-constructors")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
    else()
        message(STATUS "GCOV-based code coverage is not supported for MSVC.")
    endif()
endif()

# Initialize an empty list for DSP library sources
set(DSP_LIBRARY_SOURCES "")

# Add subdirectories for DSP modules. Each subdirectory's CMakeLists.txt
# will append its sources to DSP_LIBRARY_SOURCES using PARENT_SCOPE.
add_subdirectory(src/synthesis)
add_subdirectory(src/filters)
add_subdirectory(src/effects)
add_subdirectory(src/midi)
add_subdirectory(src/devices) # This will handle platform-specific audio devices

add_library(DSPCore STATIC ${DSP_LIBRARY_SOURCES})

target_include_directories(DSPCore PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# Handle ASIO SDK include directory globally, as it's a dependency for DSPCore
if(WIN32 AND DEFINED ENV{ASIO_SDK_PATH})
    message(STATUS "ASIO SDK found at: $ENV{ASIO_SDK_PATH}")
    target_include_directories(DSPCore PUBLIC $ENV{ASIO_SDK_PATH}/common)
elseif(WIN32)
    message(STATUS "ASIO SDK not found. Set ASIO_SDK_PATH environment variable to enable ASIO support.")
endif()

# Find Qt6 for the SynthApp (UI)
find_package(Qt6 COMPONENTS Core Gui Qml Quick QUIET)

if(NOT Qt6_FOUND)
    message(STATUS "Qt6 not found. SynthApp will not be built.")
    message(STATUS "To enable the Qt UI:")
    message(STATUS "1. Install Qt6 from https://www.qt.io/download")
    message(STATUS "2. Or use: winget install miurahr.aqtinstall")
    message(STATUS "3. Then run: aqt install-qt windows desktop 6.5.0 gcc_64 --outputdir C:/Qt")
    message(STATUS "4. Set CMAKE_PREFIX_PATH to your Qt installation")
endif()

if(Qt6_FOUND)
    qt_add_executable(SynthApp
        src/ui/main.cpp
        src/ui/OscilloscopeItem.cpp
        src/synth/SynthController.cpp
    )

    qt_add_qml_module(SynthApp
        URI Synth.Engine
        VERSION 1.0
        QML_FILES qml/main.qml
    )

    target_link_libraries(SynthApp PRIVATE
        DSPCore
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
    )
else()
    message(STATUS "Qt6 not found. SynthApp will not be built.")
endif()

add_executable(dsp_example examples/main.cpp)
target_link_libraries(dsp_example PRIVATE DSPCore)

# --- Unit Tests ---
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
# Prevent overriding the parent project's compiler/linker on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# --- Profiling Flags ---
if(NOT MSVC)
    # Required for accurate stack traces in Flame Graphs
    target_compile_options(DSPCore PRIVATE -fno-omit-frame-pointer)
    add_compile_options(-fno-omit-frame-pointer)
endif()

# --- Benchmarks ---
FetchContent_Declare(
    googlebenchmark
    URL https://github.com/google/benchmark/archive/refs/tags/v1.8.3.zip
)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googlebenchmark)

add_executable(dsp_benchmarks tests/benchmark_oscillator.cpp)
target_link_libraries(dsp_benchmarks PRIVATE
    DSPCore
    benchmark::benchmark
)
if(NOT MSVC)
    target_link_libraries(dsp_benchmarks PRIVATE pthread)
endif()

# --- Flame Graph Generation (Linux/perf) ---
find_program(PERF_EXECUTABLE perf)
if(PERF_EXECUTABLE AND NOT MSVC)
    # You will need the FlameGraph scripts cloned locally:
    # git clone https://github.com/brendangregg/FlameGraph.git
    set(FLAMEGRAPH_DIR "${CMAKE_SOURCE_DIR}/tools/FlameGraph" CACHE PATH "Path to FlameGraph scripts")

    if(EXISTS "${FLAMEGRAPH_DIR}/flamegraph.pl")
        add_custom_target(flamegraph
            COMMAND ${PERF_EXECUTABLE} record -g -F 999 -- ./dsp_benchmarks --benchmark_min_time=5
            COMMAND ${PERF_EXECUTABLE} script > out.perf
            COMMAND ${FLAMEGRAPH_DIR}/stackcollapse-perf.pl out.perf > out.folded
            COMMAND ${FLAMEGRAPH_DIR}/flamegraph.pl out.folded > oscillator_flamegraph.svg
            COMMAND ${CMAKE_COMMAND} -E echo "Flame graph generated: oscillator_flamegraph.svg"
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            DEPENDS dsp_benchmarks
            COMMENT "Running benchmarks under perf and generating flame graph..."
        )
    else()
        message(STATUS "FlameGraph scripts not found at ${FLAMEGRAPH_DIR}. Target 'flamegraph' disabled.")
        message(STATUS "To enable: git clone https://github.com/brendangregg/FlameGraph.git into a 'tools' directory.")
    endif()
endif()

add_executable(dsp_tests 
    tests/test_dsp.cpp 
    tests/test_integration.cpp
)
target_link_libraries(dsp_tests PRIVATE
    DSPCore
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(dsp_tests)

# --- Code Coverage ---
find_program(LCOV_EXECUTABLE lcov)
find_program(GENHTML_EXECUTABLE genhtml)

if(LCOV_EXECUTABLE AND GENHTML_EXECUTABLE AND (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "Coverage") AND NOT MSVC)
    message(STATUS "LCOV and GenHTML found. Code coverage reporting can be generated.")

    # Define a custom target for coverage
    add_custom_target(coverage
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/coverage_report
        COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_BINARY_DIR}/*.gcda ${CMAKE_BINARY_DIR}/*.gcno
        COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_BINARY_DIR}/src/**/*.gcda ${CMAKE_BINARY_DIR}/src/**/*.gcno
        # Capture initial coverage data (before running tests)
        COMMAND ${LCOV_EXECUTABLE} --capture --initial --directory ${CMAKE_BINARY_DIR} --output-file ${CMAKE_BINARY_DIR}/coverage_base.info
        # Build and run the tests
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target dsp_tests
        COMMAND ${CMAKE_BINARY_DIR}/dsp_tests
        # Capture coverage data after running tests
        COMMAND ${LCOV_EXECUTABLE} --capture --directory ${CMAKE_BINARY_DIR} --output-file ${CMAKE_BINARY_DIR}/coverage_test.info
        # Combine initial and test coverage data
        COMMAND ${LCOV_EXECUTABLE} --add-service-info ${CMAKE_BINARY_DIR}/coverage_base.info --add-service-info ${CMAKE_BINARY_DIR}/coverage_test.info --output-file ${CMAKE_BINARY_DIR}/coverage_total.info
        # Filter out irrelevant files (googletest, test files, headers, etc.)
        COMMAND ${LCOV_EXECUTABLE} --remove ${CMAKE_BINARY_DIR}/coverage_total.info "*googletest*" "*test_dsp.cpp*" "*test_integration.cpp*" "*framework.h*" "*dsp.h*" --output-file ${CMAKE_BINARY_DIR}/coverage_filtered.info
        # Generate HTML report
        COMMAND ${GENHTML_EXECUTABLE} ${CMAKE_BINARY_DIR}/coverage_filtered.info --output-directory ${CMAKE_BINARY_DIR}/coverage_report
        COMMAND ${CMAKE_COMMAND} -E echo "Code coverage report generated at: ${CMAKE_BINARY_DIR}/coverage_report/index.html"
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generating code coverage report"
    )
else()
    message(STATUS "LCOV or GenHTML not found, or build type is not Debug/Coverage, or MSVC is used. Code coverage reporting will not be available.")
endif()

# --- Compiler Specifics ---

if(MSVC)
    # Enable AVX2 instructions for SIMD refactors
    target_compile_options(DSPCore PRIVATE /W4 /Ox /arch:AVX2 /fp:fast)
    # ASIO needs __WINDOWS__ defined
    target_compile_definitions(SynthApp PRIVATE _CRT_SECURE_NO_WARNINGS WIN32 __WINDOWS__) #
else()
    # Enable AVX2, FMA, and Fast Math for high-performance math
    target_compile_options(DSPCore PRIVATE -Wall -O3 -mavx2 -mfma -ffast-math)
endif()
message(STATUS "DSP Framework Project Configured.")
message(STATUS "Build Mode: ${CMAKE_BUILD_TYPE}")
message(STATUS "Qt6 Version: ${Qt6_VERSION}")
if(CMAKE_CUDA_COMPILER_LOADED)
    message(STATUS "CUDA Support: Enabled (${CMAKE_CUDA_COMPILER_ID})")
endif()