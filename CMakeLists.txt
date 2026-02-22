cmake_minimum_required(VERSION 3.5)
project(ProjetoCG)

# Usa o padrão C++17
set(CMAKE_CXX_STANDARD 17)

# Adiciona os dois subprojetos
add_subdirectory(generator)
add_subdirectory(engine)