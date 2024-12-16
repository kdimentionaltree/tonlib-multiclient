cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(tonlib_multiclient CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS FALSE)

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
set(PY_TONLIB_MULTICLIENT FALSE CACHE BOOL "Build python bindings")
set(TON_HTTP_API_CPP TRUE CACHE BOOL "Build TON HTTP API")

add_subdirectory(external/ton EXCLUDE_FROM_ALL)

add_subdirectory(tonlib-multiclient)
add_subdirectory(examples)

if (PY_TONLIB_MULTICLIENT)
  add_subdirectory(external/pybind11 EXCLUDE_FROM_ALL)
  add_subdirectory(py)
endif()

if (TON_HTTP_API_CPP)
  add_subdirectory(external/userver EXCLUDE_FROM_ALL)
  add_subdirectory(ton-http-api)
endif()