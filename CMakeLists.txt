cmake_minimum_required(VERSION 3.8)

project(RadeonRays CXX)

option(RR_EMBED_KERNELS "Embed CL kernels into binary module" OFF)
option(RR_ALLOW_CPU_DEVICES "Allows CPU Devices" OFF)
option(RR_USE_OPENCL "Use OpenCL for GPU hit testing" ON)
option(RR_USE_EMBREE "Use Intel(R) Embree for CPU hit testing" OFF)
option(RR_USE_VULKAN "Use vulkan for GPU hit testing" OFF)
option(RR_NO_TESTS "Don't add any unit tests and remove any test functionality from the library" OFF)
option(RR_ENABLE_STATIC "Create static libraries rather than dynamic" OFF)
option(RR_SHARED_CALC "Link Calc(compute abstraction layer) dynamically" OFF)
option(RR_ENABLE_RAYMASK "Enable ray masking in intersection kernels" OFF)
option(RR_TUTORIALS "Add tutorials projects" OFF)
option(RR_SAFE_MATH "use safe math" OFF)

if (RR_SHARED_CALC AND RR_USE_VULKAN)
    message(FATAL_ERROR "shared_calc option is not yet supported for Vulkan backend")
endif (RR_SHARED_CALC AND RR_USE_VULKAN)

#Find required packages
find_package(Threads)

if (RR_EMBED_KERNELS)
    find_package(PythonInterp 2.7 REQUIRED)
endif(RR_EMBED_KERNELS)

if (RR_USE_OPENCL)
    find_package(OpenCL REQUIRED)
endif (RR_USE_OPENCL)

if (RR_USE_VULKAN)
    find_package(Vulkan REQUIRED)
    add_subdirectory(Anvil)
endif (RR_USE_VULKAN)

if (RR_USE_OPENCL)
    add_subdirectory(CLW)
endif (RR_USE_OPENCL)

#add_subdirectory(RadeonRays)
#add_subdirectory(Calc)

if (NOT RR_NO_TESTS)
    add_subdirectory(Gtest)
#    add_subdirectory(UnitTest)
endif (NOT RR_NO_TESTS)

if (RR_TUTORIALS)
#    add_subdirectory(Tutorials)
endif (RR_TUTORIALS)

