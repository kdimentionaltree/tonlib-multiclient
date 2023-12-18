cmake_minimum_required(VERSION 3.21)

project(tonlib-multiclient)

add_subdirectory(external/ton EXCLUDE_FROM_ALL)

# tonlib-multiclient
add_executable(tonlib-multiclient 
    src/main.cpp
    src/TonlibMultiClient.cpp
)

target_include_directories(tonlib-multiclient 
    PUBLIC external/ton
    PUBLIC src/
)

target_link_directories(tonlib-multiclient 
    PUBLIC external/ton
)

target_compile_features(tonlib-multiclient PRIVATE cxx_std_17)
target_link_libraries(tonlib-multiclient tdutils tdactor adnllite tl_api ton_crypto ton_block tl_tonlib_api tonlib)
