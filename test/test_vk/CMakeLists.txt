find_package(Vulkan REQUIRED)

    set(VK_SPIRV
            ${PROJECT_SOURCE_DIR}/src/core/src/vlk/spirv_tools/spirv_cross.cpp
            ${PROJECT_SOURCE_DIR}/src/core/src/vlk/spirv_tools/spirv_parser.cpp
            ${PROJECT_SOURCE_DIR}/src/core/src/vlk/spirv_tools/spirv_cross_parsed_ir.cpp
            ${PROJECT_SOURCE_DIR}/src/core/src/vlk/spirv_tools/spirv_cfg.cpp
            ${PROJECT_SOURCE_DIR}/src/core/src/vlk/spirv_tools/spirv_cross_c.cpp
            ${PROJECT_SOURCE_DIR}/src/core/src/vlk/spirv_tools/spirv_glsl.cpp
            ${PROJECT_SOURCE_DIR}/src/core/src/vlk/spirv_tools/spirv_hlsl.cpp
            ${PROJECT_SOURCE_DIR}/src/core/src/vlk/spirv_tools/spirv_msl.cpp
            ${PROJECT_SOURCE_DIR}/src/core/src/vlk/spirv_tools/spirv_cpp.cpp
            ${PROJECT_SOURCE_DIR}/src/core/src/vlk/spirv_tools/spirv_reflect.cpp
            ${PROJECT_SOURCE_DIR}/src/core/src/vlk/spirv_tools/spirv_cross_util.cpp)
    source_group(spirv FILES ${VK_SPIRV})
    if (MSVC)
        set_source_files_properties(${VK_SPIRV} PROPERTIES COMPILE_FLAGS /wd4245)
    endif(MSVC)


add_executable(test_vk
    main.cpp
    algos_test.h
    basic_test.h
    common.h
    hlbvh_test.h
    internal_resources_test.h
    mesh_data.h
    stb_image_write.h
    tiny_obj_loader.h
    tiny_obj_loader.cc
    ${PROJECT_SOURCE_DIR}/src/core/src/vlk/hlbvh_builder.h
    ${PROJECT_SOURCE_DIR}/src/core/src/vlk/hlbvh_builder.cpp
    ${PROJECT_SOURCE_DIR}/src/core/src/vlk/radix_sort.h
    ${PROJECT_SOURCE_DIR}/src/core/src/vlk/radix_sort.cpp
    ${PROJECT_SOURCE_DIR}/src/core/src/vlk/restructure_hlbvh.h
    ${PROJECT_SOURCE_DIR}/src/core/src/vlk/restructure_hlbvh.cpp
    ${PROJECT_SOURCE_DIR}/src/core/src/vlk/scan.h
    ${PROJECT_SOURCE_DIR}/src/core/src/vlk/scan.cpp
    ${PROJECT_SOURCE_DIR}/src/core/src/vlk/shader_manager.h
    ${PROJECT_SOURCE_DIR}/src/core/src/vlk/shader_manager.cpp
    ${PROJECT_SOURCE_DIR}/src/core/src/vlk/update_hlbvh.h
    ${PROJECT_SOURCE_DIR}/src/core/src/vlk/update_hlbvh.cpp
    ${VK_SPIRV})

target_include_directories(test_vk PRIVATE
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/src/core/src
)

target_link_libraries(test_vk PRIVATE radeonrays project_options gtest Vulkan::Vulkan)
if (EMBEDDED_KERNELS)
    target_compile_definitions(test_vk PRIVATE RR_EMBEDDED_KERNELS)
endif(EMBEDDED_KERNELS)

