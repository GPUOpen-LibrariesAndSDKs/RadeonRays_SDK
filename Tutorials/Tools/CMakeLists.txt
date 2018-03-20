project(Tools CXX)

set(SOURCES 
    shader_manager.cpp
    shader_manager.h
    tiny_obj_loader.cpp
    tiny_obj_loader.h)

add_library(TutorialTools OBJECT ${SOURCES})

target_compile_features(TutorialTools PRIVATE cxx_std_11)
if (WIN32)
    target_include_directories(TutorialTools 
        PRIVATE ${GLUT_INCLUDES} 
        PRIVATE ${GLEW_INCLUDES} 
        PRIVATE ${OIIO_INCLUDES})
endif (WIN32)
