cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(tonlib_multiclient_lib CXX)

set(TONLIB_MULTICLIENT_LIB_SOURCE
    multi_client.cpp
    multi_client_actor.cpp
    client_wrapper.cpp
)

add_library(${PROJECT_NAME} SHARED ${TONLIB_MULTICLIENT_LIB_SOURCE})
add_library(tonlib::multiclient ALIAS ${PROJECT_NAME})

target_link_libraries(
  ${PROJECT_NAME}
  tonlib tdactor
  tl_api tl_tonlib_api_json tl_tonlib_api tl_lite_api tl-lite-utils
  tdutils ton_crypto ton_block
)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
add_dependencies(${PROJECT_NAME} tl_generate_common)
