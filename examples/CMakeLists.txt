cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

add_executable(tonlib_multiclient_sync_example_bin main.cpp)
target_link_libraries(tonlib_multiclient_sync_example_bin PUBLIC tonlib::multiclient)
target_include_directories(tonlib_multiclient_sync_example_bin PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../src)
