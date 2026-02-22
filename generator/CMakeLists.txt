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
)

target_include_directories(generator PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(generator m)