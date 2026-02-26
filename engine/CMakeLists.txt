cmake_minimum_required(VERSION 3.10)
project(engine CXX)

set(CMAKE_CXX_STANDARD 17)

find_package(OpenGL REQUIRED)

# No Windows o freeglut pode estar em localizações não-standard
if(WIN32)
    list(APPEND CMAKE_PREFIX_PATH
        "C:/freeglut"
        "C:/Program Files/freeglut"
        "C:/vcpkg/installed/x64-windows"
    )
endif()
find_package(GLUT REQUIRED)

# Expat: tentar find_package primeiro (vcpkg/conan), fallback para find_library
find_package(EXPAT QUIET)
if(NOT EXPAT_FOUND)
    find_library(EXPAT_LIB NAMES expat libexpat expat_static REQUIRED)
    find_path   (EXPAT_INC expat.h)
    set(EXPAT_LIBRARIES    ${EXPAT_LIB})
    set(EXPAT_INCLUDE_DIRS ${EXPAT_INC})
endif()

add_executable(engine
    main.cpp
    xmlParser.cpp
    renderer.cpp
)

target_include_directories(engine PRIVATE
    ${OPENGL_INCLUDE_DIR}
    ${GLUT_INCLUDE_DIR}
    ${EXPAT_INCLUDE_DIRS}
)

target_link_libraries(engine
    ${OPENGL_LIBRARIES}
    ${GLUT_LIBRARIES}
    ${EXPAT_LIBRARIES}
)

# libm só existe em Unix/Linux
if(NOT WIN32)
    target_link_libraries(engine m)
endif()