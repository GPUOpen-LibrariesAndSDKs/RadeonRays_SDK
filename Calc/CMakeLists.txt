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
        PRIVATE ${RadeonRaysSDK_SOURCE_DIR}/Anvil/deps
        PRIVATE ${RadeonRaysSDK_SOURCE_DIR}/Anvil/include)
    target_link_libraries(Calc PUBLIC Vulkan::Vulkan)
endif (RR_USE_VULKAN)
