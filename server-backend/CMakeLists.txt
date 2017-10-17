project(Postr)
cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

SET(CMAKE_CXX_FLAGS "-std=c++0x")

add_subdirectory(src/pipeline)
add_subdirectory(src/editor)
