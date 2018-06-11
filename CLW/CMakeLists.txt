project(CLW CXX)

set(SOURCES
    CLWCommandQueue.cpp
    CLWContext.cpp
    CLWDevice.cpp
    CLWEvent.cpp
    CLWImage2D.cpp
    CLWKernel.cpp
    CLWParallelPrimitives.cpp
    CLWPlatform.cpp
    CLWProgram.cpp
    ParameterHolder.cpp
    ReferenceCounter.cpp
    CLW.h
    CLWBuffer.h
    CLWCommandQueue.h
    CLWContext.h
    CLWDevice.h
    CLWEvent.h
    CLWExcept.h
    CLWImage2D.h
    CLWKernel.h
    CLWParallelPrimitives.h
    CLWPlatform.h
    CLWProgram.h
    ParameterHolder.h
    ReferenceCounter.h
    )
    
set(SHADER CL/CLW.cl)
source_group("CL" ${SHADER})
add_library(CLW STATIC ${SOURCES} ${SHADER})

if (UNIX)
    target_compile_options(CLW PUBLIC -std=c++11 -fPIC)
elseif (APPLE)
    target_compile_options(CLW PUBLIC -std=c++11 -stdlib=libc++)
endif (UNIX)



if (RR_ALLOW_CPU_DEVICES)
    target_compile_definitions(CLW PRIVATE RR_ALLOW_CPU_DEVICES=1)
endif(RR_ALLOW_CPU_DEVICES)

#CLW always embed its kernels
target_compile_definitions(CLW PRIVATE RR_EMBED_KERNELS=1)
add_custom_command(
    OUTPUT "${CLW_BINARY_DIR}/clwkernels_cl.h"
    COMMAND "${PYTHON_EXECUTABLE}" "${STRINGIFY_SCRIPT}" "${CLW_SOURCE_DIR}/CL/" .cl opencl > "${CLW_BINARY_DIR}/clwkernels_cl.h"
    MAIN_DEPENDENCY "${SHADER}"
    COMMENT "Generating CLW kernel cache"
    VERBATIM)
target_include_directories(CLW PRIVATE "${CLW_BINARY_DIR}")
target_include_directories(CLW PUBLIC .)
add_custom_target(clw_kernel_cache_h DEPENDS "${CLW_BINARY_DIR}/clwkernels_cl.h")

add_dependencies(CLW clw_kernel_cache_h)

target_link_libraries(CLW PUBLIC OpenCL::OpenCL)

