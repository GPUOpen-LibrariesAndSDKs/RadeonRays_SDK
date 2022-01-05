cmake_minimum_required(VERSION 3.8)

project(RadeonRaysSDK CXX)

option(RR_EMBED_KERNELS "Embed CL kernels into binary module" OFF)
option(RR_ALLOW_CPU_DEVICES "Allows CPU Devices" OFF)
option(RR_USE_OPENCL "Use OpenCL for GPU hit testing" ON)
option(RR_USE_EMBREE "Use Intel(R) Embree for CPU hit testing" OFF)
option(RR_USE_VULKAN "Use vulkan for GPU hit testing" OFF)
option(RR_NO_TESTS "Don't add any unit tests and remove any test functionality from the library" OFF)
option(RR_ENABLE_STATIC "Create static libraries rather than dynamic" OFF)
option(RR_SHARED_CALC "Link Calc(compute abstraction layer) dynamically" OFF)
option(RR_ENABLE_RAYACTIVEFLAG "Enable ray masking in intersection kernels" OFF)
option(RR_ENABLE_RAYMASK "Enable ray masking in intersection kernels" OFF)
option(RR_ENABLE_BACKFACE_CULL "Enable backface culling in intersection kernels" OFF)
#option(RR_TUTORIALS "Add tutorials projects" OFF)
option(RR_SAFE_MATH "use safe math" OFF)
mark_as_advanced(FORCE RR_USE_VULKAN)

#global settings
if (WIN32)
    add_definitions(/MP -D_CRT_SECURE_NO_WARNINGS)
elseif (UNIX)
    add_definitions(-fvisibility=hidden)
endif (WIN32)

set(CMAKE_DEBUG_POSTFIX D)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if (RR_SHARED_CALC AND RR_USE_VULKAN)
    message(FATAL_ERROR "shared_calc option is not yet supported for Vulkan backend")
endif (RR_SHARED_CALC AND RR_USE_VULKAN)

#Find required packages
find_package(Threads)

find_package(PythonInterp 2.7 REQUIRED)
set (STRINGIFY_SCRIPT ${RadeonRaysSDK_SOURCE_DIR}/Tools/scripts/stringify.py)
set (EMBREE_INCLUDE_PATH ${RadeonRaysSDK_SOURCE_DIR}/3rdparty/embree/include)
if (WIN32)
    set (EMBREE_LIB ${RadeonRaysSDK_SOURCE_DIR}/3rdparty/embree/lib/x64/embree.lib)
elseif (APPLE)
    set (EMBREE_LIB ${RadeonRaysSDK_SOURCE_DIR}/3rdparty/embree/lib/x64/libembree.2.dylib)
elseif (UNIX)
    set (EMBREE_LIB embree2)
endif (WIN32)

if (RR_USE_OPENCL)
    set(OpenCL_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/CLW/CL")
    find_library(OpenCL_LIBRARY
	    NAMES OpenCL
	    PATHS "${CMAKE_CURRENT_SOURCE_DIR}/CLW/CL/lib"
    )

    if(NOT IS_ABSOLUTE ${OpenCL_LIBRARY})
        set(p "${CMAKE_CURRENT_SOURCE_DIR}/${OpenCL_LIBRARY}")
    endif()
    
    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(
	    OpenCL
	    FOUND_VAR OpenCL_FOUND
	    REQUIRED_VARS OpenCL_LIBRARY OpenCL_INCLUDE_DIRS
    )
    
    mark_as_advanced(
	    OpenCL_INCLUDE_DIRS
	    OpenCL_LIBRARY
    )

	add_library(OpenCL::OpenCL UNKNOWN IMPORTED)
	set_target_properties(OpenCL::OpenCL PROPERTIES
		IMPORTED_LOCATION "${OpenCL_LIBRARY}")
	set_target_properties(OpenCL::OpenCL PROPERTIES
    		INTERFACE_INCLUDE_DIRECTORIES "${OpenCL_INCLUDE_DIRS}")

    add_definitions(-DUSE_OPENCL=1)
    
    add_subdirectory(CLW)
endif (RR_USE_OPENCL)

if (RR_USE_VULKAN)
    find_package(Vulkan REQUIRED)
    add_definitions(-DUSE_VULKAN=1)
    
    add_subdirectory(Anvil)
endif (RR_USE_VULKAN)

add_subdirectory(Calc)
add_subdirectory(RadeonRays)


if (NOT RR_NO_TESTS)
    add_subdirectory(Gtest)
    add_subdirectory(UnitTest)
endif (NOT RR_NO_TESTS)
