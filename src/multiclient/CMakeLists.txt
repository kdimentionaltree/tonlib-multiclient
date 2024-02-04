cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(tonlib_multiclient_lib CXX)

add_library(${PROJECT_NAME} STATIC
  multi_client.cpp
  multi_client_actor.cpp
  client_wrapper.cpp
)
add_library(tonlib::multiclient ALIAS ${PROJECT_NAME})

target_link_libraries(
  ${PROJECT_NAME}
  tonlib tdactor
  tl_tonlib_api tl_tonlib_api_json tl_lite_api tl-lite-utils
  tdutils ton_crypto ton_block
)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
