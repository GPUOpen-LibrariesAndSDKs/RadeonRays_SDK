cmake_minimum_required(VERSION 3.12)

project(app_mtl CXX)

add_executable(app_mtl
    application.h
    main.mm
    mesh_data.h
    stb_image_write.h
    tiny_obj_loader.cc
    tiny_obj_loader.h
    )

target_compile_features(app_mtl PRIVATE cxx_std_17)

target_include_directories(app_mtl PRIVATE
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/../include
)

target_link_directories(app_mtl PRIVATE
    ${PROJECT_SOURCE_DIR}/../bin
)

target_link_libraries(app_mtl PRIVATE radeonrays
                      "-framework Foundation"
                      "-framework Metal"
                      "-framework MetalPerformanceShaders")
