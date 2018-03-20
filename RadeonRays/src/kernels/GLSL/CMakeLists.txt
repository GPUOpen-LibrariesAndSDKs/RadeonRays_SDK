add_custom_command(
    OUTPUT ${RadeonRays_BINARY_DIR}/kernels_vk.h
    COMMAND ${PYTHON_EXECUTABLE} ${STRINGIFY_SCRIPT} ${RadeonRays_SOURCE_DIR}/src/kernels/GLSL/ .comp vulkan > ${RadeonRays_BINARY_DIR}/kernels_vk.h
    MAIN_DEPENDENCY ${SHADER}
    COMMENT "Generating RadeonRays Vulkan kernel cache"
    VERBATIM)
add_custom_target(RadeonRaysKernelCache DEPENDS "${RadeonRays_BINARY_DIR}/kernels_vk.h")

