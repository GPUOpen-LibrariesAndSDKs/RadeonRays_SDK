cmake_minimum_required(VERSION 3.12)

project(app_dx CXX)

add_executable(app_dx
    main.cpp
    application.h
    tiny_obj_loader.h
    tiny_obj_loader.cc
    mesh_data.h
    stb_image_write.h
    dx/dxassist.h
    dx/dxassist.cpp
    dx/shader_compiler.h
    dx/shader_compiler.cpp
    dx/common.h
    dx/common.cpp)

target_include_directories(app_dx PRIVATE
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/../include
)

target_link_directories(app_dx PRIVATE
    ${PROJECT_SOURCE_DIR}/../bin
    ${PROJECT_SOURCE_DIR}/../lib
)

target_link_libraries(app_dx PRIVATE radeonrays d3d12 dxgi dxguid)
