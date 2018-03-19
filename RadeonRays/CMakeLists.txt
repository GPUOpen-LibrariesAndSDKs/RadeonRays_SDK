project(RadeonRays CXX)

#Grab RadeonRays sources
set(PUBLIC_HEADERS
    include/math/bbox.h
    include/math/float2.h
    include/math/float3.h
    include/math/int2.h
    include/math/mathutils.h
    include/math/matrix.h
    include/math/quaternion.h
    include/math/ray.h
    include/radeon_rays.h
    include/radeon_rays_cl.h
    include/radeon_rays_vk.h)
    
set(ACCELERATOR_SOURCES
    src/accelerator/bvh.cpp
    src/accelerator/bvh.h
    src/accelerator/bvh2.cpp
    src/accelerator/bvh2.h
    src/accelerator/hlbvh.cpp
    src/accelerator/hlbvh.h
    src/accelerator/split_bvh.cpp
    src/accelerator/split_bvh.h)

set(API_SOURCES
    src/api/radeon_rays.cpp
    src/api/radeon_rays_impl.cpp
    src/api/radeon_rays_impl.h)

set(ASYNC_SOURCES src/async/thread_pool.h)
set(DEVICE_SOURCES
    src/device/calc_holder.h
    src/device/calc_intersection_device.cpp
    src/device/calc_intersection_device.h
    src/device/intersection_device.h)

set(EXCEPT_SOURCES src/except/except.h)

set(INTERSECTOR_SOURCES
    src/intersector/intersector.cpp
    src/intersector/intersector.h
    src/intersector/intersector_2level.cpp
    src/intersector/intersector_2level.h
    src/intersector/intersector_bittrail.cpp
    src/intersector/intersector_bittrail.h
    src/intersector/intersector_hlbvh.cpp
    src/intersector/intersector_hlbvh.h
    src/intersector/intersector_lds.cpp
    src/intersector/intersector_lds.h
    src/intersector/intersector_short_stack.cpp
    src/intersector/intersector_short_stack.h
    src/intersector/intersector_skip_links.cpp
    src/intersector/intersector_skip_links.h)

set(PRIMITIVE_SOURCES
    src/primitive/instance.h
    src/primitive/mesh.cpp
    src/primitive/mesh.h
    src/primitive/shapeimpl.h)
    
set(TRANSLATOR_SOURCES
    src/translator/fatnode_bvh_translator.cpp
    src/translator/fatnode_bvh_translator.h
    src/translator/plain_bvh_translator.cpp
    src/translator/plain_bvh_translator.h
    src/translator/q_bvh_translator.cpp
    src/translator/q_bvh_translator.h)
    
set(UTIL_SOURCES
    src/util/alignedalloc.h
    src/util/options.cpp
    src/util/options.h
    src/util/perfect_hash_map.h
    src/util/progressreporter.h)

set(WORLD_SOURCES
    src/world/world.cpp
    src/world/world.h)

if (RR_USE_EMBREE)
    list (APPEND DEVICE_SOURCES 
    src/device/embree_intersection_device.cpp
    src/device/embree_intersection_device.h)
endif (RR_USE_EMBREE)

if (RR_USE_OPENCL)
    list (APPEND DEVICE_SOURCES 
        src/device/calc_intersection_device_cl.cpp
        src/device/calc_intersection_device_cl.h)
endif (RR_USE_OPENCL)

if (RR_USE_VULKAN)
    list (APPEND DEVICE_SOURCES 
        src/device/calc_intersection_device_vk.cpp
        src/device/calc_intersection_device_vk.h)
endif (RR_USE_VULKAN)

source_group("accelerator" FILES ${ACCELERATOR_SOURCES})
source_group("api" FILES ${API_SOURCES})
source_group("async" FILES ${ASYNC_SOURCES})
source_group("device" FILES ${DEVICE_SOURCES})
source_group("except" FILES ${EXCEPT_SOURCES})
source_group("intersecotr" FILES ${INTERSECTOR_SOURCES})
source_group("primitive" FILES ${PRIMITIVE_SOURCES})
source_group("translator" FILES ${TRANSLATOR_SOURCES})
source_group("util" FILES ${UTIL_SOURCES})
source_group("world" FILES ${WORLD_SOURCES})

#Gather all sources together
set(SOURCES
    ${PUBLIC_HEADERS}
    ${ACCELERATOR_SOURCES}
    ${API_SOURCES}
    ${ASYNC_SOURCES}
    ${DEVICE_SOURCES}
    ${EXCEPT_SOURCES}
    ${INTERSECTOR_SOURCES}
    ${PRIMITIVE_SOURCES}
    ${TRANSLATOR_SOURCES}
    ${UTIL_SOURCES}
    ${WORLD_SOURCES})

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


