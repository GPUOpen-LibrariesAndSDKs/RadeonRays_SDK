project(Calc CXX)

set(SOURCES src/calc.cpp)
set(PUBLIC_HEADERS
    inc/buffer.h
    inc/calc.h
    inc/calc_common.h
    inc/device.h
    inc/event.h
    inc/except.h
    inc/executable.h
    inc/primitives.h
    )

if (RR_USE_OPENCL)
    list(APPEND SOURCES 
        src/calc_clw.cpp
        src/device_clw.cpp
        src/calc_clw.h
        src/calc_clw_common.h
        src/device_clw.h
        src/except_clw.h
        )
    list(APPEND PUBLIC_HEADERS
        inc/calc_cl.h
        inc/device_cl.h
        )
elseif (RR_USE_VULKAN)
    list(APPEND SOURCES 
        src/calc_vkw.cpp
        src/device_vkw.cpp
        src/calc_vkw.h
        src/device_vkw.h
        src/event_vk.h
        src/common_vk.h
        src/except_vk.h
        src/executable_vk.h
        src/function_vk.h
        )
    list(APPEND PUBLIC_HEADERS
        inc/buffer_vk.h
        inc/calc_vk.h
        inc/device_vk.h
        )
endif (RR_USE_OPENCL)

if (RR_SHARED_CALC)
    add_library(Calc SHARED ${SOURCES} ${PUBLIC_HEADERS})
    target_compile_definitions(Calc PUBLIC CALC_EXPORT_API)
else (NOT RR_SHARED_CALC)
    add_library(Calc STATIC ${SOURCES} ${PUBLIC_HEADERS})
    target_compile_definitions(Calc PUBLIC CALC_STATIC_LIBRARY)
endif (RR_SHARED_CALC)

target_include_directories(Calc PUBLIC inc)
target_include_directories(Calc PRIVATE .)

target_compile_features(Calc PRIVATE cxx_std_11)

if (UNIX AND NOT APPLE)
    target_compile_options(Calc PUBLIC -fPIC)
    target_link_libraries(Calc INTERFACE "-Wl,--no-undefined")
elseif (APPLE)
    target_compile_options(Calc PUBLIC -stdlib=libc++)
endif (UNIX AND NOT APPLE)

if (RR_USE_OPENCL)
    target_link_libraries(Calc PUBLIC CLW)
endif (RR_USE_OPENCL)

if (RR_USE_VULKAN)
    #Need to add Anvil to include path
    target_include_directories(Calc
        PRIVATE "${RadeonRaysSDK_SOURCE_DIR}/Anvil/deps"
        PRIVATE "${RadeonRaysSDK_SOURCE_DIR}/Anvil/include")
    target_link_libraries(Calc PUBLIC Vulkan::Vulkan)
endif (RR_USE_VULKAN)
