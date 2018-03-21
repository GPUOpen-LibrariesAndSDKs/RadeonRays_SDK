project (TutorialCornellBox CXX)

set(SOURCES main.cpp
    $<TARGET_OBJECTS:TutorialTools>)
    
#message(FATAL_ERROR $<TARGET_OBJECTS:TutorialTools>)

add_executable(TutorialCornellBox ${SOURCES})
target_compile_features(TutorialCornellBox PRIVATE cxx_std_11)
target_link_libraries(TutorialCornellBox PRIVATE RadeonRays)
if (APPLE OR UNIX)
    target_link_libraries(TutorialCornellBox PRIVATE OpenGL::GL GLUT::GLUT GLEW::GLEW ${OIIO_LIBS})
elseif (WIN32)
    target_include_directories(TutorialCornellBox 
        PRIVATE ${GLUT_INCLUDES} 
        PRIVATE ${GLEW_INCLUDES} 
        PRIVATE ${OIIO_INCLUDES})
    target_link_libraries(TutorialCornellBox PRIVATE OpenGL::GL ${GLUT_LIBS} ${GLEW_LIBS} ${OIIO_LIBS})
endif (APPLE OR UNIX)
