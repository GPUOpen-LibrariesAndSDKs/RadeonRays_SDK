find_package(spdlog REQUIRED)

if (ENABLE_VULKAN)
find_package(Vulkan REQUIRED)
endif(ENABLE_VULKAN)

# Common sources
set(RR_SOURCES
    include/radeonrays.h
    src/radeonrays.cpp
    src/context.h
    src/base/backend.h
    src/base/device_base.h
    src/base/command_stream_base.h
    src/base/event_base.h
    src/base/device_ptr_base.h
    src/base/intersector_base.h
    src/utils/logger.h
    src/utils/logger.cpp
    src/utils/pool.h
    src/utils/memory_layout.h
    src/utils/warning_ignore_general.h
    src/utils/warning_pop.h
    src/utils/warning_push.h)

source_group(base FILES ${RR_SOURCES})

# DX12 specific sources
if (ENABLE_DX12)
    set(DX_INTERSECTOR_FALLBACK
            src/dx/geometry_trace.h
            src/dx/geometry_trace.cpp
            src/dx/fallback_intersector.h
            src/dx/fallback_intersector.cpp
            src/dx/hlbvh_builder.h
            src/dx/hlbvh_builder.cpp
            src/dx/hlbvh_top_level_builder.h
            src/dx/hlbvh_top_level_builder.cpp
            src/dx/scene_trace.h
            src/dx/scene_trace.cpp
            src/dx/update_hlbvh.h
            src/dx/update_hlbvh.cpp)
    source_group(dx12\\intersector_fallback FILES 
        ${DX_INTERSECTOR_FALLBACK})

    set(DX_OPTIMIZER
            src/dx/restructure_hlbvh.h
            src/dx/restructure_hlbvh.cpp)
    source_group(dx12\\optimizer FILES 
        ${DX_OPTIMIZER})

    set(DX_INTERSECTOR_HWI
            src/dx/hwi/hwi_intersector.h
            src/dx/hwi/hwi_intersector.cpp
            src/dx/hwi/sah_bvh.h
            src/dx/hwi/sah_bvh.cpp
            src/dx/hwi/hlbvh.h
            src/dx/hwi/hlbvh.cpp
            src/dx/hwi/collapse_bvh.h
            src/dx/hwi/collapse_bvh.cpp
            src/dx/hwi/collapse_bvh_top_level.h
            src/dx/hwi/collapse_bvh_top_level.cpp
            src/dx/hwi/update_bvh.h
            src/dx/hwi/update_bvh.cpp
            src/dx/hwi/hlbvh_top_level.h
            src/dx/hwi/hlbvh_top_level.cpp)
    source_group(dx12\\_intersector_hwi FILES 
        ${DX_INTERSECTOR_HWI})

    set(DX_ALGO
        src/dx/algorithm/radix_sort.h
        src/dx/algorithm/radix_sort.cpp
        src/dx/algorithm/scan.h
        src/dx/algorithm/scan.cpp
        src/dx/algorithm/compress_triangles.h
        src/dx/algorithm/compress_triangles.cpp)
    source_group(dx12\\algorithms FILES
        ${DX_ALGO})

    set(DX12_WRAPPERS
        include/radeonrays_dx.h
        src/dx/common/d3dx12.h
        src/dx/common/dx12_wrappers.h
        src/dx/common.h
        src/dx/common.cpp
        src/dx/command_stream.h
        src/dx/command_stream.cpp
        src/dx/event.h
        src/dx/device.h
        src/dx/device.cpp
        src/dx/device_ptr.h
        src/dx/intersector_dispatch.h
        src/dx/intersector_dispatch.cpp
        src/dx/shader_compiler.h
        src/dx/shader_compiler.cpp)
    source_group(dx12\\wrappers FILES
        ${DX12_WRAPPERS})

    set(RR_SOURCES_DX_COMMON
        ${DX12_WRAPPERS}
        ${DX_ALGO}
        ${DX_INTERSECTOR_FALLBACK}
        ${DX_OPTIMIZER})
       
    list(APPEND RR_SOURCES
        include/radeonrays_dx.h
        ${RR_SOURCES_DX_COMMON}
        ${DX_INTERSECTOR_FALLBACK})
		
endif(ENABLE_DX12)

if (ENABLE_VULKAN)
    set(VK_WRAPPERS
            include/radeonrays_vlk.h
            src/vlk/allocation.h
            src/vlk/buffer.h
            src/vlk/command_stream.h
            src/vlk/command_stream.cpp
            src/vlk/common.h
            src/vlk/device.h
            src/vlk/device.cpp
            src/vlk/device_ptr.h
            src/vlk/event.h
            src/vlk/gpu_helper.h
            src/vlk/intersector_dispatch.h
            src/vlk/intersector_dispatch.cpp
            src/vlk/shader_manager.h
            src/vlk/shader_manager.cpp
            src/vlk/vulkan_wrappers.h)
    source_group(vk\\wrappers FILES ${VK_WRAPPERS})

    set(VK_INTERSECTOR
            src/vlk/geometry_trace.h
            src/vlk/geometry_trace.cpp
            src/vlk/hlbvh_builder.h
            src/vlk/hlbvh_builder.cpp
            src/vlk/hlbvh_top_level_builder.h
            src/vlk/hlbvh_top_level_builder.cpp
            src/vlk/intersector.h
            src/vlk/intersector.cpp
            src/vlk/restructure_hlbvh.h
            src/vlk/restructure_hlbvh.cpp
            src/vlk/scene_trace.h
            src/vlk/scene_trace.cpp
            src/vlk/shader_manager.h
            src/vlk/shader_manager.cpp
            src/vlk/update_hlbvh.h
            src/vlk/update_hlbvh.cpp)
    source_group(vk\\intersector FILES ${VK_INTERSECTOR})

    set(VK_ALGO
            src/vlk/radix_sort.h
            src/vlk/radix_sort.cpp
            src/vlk/scan.h
            src/vlk/scan.cpp)
    source_group(vk\\algorithms FILES ${VK_ALGO})

    set(VK_SPIRV
            src/vlk/spirv_tools/spirv_cross.cpp
            src/vlk/spirv_tools/spirv_parser.cpp
            src/vlk/spirv_tools/spirv_cross_parsed_ir.cpp
            src/vlk/spirv_tools/spirv_cfg.cpp
            src/vlk/spirv_tools/spirv_cross_c.cpp
            src/vlk/spirv_tools/spirv_glsl.cpp
            src/vlk/spirv_tools/spirv_hlsl.cpp
            src/vlk/spirv_tools/spirv_msl.cpp
            src/vlk/spirv_tools/spirv_cpp.cpp
            src/vlk/spirv_tools/spirv_reflect.cpp
            src/vlk/spirv_tools/spirv_cross_util.cpp)
    source_group(vk\\spirv FILES ${VK_SPIRV})
    if (MSVC)
        set_source_files_properties(${VK_SPIRV} PROPERTIES COMPILE_FLAGS /wd4245)
    endif(MSVC)

    list(APPEND RR_SOURCES
            ${VK_WRAPPERS}
            ${VK_ALGO}
            ${VK_INTERSECTOR}
            ${VK_SPIRV})
endif(ENABLE_VULKAN)

add_library(radeonrays ${RR_SOURCES})

target_include_directories(radeonrays
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${PROJECT_SOURCE_DIR}/third_party/dxc
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_definitions(radeonrays PRIVATE RR_EXPORT_API)

target_link_libraries(radeonrays PRIVATE project_options project_warnings spdlog::spdlog)
if (EMBEDDED_KERNELS)
    target_compile_definitions(radeonrays PRIVATE RR_EMBEDDED_KERNELS)
endif(EMBEDDED_KERNELS)

if (ENABLE_DX12)
    # Copy DXC dlls
    add_custom_command(
        TARGET radeonrays POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${PROJECT_SOURCE_DIR}/third_party/dxc/bin/dxcompiler.dll
        "\$\(OutDir\)/dxcompiler.dll")

	add_custom_command(
        TARGET radeonrays POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${PROJECT_SOURCE_DIR}/third_party/dxc/bin/dxil.dll
        "\$\(OutDir\)/dxil.dll")

    target_compile_definitions(radeonrays PRIVATE RR_ENABLE_DX12)
    target_link_libraries(radeonrays PRIVATE d3d12 dxgi dxguid)
endif(ENABLE_DX12)

if (ENABLE_VULKAN)
    target_compile_definitions(radeonrays PRIVATE RR_ENABLE_VK)
    target_compile_definitions(radeonrays PRIVATE SPIRV_CROSS_C_API_GLSL)
    target_link_libraries(radeonrays PRIVATE Vulkan::Vulkan)
    add_subdirectory(src/vlk/kernels ${CMAKE_CURRENT_BINARY_DIR}/vlk_kernels_project_files)
endif(ENABLE_VULKAN)