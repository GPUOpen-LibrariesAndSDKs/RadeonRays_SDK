cmake_minimum_required(VERSION 3.12)

project(radeonrays CXX)
include(cmake/StandardProjectSettings.cmake)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

# Link this 'library' to set the c++ standard / compile-time options requested
add_library(project_options INTERFACE)
target_compile_features(project_options INTERFACE cxx_std_17)

# Link this 'library' to use the warnings specified in CompilerWarnings.cmake
add_library(project_warnings INTERFACE)

# sanitizer options if supported by compiler
include(cmake/Sanitizers.cmake)
enable_sanitizers(project_options)

# enable doxygen
include(cmake/Doxygen.cmake)
enable_doxygen()

# allow for static analysis options
include(cmake/StaticAnalyzers.cmake)

option(BUILD_SHARED_LIBS "Enable compilation of shared libraries" ON)
option(ENABLE_TESTING "Enable Test Builds" ON)
option(ENABLE_DX12 "Enable DX12 backend" OFF)
option(ENABLE_VULKAN "Enable Vulkan backend" OFF)
option(EMBEDDED_KERNELS "Enable embedding kernels/shaders into library" OFF)

# Very basic PCH example
option(ENABLE_PCH "Enable Precompiled Headers" OFF)
if (ENABLE_PCH)
    # This sets a global PCH parameter, each project will build its own PCH, which
    # is a good idea if any #define's change
    # 
    # 
    target_precompile_headers(project_options INTERFACE <vector> <string> <map> <utility>)
endif()

if(ENABLE_VULKAN)
    include(cmake/KernelUtils.cmake)
endif(ENABLE_VULKAN)

if(ENABLE_TESTING)
    enable_testing()
    set(gtest_force_shared_crt ON CACHE BOOL "Use /MD and /MDd" FORCE)
    add_subdirectory(third_party/gtest)

    set(THIRD_PARTY_TARGETS
        gtest
        gtest_main
        gmock
        gmock_main)

    foreach(TGT ${THIRD_PARTY_TARGETS})
        set_property(TARGET ${TGT} PROPERTY FOLDER "third_party")
    endforeach()

    add_subdirectory(test)
endif(ENABLE_TESTING)

if(ENABLE_FUZZING)
    message(
        "Building Fuzz Tests, using fuzzing sanitizer https://www.llvm.org/docs/LibFuzzer.html"
    )
    add_subdirectory(fuzz_test)
endif(ENABLE_FUZZING)

add_subdirectory(src)
add_subdirectory(bvh_analyzer)
