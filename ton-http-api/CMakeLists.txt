cmake_minimum_required(VERSION 3.16)
project(ton-http-api-cpp CXX)

userver_setup_environment()

file(READ openapi/openapi.html OPENAPI_HTML_CONTENT)
file(READ openapi/openapi.json OPENAPI_JSON_CONTENT)
configure_file(openapi/openapi_page.hpp.in ${CMAKE_CURRENT_SOURCE_DIR}/openapi/openapi_page.hpp @ONLY)

set(TON_HTTP_API_CPP_SOURCE
    main.cpp
    tonlib_component.cpp
    tonlib_component.h
    tonlib_worker.cpp
    tonlib_worker.h
    handler_api_v2.cpp
    handler_api_v2.h
    tonlib_postprocessor.cpp
    utils.cpp
    tokens-tlb.cpp
    cache.cpp
    request.hpp
)

# generate tokens schemas
set(TLB_TOKENS
        ${CMAKE_CURRENT_SOURCE_DIR}/tokens-tlb.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tokens-tlb.h
)

add_custom_command(
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND tlbc -o tokens-tlb -n tokens::gen -z tlb/tokens.tlb
        COMMENT "Generate tokes tlb source files"
        OUTPUT ${TLB_TOKENS}
        DEPENDS tlbc tlb/tokens.tlb
)

add_custom_target(tlb_generate_tokens DEPENDS ${TLB_TOKENS})

add_executable(${PROJECT_NAME} ${TON_HTTP_API_CPP_SOURCE})
target_include_directories(${PROJECT_NAME}
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../external/userver
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../external/ton
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_dependencies(${PROJECT_NAME} tlb_generate_tokens)


target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
target_link_libraries(${PROJECT_NAME} userver::core tonlib::multiclient)
target_link_options(ton-http-api-cpp PUBLIC -rdynamic)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
