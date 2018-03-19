project(RadeonRays CXX)

#Grab RadeonRays sources
set(SOURCES
    src/accelerator/bvh.cpp
    src/accelerator/bvh2.cpp
    src/accelerator/hlbvh.cpp
    src/accelerator/split_bvh.cpp
    src/api/radeon_rays.cpp
    src/api/radeon_rays_impl.cpp
    src/device/calc_intersection_device.cpp
    src/intersector/intersector.cpp
    src/intersector/intersector_2level.cpp
    src/intersector/intersector_bittrail.cpp
    src/intersector/intersector_hlbvh.cpp
    src/intersector/intersector_lds.cpp
    src/intersector/intersector_short_stack.cpp
    src/intersector/intersector_skip_links.cpp
    src/primitive/mesh.cpp
    src/translator/fatnode_bvh_translator.cpp
    src/translator/plain_bvh_translator.cpp
    src/translator/q_bvh_translator.cpp
    src/util/options.cpp
    src/world/world.cpp)

if (RR_USE_EMBREE)
    list (APPEND SOURCES src/device/embree_intersection_device.cpp)
endif (RR_USE_EMBREE)

if (RR_USE_OPENCL)
    list (APPEND SOURCES src/device/calc_intersection_device_cl.cpp)
endif (RR_USE_OPENCL)

if (RR_USE_VULKAN)
    list (APPEND SOURCES src/device/calc_intersection_device_vk.cpp)
endif (RR_USE_VULKAN)

#Declare RadeonRays library
if (RR_ENABLE_STATIC)
    add_library(RadeonRays STATIC ${SOURCES})
    target_compile_definitions(RadeonRays PUBLIC RR_STATIC_LIBRARY=1)
else (NOT RR_ENABLE_STATIC)
    add_library(RadeonRays SHARED ${SOURCES})
    target_compile_definitions(RadeonRays PUBLIC RR_STATIC_LIBRARY=0)
endif(RR_ENABLE_STATIC)

#Configure RadeonRays build
target_include_directories(RadeonRays PUBLIC include)
target_include_directories(RadeonRays 
    PRIVATE . 
    PRIVATE ${EMBREE_INCLUDE_PATH})

target_link_libraries(RadeonRays PUBLIC Calc)

target_compile_definitions(RadeonRays PRIVATE EXPORT_API)

if (RR_SAFE_MATH)
    target_compile_definitions(RadeonRays PRIVATE USE_SAFE_MATH=1)
endif (RR_SAFE_MATH)

if (RR_USE_EMBREE)
    target_compile_definitions(RadeonRays PRIVATE USE_EMBREE=1)
    target_link_libraries(RadeonRays PUBLIC ${EMBREE_LIB})
endif (RR_USE_EMBREE)

if (RR_ENABLE_RAYMASK)
    target_compile_definitions(RadeonRays PRIVATE RR_RAY_MASK)
endif (RR_ENABLE_RAYMASK)

if (RR_USE_OPENCL)
    target_link_libraries(RadeonRays PUBLIC OpenCL::OpenCL)
endif (RR_USE_OPENCL)

if (RR_USE_VULKAN)
    target_link_libraries(RadeonRays PUBLIC Vulkan::Vulkan Anvil Thread::Threads)
endif (RR_USE_VULKAN)

if (UNIX)
        target_compile_options(RadeonRays PUBLIC -std=c++14 -msse4.2 -fPIC)
        target_link_libraries(RadeonRays INTERFACE "-Wl,--no-undefined")
        
        #read version from header
        file(STRINGS include/radeon_rays.h RR_API_VERSION REGEX "RADEONRAYS_API_VERSION")
        string(REGEX MATCH "[0-9]*\.[0-9]*$" RR_API_VERSION ${RR_API_VERSION})
        
        set_target_properties(RadeonRays PROPERTIES SOVERSION ${RR_API_VERSION})
elseif (APPLE)
    target_compile_options(RadeonRays PUBLIC -std=c++14 -stdlib=libc++)
endif (UNIX)


