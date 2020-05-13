cmake_minimum_required(VERSION 3.12)

project(app_vk CXX)

find_package(Vulkan REQUIRED)

add_executable(app_vk
    main.cpp
    application.h
    mesh_data.h
    stb_image_write.h
    tiny_obj_loader.h
    tiny_obj_loader.cc)

target_include_directories(app_vk PRIVATE
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/../include
)

target_link_directories(app_vk PRIVATE
    ${PROJECT_SOURCE_DIR}/../bin
    ${PROJECT_SOURCE_DIR}/../lib
)

target_link_libraries(app_vk PRIVATE radeonrays Vulkan::Vulkan)
