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
    ReferenceCounter.cpp)
    
add_library(CLW STATIC ${SOURCES})

if (UNIX)
    target_compile_options(CLW PUBLIC -std=c++11 -fPIC)
elseif (APPLE)
    target_compile_options(CLW PUBLIC -std=c++11 -stdlib=libc++)
endif (UNIX)

if (RR_ALLOW_CPU_DEVICES)
    target_compile_definitions(CLW PUBLIC RR_ALLOW_CPU_DEVICES=1)
endif(RR_ALLOW_CPU_DEVICES)

target_link_libraries(CLW PUBLIC OpenCL::OpenCL)

