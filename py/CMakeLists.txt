cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(py_tonlib_multiclient)

find_package(Python COMPONENTS Interpreter Development)

add_library(${PROJECT_NAME} MODULE tonlib_multiclient.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE pybind11::module pybind11::lto tonlib::multiclient)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
set_property(TARGET ${PROJECT_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)
