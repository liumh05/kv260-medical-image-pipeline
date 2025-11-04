cmake_minimum_required(VERSION 3.15)
project(marching_cubes_cpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(marching_cubes
    src/main.cpp
    src/npy_reader.cpp
    src/marching_cubes.cpp
    src/vtk_writer.cpp
)

target_include_directories(marching_cubes PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Enable higher warnings if using GCC/Clang
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(marching_cubes PRIVATE -Wall -Wextra -Wpedantic)
endif()

