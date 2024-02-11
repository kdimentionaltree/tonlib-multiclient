cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(tonlib_multiclient CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS FALSE)

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

add_subdirectory(external/ton EXCLUDE_FROM_ALL)

add_subdirectory(src)
add_subdirectory(examples)
