cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
project(tonlib_multiclient CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS FALSE)

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
set(PY_TONLIB_MULTICLIENT TRUE CACHE BOOL "Build python bindings")
set(TONLIB_MULTICLIENT_EXAMPLES TRUE CACHE BOOL "Build examples")
set(TON_HTTP_API_CPP TRUE CACHE BOOL "Build TON HTTP API")

add_subdirectory(external/ton EXCLUDE_FROM_ALL)

add_subdirectory(tonlib-multiclient)

if (TONLIB_MULTICLIENT_EXAMPLES)
  add_subdirectory(examples)
endif()

if (PY_TONLIB_MULTICLIENT)
  add_subdirectory(external/pybind11 EXCLUDE_FROM_ALL)
  add_subdirectory(py)
endif()

if (TON_HTTP_API_CPP)
  # userver config
  set(USERVER_FEATURE_REDIS TRUE)
  set(USERVER_FEATURE_UBOOST_CORO TRUE)
  set(USERVER_FEATURE_CHAOTIC TRUE)
  set(USERVER_FEATURE_CHAOTIC_EXPERIMENAL TRUE)
  set(USERVER_FEATURE_UTEST FALSE)
  set(USERVER_FEATURE_TESTSUITE FALSE)
  set(USERVER_USE_STATIC_LIBS TRUE)
  add_subdirectory(external/userver EXCLUDE_FROM_ALL)
  add_subdirectory(ton-http-api)
endif()