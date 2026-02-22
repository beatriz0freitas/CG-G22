cmake_minimum_required(VERSION 3.10)
project(engine CXX)

set(CMAKE_CXX_STANDARD 17)

find_package(OpenGL REQUIRED)
find_package(GLUT   REQUIRED)

find_library(EXPAT_LIB expat REQUIRED)
find_path   (EXPAT_INC expat.h)

add_executable(engine
    main.cpp
    xmlParser.cpp
    renderer.cpp
)

target_include_directories(engine PRIVATE
    ${OPENGL_INCLUDE_DIR}
    ${GLUT_INCLUDE_DIR}
    ${EXPAT_INC}
)

target_link_libraries(engine
    ${OPENGL_LIBRARIES}
    ${GLUT_LIBRARIES}
    ${EXPAT_LIB}
    m
)