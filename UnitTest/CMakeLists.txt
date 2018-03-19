project(UnitTest CXX)

set(SOURCES 
    test_main.cpp
    tiny_obj_loader.cpp
    utils.cpp)

add_executable(UnitTest ${SOURCES})

target_link_libraries(UnitTest PRIVATE GTest RadeonRays Calc)
#Add root for unittests. They use private headers
target_include_directories(UnitTest PRIVATE ${RadeonRaysSDK_SOURCE_DIR})

if (RR_SHARED_CALC)
    target_compile_definitions(UnitTest PRIVATE CALC_IMPORT_API)
else (NOT RR_SHARED_CALC)
    target_compile_definitions(UnitTest PRIVATE CALC_STATIC_LIBRARY)
endif (RR_SHARED_CALC)


target_compile_features(UnitTest PRIVATE cxx_std_11)
if (APPLE)
    target_compile_options(UnitTest PRIVATE -stdlib=libc++)
endif (APPLE)

if (RR_USE_EMBREE)
    target_compile_definitions(UnitTest PRIVATE USE_EMBREE=1)
endif (RR_USE_EMBREE)

if (RR_USE_OPENCL)
    target_compile_definitions(UnitTest PRIVATE USE_OPENCL=1)
endif (RR_USE_OPENCL)

if (RR_USE_VULKAN)
    target_compile_definitions(UnitTest PRIVATE USE_VULKAN=1)
endif (RR_USE_VULKAN)


if (RR_ENABLE_RAYMASK) 
    target_compile_definitions(UnitTest PRIVATE RR_RAY_MASK)
endif (RR_ENABLE_RAYMASK)
