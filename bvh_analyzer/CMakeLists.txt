find_package(OpenMP)

file(GLOB HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(bvh_analyzer ${HEADER_FILES} ${SOURCE_FILES})

target_link_libraries(bvh_analyzer PRIVATE project_options)
if(OpenMP_CXX_FOUND)
    target_link_libraries(bvh_analyzer PRIVATE OpenMP::OpenMP_CXX)
endif()