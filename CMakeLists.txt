cmake_minimum_required(VERSION 3.5)
project(radeonrays)

set(CMAKE_CXX_STANDARD 11 )

if( NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../Anvil/")
    message( ERROR "Anvil must live in the directory parallel to this directory to use the vulcan backend")
else()
    if( NOT HAS_ANVIL )
        message( ERROR "Anvil must have been exported prior to this CMakeList.txt file")
    endif()
endif()

FIND_PACKAGE( Threads REQUIRED )
FIND_PACKAGE( OpenCL REQUIRED )
FIND_PACKAGE( OpenGL REQUIRED )

if(UNIX)
    FIND_PACKAGE(XCB MODULE REQUIRED COMPONENTS keysyms)
    include_directories( ${LIBXCB_INCLUDE_DIR} )
endif()


set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")
add_definitions( -DUSE_VULKAN=1 )

if(UNIX)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")
    add_definitions( -DVK_USE_PLATFORM_XCB_KHR=1)
    link_directories( 3rdParty/embree/lib/x64/ )
elseif(WIN32)
    link_directories( 3rdParty/embree/lib/win32 )
    add_definitions( -DVK_USE_PLATFORM_WIN32_KHR=1)
endif()

remove_definitions( -DUSE_EMBREE )

include_directories( ${OpenCL_INCLUDE_DIR} )
include_directories( ${VULKAN_INCLUDE_DIR} )

include_directories( Anvil/include Anvil_premake )
include_directories( Anvil/deps )
include_directories( Calc/inc )
include_directories( CLW )
include_directories( RadeonRays/include )
include_directories( Gtest/include Gtest )

set( CALC_SOURCE_FILES
        Calc/inc/buffer.h
        Calc/inc/buffer_vk.h
        Calc/inc/calc.h
        Calc/inc/calc_cl.h
        Calc/inc/calc_vk.h
        Calc/inc/calc_common.h
        Calc/inc/device.h
        Calc/inc/device_cl.h
        Calc/inc/device_vk.h
        Calc/inc/event.h
        Calc/inc/except.h
        Calc/inc/executable.h
        Calc/inc/primitives.h
        Calc/src/calc.cpp
        Calc/src/calc_clw.cpp
        Calc/src/calc_clw.h
        Calc/src/calc_clw_common.h
        Calc/src/calc_vkw.cpp
        Calc/src/calc_vkw.h
        Calc/src/common_vk.h
        Calc/src/device_clw.cpp
        Calc/src/device_clw.h
        Calc/src/device_vkw.cpp
        Calc/src/device_vkw.h
        Calc/src/event_vk.h
        Calc/src/except_clw.h
        Calc/src/except_vk.h
        Calc/src/executable_vk.h
        Calc/src/function_vk.h
        )
set( CLW_SOURCE_FILES
        CLW/CL/cache/kernels.h
        CLW/CLW.h
        CLW/CLWBuffer.h
        CLW/CLWCommandQueue.cpp
        CLW/CLWCommandQueue.h
        CLW/CLWContext.cpp
        CLW/CLWContext.h
        CLW/CLWDevice.cpp
        CLW/CLWDevice.h
        CLW/CLWEvent.cpp
        CLW/CLWEvent.h
        CLW/CLWExcept.h
        CLW/CLWImage2D.cpp
        CLW/CLWImage2D.h
        CLW/CLWKernel.cpp
        CLW/CLWKernel.h
        CLW/CLWParallelPrimitives.cpp
        CLW/CLWParallelPrimitives.h
        CLW/CLWPlatform.cpp
        CLW/CLWPlatform.h
        CLW/CLWProgram.cpp
        CLW/CLWProgram.h
        CLW/ParameterHolder.cpp
        CLW/ParameterHolder.h
        CLW/ReferenceCounter.cpp
        CLW/ReferenceCounter.h

        )

set( RadeonRays_SOURCE_FILES
        RadeonRays/include/math/bbox.h
        RadeonRays/include/math/float2.h
        RadeonRays/include/math/float3.h
        RadeonRays/include/math/int2.h
        RadeonRays/include/math/mathutils.h
        RadeonRays/include/math/matrix.h
        RadeonRays/include/math/quaternion.h
        RadeonRays/include/math/ray.h
        RadeonRays/include/radeonrays.h
        RadeonRays/include/radeonrays_cl.h
        RadeonRays/include/radeonrays_vk.h
        RadeonRays/src/accelerator/bvh.cpp
        RadeonRays/src/accelerator/bvh.h
        RadeonRays/src/accelerator/hlbvh.cpp
        RadeonRays/src/accelerator/hlbvh.h
        RadeonRays/src/async/thread_pool.h
        RadeonRays/src/device/calc_holder.h
        RadeonRays/src/device/calc_intersection_device.cpp
        RadeonRays/src/device/calc_intersection_device.h
        RadeonRays/src/device/calc_intersection_device_cl.cpp
        RadeonRays/src/device/calc_intersection_device_cl.h
        RadeonRays/src/device/calc_intersection_device_vk.cpp
        RadeonRays/src/device/calc_intersection_device_vk.h
    #    RadeonRays/src/device/embree_intersection_device.cpp
    #    RadeonRays/src/device/embree_intersection_device.h
        RadeonRays/src/device/intersection_device.h
        RadeonRays/src/except/except.h
        RadeonRays/src/intersection/RadeonRays.cpp
        RadeonRays/src/intersection/RadeonRays_impl.cpp
        RadeonRays/src/intersection/RadeonRays_impl.h
        RadeonRays/src/kernel/CL/cache/kernels.h
        RadeonRays/src/primitive/instance.h
        RadeonRays/src/primitive/mesh.cpp
        RadeonRays/src/primitive/mesh.h
        RadeonRays/src/primitive/shapeimpl.h
        RadeonRays/src/strategy/bvh2lstrategy.cpp
        RadeonRays/src/strategy/bvh2lstrategy.h
        RadeonRays/src/strategy/bvhstrategy.cpp
        RadeonRays/src/strategy/bvhstrategy.h
        RadeonRays/src/strategy/fatbvhstrategy.cpp
        RadeonRays/src/strategy/fatbvhstrategy.h
        RadeonRays/src/strategy/hlbvh_strategy.cpp
        RadeonRays/src/strategy/hlbvh_strategy.h
        RadeonRays/src/strategy/strategy.h
        RadeonRays/src/translator/fatnode_bvh_translator.cpp
        RadeonRays/src/translator/fatnode_bvh_translator.h
        RadeonRays/src/translator/plain_bvh_translator.cpp
        RadeonRays/src/translator/plain_bvh_translator.h
        RadeonRays/src/util/alignedalloc.h
        RadeonRays/src/util/options.cpp
        RadeonRays/src/util/options.h
        RadeonRays/src/util/progressreporter.h
        RadeonRays/src/world/world.cpp
        RadeonRays/src/world/world.h
    )



set( UNIT_TEST_SOURCE_FILES
        Gtest/include/gtest/gtest-death-test.h
        Gtest/include/gtest/gtest-message.h
        Gtest/include/gtest/gtest-param-test.h
        Gtest/include/gtest/gtest-printers.h
        Gtest/include/gtest/gtest-spi.h
        Gtest/include/gtest/gtest-test-part.h
        Gtest/include/gtest/gtest-typed-test.h
        Gtest/include/gtest/gtest.h
        Gtest/include/gtest/gtest_pred_impl.h
        Gtest/include/gtest/gtest_prod.h
        Gtest/src/gtest-all.cc
        UnitTest/calc_test_cl.h
        UnitTest/calc_test_vk.h
        UnitTest/clw_cl_test.h
        UnitTest/clw_test.h
        UnitTest/radeonrays_cl_test.h
        UnitTest/radeonrays_conformance_test_cl.h
        UnitTest/radeonrays_conformance_test_vk.h
        UnitTest/radeonrays_performance_test_cl.h
        UnitTest/radeonrays_performance_test_vk.h
        UnitTest/radeonrays_apitest_cl.h
        UnitTest/radeonrays_apitest_vk.h
        UnitTest/test_main.cpp
        UnitTest/tiny_obj_loader.cpp
        UnitTest/tiny_obj_loader.h
    )

add_library(clw STATIC ${CLW_SOURCE_FILES} )
target_link_libraries( clw ${OpenCL_LIBRARIES} ${VULKAN_LIBRARY} )

add_library(calc STATIC ${CALC_SOURCE_FILES} )
target_link_libraries( calc clw ${OpenCL_LIBRARIES} ${VULKAN_LIBRARY} )

add_library(RadeonRays SHARED ${RadeonRays_SOURCE_FILES} )
target_link_libraries( RadeonRays Anvil calc clw ${OpenCL_LIBRARIES} ${VULKAN_LIBRARY} )
target_compile_definitions( RadeonRays PRIVATE EXPORT_API=1 )

export(TARGETS RadeonRays calc clw FILE radeonrays-exports.cmake)

add_executable(radeonrays_vulkan_unittest ${UNIT_TEST_SOURCE_FILES})
target_link_libraries(radeonrays_vulkan_unittest
        Anvil
        calc
        clw
        RadeonRays
        ${OpenCL_LIBRARIES}
        ${OPENGL_LIBRARIES}
        ${GLEW_LIBRARY}
        ${GLUT_LIBRARY}
        ${VULKAN_LIBRARY}
        )

if (WIN32)
elseif(UNIX)
    target_link_libraries( radeonrays_vulkan_unittest
            pthread
            ${LIBXCB_LIBRARIES}
            ${GLX_LIBRARIES} )
endif()