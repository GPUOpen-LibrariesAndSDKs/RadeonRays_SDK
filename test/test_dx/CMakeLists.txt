add_executable(test_dx
    main.cpp
    algos_test.h
    basic_test.h
    hlbvh_test.h
    mesh_data.h
    stb_image_write.h
    tiny_obj_loader.h
    tiny_obj_loader.cc
    dx/dxassist.h
    dx/dxassist.cpp
    dx/common.h
    dx/common.cpp
    ${PROJECT_SOURCE_DIR}/src/core/src/dx/algorithm/radix_sort.h
    ${PROJECT_SOURCE_DIR}/src/core/src/dx/algorithm/radix_sort.cpp
    ${PROJECT_SOURCE_DIR}/src/core/src/dx/algorithm/scan.h
    ${PROJECT_SOURCE_DIR}/src/core/src/dx/algorithm/scan.cpp
    ${PROJECT_SOURCE_DIR}/src/core/src/dx/hlbvh_builder.h
    ${PROJECT_SOURCE_DIR}/src/core/src/dx/hlbvh_builder.cpp
    ${PROJECT_SOURCE_DIR}/src/core/src/dx/restructure_hlbvh.h
    ${PROJECT_SOURCE_DIR}/src/core/src/dx/restructure_hlbvh.cpp
    ${PROJECT_SOURCE_DIR}/src/core/src/dx/shader_compiler.h
    ${PROJECT_SOURCE_DIR}/src/core/src/dx/shader_compiler.cpp
    ${PROJECT_SOURCE_DIR}/src/core/src/dx/update_hlbvh.h
    ${PROJECT_SOURCE_DIR}/src/core/src/dx/update_hlbvh.cpp
)

target_include_directories(test_dx PRIVATE
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/src/core/src
)

target_link_libraries(test_dx PRIVATE radeonrays project_options gtest d3d12 dxgi dxguid)
