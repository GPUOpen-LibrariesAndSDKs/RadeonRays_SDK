# sort group histograms
KernelUtils_build_kernels_from_one_source(
    SOURCE radix_sort_group_histograms.comp
    PARAMETERS -DPP_GROUP_SIZE=256 --target-env vulkan1.1
    OUTPUTS
    "-DPP_KEYS_PER_THREAD=8: radix_sort_group_histograms.comp.spv"
    "-DPP_KEYS_PER_THREAD=4: radix_sort_group_histograms_amd.comp.spv"
)

# sort scatter keys
KernelUtils_build_kernels_from_one_source(
    SOURCE radix_sort_scatter_keys.comp
    PARAMETERS -DPP_GROUP_SIZE=256 --target-env vulkan1.1
    OUTPUTS
    "-DPP_KEYS_PER_THREAD=8: radix_sort_scatter.comp.spv"
    "-DPP_KEYS_PER_THREAD=4,-DPP_USE_SUBGROUP_OPS,-DPP_USE_AMD_SHADER_BALLOT: radix_sort_scatter_amd.comp.spv"
)

# scan exlusive add
KernelUtils_build_kernels_from_one_source(
    SOURCE scan_exclusive_add.comp
    PARAMETERS --target-env vulkan1.1
    OUTPUTS
    "-DPP_KEYS_PER_THREAD=2,-DPP_GROUP_SIZE=256: scan_exclusive_add.comp.spv"
    "-DPP_USE_SUBGROUP_OPS,-DPP_USE_AMD_SHADER_BALLOT,-DPP_KEYS_PER_THREAD=4,-DPP_GROUP_SIZE=256: scan_exclusive_add_amd.comp.spv"
)

# scan exclusive add group reduce
KernelUtils_build_kernels_from_one_source(
    SOURCE scan_exclusive_add_group_reduce.comp
    PARAMETERS --target-env vulkan1.1
    OUTPUTS
    "-DPP_KEYS_PER_THREAD=2,-DPP_GROUP_SIZE=256: scan_exclusive_add_group_reduce.comp.spv"
    "-DPP_USE_SUBGROUP_OPS,-DPP_USE_AMD_SHADER_BALLOT,-DPP_KEYS_PER_THREAD=4,-DPP_GROUP_SIZE=256: scan_exclusive_add_group_reduce_amd.comp.spv"
)

# lbvh kernels
KernelUtils_build_kernels(
    SOURCES
    lbvh_calc_morton_codes_scene.comp
    lbvh_emit_hierarchy_scene.comp
    lbvh_fit_aabb_scene.comp
    lbvh_init_scene.comp
    lbvh_calc_scene_aabb.comp
    PARAMETERS -DRR_GROUP_SIZE=128 -DPRIMITIVES_PER_THREAD=8 --target-env vulkan1.1
)

# lbvh kernels
KernelUtils_build_kernels(
    SOURCES
    lbvh_calc_morton_codes_mesh.comp
    lbvh_emit_hierarchy_mesh.comp
    lbvh_fit_aabb_mesh.comp
    lbvh_init_mesh.comp
    lbvh_calc_mesh_aabb.comp
    PARAMETERS -DRR_GROUP_SIZE=128 -DPRIMITIVES_PER_THREAD=8 --target-env vulkan1.1
)

# lbvh kernels
KernelUtils_build_kernels(
    SOURCES
    find_treelet_roots.comp
    init_primitive_count.comp
    restructure_bvh.comp
    PARAMETERS -DRR_GROUP_SIZE=64 --target-env vulkan1.1
)

KernelUtils_build_kernels_from_one_source(
    SOURCE lbvh_fit_aabb_mesh.comp
    PARAMETERS -DRR_GROUP_SIZE=128 -DPRIMITIVES_PER_THREAD=8 --target-env vulkan1.1
    OUTPUTS
    "-DUPDATE_KERNEL: lbvh_update_mesh.comp.spv"
)
KernelUtils_build_kernels_from_one_source(
    SOURCE lbvh_reset_update_flags.comp
    PARAMETERS -DRR_GROUP_SIZE=128 -DPRIMITIVES_PER_THREAD=8 --target-env vulkan1.1
    OUTPUTS
    "-DUPDATE_KERNEL: lbvh_reset_update_flags.comp.spv"
)

KernelUtils_build_kernels_from_one_source(
    SOURCE isect.comp
    PARAMETERS -DRR_GROUP_SIZE=128 --target-env vulkan1.1
    OUTPUTS
    "-DRR_OUTPUT_TYPE_FULL_HIT, -DRR_QUERY_CLOSEST: trace_geometry_full_closest.comp.spv"
    "-DRR_OUTPUT_TYPE_FULL_HIT, -DRR_QUERY_ANY: trace_geometry_full_any.comp.spv"
    "-DRR_OUTPUT_TYPE_INSTANCE, -DRR_QUERY_CLOSEST: trace_geometry_instance_closest.comp.spv"
    "-DRR_OUTPUT_TYPE_INSTANCE, -DRR_QUERY_ANY: trace_geometry_instance_any.comp.spv"
    "-DRR_OUTPUT_TYPE_FULL_HIT, -DRR_QUERY_CLOSEST, -DRR_INDIRECT_KERNEL: trace_geometry_full_closest_i.comp.spv"
    "-DRR_OUTPUT_TYPE_FULL_HIT, -DRR_QUERY_ANY, -DRR_INDIRECT_KERNEL: trace_geometry_full_any_i.comp.spv"
    "-DRR_OUTPUT_TYPE_INSTANCE, -DRR_QUERY_CLOSEST, -DRR_INDIRECT_KERNEL: trace_geometry_instance_closest_i.comp.spv"
    "-DRR_OUTPUT_TYPE_INSTANCE, -DRR_QUERY_ANY, -DRR_INDIRECT_KERNEL: trace_geometry_instance_any_i.comp.spv"
)

KernelUtils_build_kernels_from_one_source(
    SOURCE isect_2l.comp
    PARAMETERS -DRR_GROUP_SIZE=128 --target-env vulkan1.1
    OUTPUTS
    "-DRR_OUTPUT_TYPE_FULL_HIT, -DRR_QUERY_CLOSEST: trace_scene_full_closest.comp.spv"
    "-DRR_OUTPUT_TYPE_FULL_HIT, -DRR_QUERY_ANY: trace_scene_full_any.comp.spv"
    "-DRR_OUTPUT_TYPE_INSTANCE, -DRR_QUERY_CLOSEST: trace_scene_instance_closest.comp.spv"
    "-DRR_OUTPUT_TYPE_INSTANCE, -DRR_QUERY_ANY: trace_scene_instance_any.comp.spv"
    "-DRR_OUTPUT_TYPE_FULL_HIT, -DRR_QUERY_CLOSEST, -DRR_INDIRECT_KERNEL: trace_scene_full_closest_i.comp.spv"
    "-DRR_OUTPUT_TYPE_FULL_HIT, -DRR_QUERY_ANY, -DRR_INDIRECT_KERNEL: trace_scene_full_any_i.comp.spv"
    "-DRR_OUTPUT_TYPE_INSTANCE, -DRR_QUERY_CLOSEST, -DRR_INDIRECT_KERNEL: trace_scene_instance_closest_i.comp.spv"
    "-DRR_OUTPUT_TYPE_INSTANCE, -DRR_QUERY_ANY, -DRR_INDIRECT_KERNEL: trace_scene_instance_any_i.comp.spv"
)

KernelUtils_add_build_kernel_target(radeonrays)

if (EMBEDDED_KERNELS)
    add_executable(spv_bin_reader spv_bin_reader.cpp)
    target_compile_features(spv_bin_reader PRIVATE cxx_std_17)
    if (UNIX AND NOT APPLE)
        target_link_libraries(spv_bin_reader PUBLIC stdc++fs)
    elseif (APPLE)
        target_link_libraries(spv_bin_reader PUBLIC c++experimental)
    endif ()


    set(COMMON_HEADER "compiled_spv.h")
    set(COMMON_HEADER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../${COMMON_HEADER}")
    set(COMMON_MAP_HEADER "compiled_map_spv.h")
    set(COMMON_MAP_HEADER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../${COMMON_MAP_HEADER}")
    set(PARSE_CMD spv_bin_reader . . kernels ${COMMON_HEADER_PATH} ${COMMON_MAP_HEADER_PATH})
    FILE(GLOB SHADERS *.spv)

    add_custom_command(
        OUTPUT ${COMMON_HEADER_PATH}
        COMMAND ${PARSE_CMD}
        DEPENDS spv_bin_reader radeonraysBuildKernels ${SHADERS}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Parsing kernels to header"
        VERBATIM)
    
    add_custom_target(PARSE_TARGET DEPENDS ${COMMON_HEADER_PATH})
    add_dependencies(radeonrays PARSE_TARGET)

endif(EMBEDDED_KERNELS)