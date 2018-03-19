project(Calc CXX)

set(SOURCES src/calc.cpp)
    
if (RR_USE_OPENCL)
    list(APPEND SOURCES 
        src/calc_clw.cpp
        src/device_clw.cpp)
elseif (RR_USE_VULKAN)
    list(APPEND SOURCES 
        src/calc_vkw.cpp
        src/device_vkw.cpp)
endif (RR_USE_OPENCL)

if (RR_SHARED_CALC)
    add_library(Calc SHARED ${SOURCES})
    target_compile_definitions(Calc PUBLIC CALC_EXPORT_API)
else (NOT RR_SHARED_CALC)
    add_library(Calc STATIC ${SOURCES})
    target_compile_definitions(Calc PUBLIC CALC_STATIC_LIBRARY)
endif (RR_SHARED_CALC)

target_include_directories(Calc PUBLIC inc)
target_include_directories(Calc PRIVATE .)

if (UNIX)
    target_compile_options(Calc PUBLIC -std=c++11 -fPIC)
    target_link_libraries(RadeonRays INTERFACE "-Wl,--no-undefined")
elseif (APPLE)
    target_compile_options(Calc PUBLIC -std=c++11 -stdlib=libc++)
endif (UNIX)

if (RR_USE_OPENCL)
    target_link_libraries(Calc PUBLIC CLW)
endif (RR_USE_OPENCL)
