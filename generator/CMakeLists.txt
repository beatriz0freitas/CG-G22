cmake_minimum_required(VERSION 3.10)
project(generator CXX)

set(CMAKE_CXX_STANDARD 17)

add_executable(generator
    main.cpp
    filewriter.cpp
    primitives/plane.cpp
    primitives/box.cpp
    primitives/sphere.cpp
    primitives/cone.cpp
    primitives/cylinder.cpp
    primitives/torus.cpp
    primitives/bezier.cpp
)

target_include_directories(generator PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Define _USE_MATH_DEFINES para M_PI no Windows
if(WIN32)
    target_compile_definitions(generator PRIVATE _USE_MATH_DEFINES)
endif()

# libm só existe em Unix/Linux; no Windows a matemática está na CRT
if(NOT WIN32)
    target_link_libraries(generator m)
endif()