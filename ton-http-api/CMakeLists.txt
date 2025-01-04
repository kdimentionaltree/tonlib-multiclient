cmake_minimum_required(VERSION 3.16)
project(ton-http-api-cpp CXX)

set(USERVER_FEATURE_REDIS TRUE)
set(USERVER_FEATURE_UBOOST_CORO TRUE)
userver_setup_environment()

file(READ openapi/openapi.html OPENAPI_HTML_CONTENT)
file(READ openapi/openapi.json OPENAPI_JSON_CONTENT)
configure_file(openapi/openapi_page.hpp.in ${CMAKE_CURRENT_SOURCE_DIR}/openapi/openapi_page.hpp @ONLY)

set(TON_HTTP_API_CPP_SOURCE
    main.cpp
    handlers.hpp
    handlers.cpp
    tonlib_component.cpp
    tonlib_component.h
    tonlib_worker.cpp
    tonlib_worker.h
    handler_api_v2.cpp
    handler_api_v2.h
    tonlib_postprocessor.cpp
        utils.hpp
)

add_executable(${PROJECT_NAME} ${TON_HTTP_API_CPP_SOURCE})
target_include_directories(${PROJECT_NAME}
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../external/userver
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../external/ton
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..
)
#target_link_directories(ton-http-api-cpp
#        PRIVATE external/userver
#        PRIVATE external/ton
#)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
target_link_libraries(${PROJECT_NAME} userver::core tonlib::multiclient)
target_link_options(ton-http-api-cpp PUBLIC -rdynamic)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
