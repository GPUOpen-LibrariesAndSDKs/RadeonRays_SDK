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
    set(KERNEL_SOURCES
        src/kernels/CL/build_hlbvh.cl
        src/kernels/CL/common.cl
        src/kernels/CL/intersect_bvh2level_skiplinks.cl
        src/kernels/CL/intersect_bvh2_bittrail.cl
        src/kernels/CL/intersect_bvh2_lds.cl
        src/kernels/CL/intersect_bvh2_lds_fp16.cl
        src/kernels/CL/intersect_bvh2_short_stack.cl
        src/kernels/CL/intersect_bvh2_skiplinks.cl
        src/kernels/CL/intersect_hlbvh_stack.cl)
endif (RR_USE_OPENCL)

if (RR_USE_VULKAN)
    list (APPEND DEVICE_SOURCES 
        src/device/calc_intersection_device_vk.cpp
        src/device/calc_intersection_device_vk.h)
    set(KERNEL_SOURCES
        src/kernels/GLSL/bvh.comp
        src/kernels/GLSL/bvh2.comp
        src/kernels/GLSL/bvh2l.comp
        src/kernels/GLSL/bvh2_fp16.comp
        src/kernels/GLSL/fatbvh.comp
        src/kernels/GLSL/hlbvh.comp
        src/kernels/GLSL/hlbvh_build.comp)
endif (RR_USE_VULKAN)

source_group("accelerator" FILES ${ACCELERATOR_SOURCES})
source_group("api" FILES ${API_SOURCES})
source_group("async" FILES ${ASYNC_SOURCES})
source_group("device" FILES ${DEVICE_SOURCES})
source_group("except" FILES ${EXCEPT_SOURCES})
source_group("intersector" FILES ${INTERSECTOR_SOURCES})
source_group("primitive" FILES ${PRIMITIVE_SOURCES})
source_group("translator" FILES ${TRANSLATOR_SOURCES})
source_group("util" FILES ${UTIL_SOURCES})
source_group("world" FILES ${WORLD_SOURCES})
source_group("kernels" FILES ${KERNEL_SOURCES})

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
    ${WORLD_SOURCES}
    ${KERNEL_SOURCES})

if (RR_EMBED_KERNELS)
    add_subdirectory(src/kernels)
endif (RR_EMBED_KERNELS)

if (RR_USE_VULKAN)
    #Anvil specify vulkan-1 lib directly so we need to add correct paths
    if (WIN32)
        link_directories   ($ENV{VK_SDK_PATH}/Lib
                            $ENV{VULKAN_SDK}/Lib)
    else()
        link_directories   ($ENV{VK_SDK_PATH}/x86_64/lib
                            $ENV{VULKAN_SDK}/x86_64/lib)
    endif (WIN32)
endif (RR_USE_VULKAN)

#Declare RadeonRays library
if (RR_ENABLE_STATIC)
    add_library(RadeonRays STATIC ${SOURCES})
    target_compile_definitions(RadeonRays PUBLIC RR_STATIC_LIBRARY=1)
else (NOT RR_ENABLE_STATIC)
    add_library(RadeonRays SHARED ${SOURCES})
    target_compile_definitions(RadeonRays PUBLIC RR_STATIC_LIBRARY=0)
endif(RR_ENABLE_STATIC)

if (RR_EMBED_KERNELS)
    target_compile_definitions(RadeonRays PRIVATE RR_EMBED_KERNELS=1)
    add_dependencies(RadeonRays RadeonRaysKernelCache)
    target_include_directories(RadeonRays PRIVATE ${RadeonRays_BINARY_DIR})
endif (RR_EMBED_KERNELS)

#Configure RadeonRays build
target_include_directories(RadeonRays PUBLIC include)
target_include_directories(RadeonRays 
    PRIVATE . 
    PRIVATE ${EMBREE_INCLUDE_PATH})

target_link_libraries(RadeonRays PUBLIC Calc Threads::Threads)

target_compile_definitions(RadeonRays PRIVATE EXPORT_API)

if (RR_SAFE_MATH)
    target_compile_definitions(RadeonRays PUBLIC USE_SAFE_MATH=1)
endif (RR_SAFE_MATH)

if (RR_USE_EMBREE)
    target_compile_definitions(RadeonRays PUBLIC USE_EMBREE=1)
    target_link_libraries(RadeonRays PUBLIC ${EMBREE_LIB})
endif (RR_USE_EMBREE)

if (RR_ENABLE_RAYMASK)
    target_compile_definitions(RadeonRays PRIVATE RR_RAY_MASK)
endif (RR_ENABLE_RAYMASK)

if (RR_ENABLE_BACKFACE_CULL)
    target_compile_definitions(RadeonRays PRIVATE RR_BACKFACE_CULL)
endif (RR_ENABLE_BACKFACE_CULL)

if (RR_USE_OPENCL)
    target_link_libraries(RadeonRays PUBLIC OpenCL::OpenCL)
    target_compile_definitions(RadeonRays PUBLIC USE_OPENCL=1)
endif (RR_USE_OPENCL)

if (RR_USE_VULKAN)
    #Need to add Anvil to include path
    target_include_directories(RadeonRays
        PRIVATE "${RadeonRaysSDK_SOURCE_DIR}/Anvil/deps"
        PRIVATE "${RadeonRaysSDK_SOURCE_DIR}/Anvil/include")
  
    target_link_libraries(RadeonRays PUBLIC Vulkan::Vulkan Anvil)
    target_compile_definitions(RadeonRays PUBLIC USE_VULKAN=1)
endif (RR_USE_VULKAN)

target_compile_features(RadeonRays PRIVATE cxx_std_14)

if (UNIX AND NOT APPLE)
        target_compile_options(RadeonRays PUBLIC -msse4.2 -fPIC)
        target_link_libraries(RadeonRays INTERFACE "-Wl,--no-undefined")
        
        #read version from header
        file(STRINGS include/radeon_rays.h RR_API_VERSION REGEX "RADEONRAYS_API_VERSION")
        string(REGEX MATCH "[0-9]*\.[0-9]*$" RR_API_VERSION ${RR_API_VERSION})
        
        set_target_properties(RadeonRays PROPERTIES SOVERSION ${RR_API_VERSION})
elseif (APPLE)
    target_compile_options(RadeonRays PUBLIC -stdlib=libc++)
endif (UNIX AND NOT APPLE)


