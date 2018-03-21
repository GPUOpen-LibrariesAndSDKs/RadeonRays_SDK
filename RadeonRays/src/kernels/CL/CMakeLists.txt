add_custom_command(
    OUTPUT ${RadeonRays_BINARY_DIR}/kernels_cl.h
    COMMAND ${PYTHON_EXECUTABLE} ${STRINGIFY_SCRIPT} ${RadeonRays_SOURCE_DIR}/src/kernels/CL/ .cl opencl > ${RadeonRays_BINARY_DIR}/kernels_cl.h
    MAIN_DEPENDENCY ${SHADER}
    COMMENT "Generating RadeonRays OpenCL kernel cache"
    VERBATIM)
add_custom_target(RadeonRaysKernelCache DEPENDS "${RadeonRays_BINARY_DIR}/kernels_cl.h")

