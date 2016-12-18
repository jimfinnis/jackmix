cmake_minimum_required(VERSION 2.6)
project(jackmix)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
find_package(Jack REQUIRED)
find_package(Threads REQUIRED)

include_directories(${JACK_INCLUDE_DIR} ${CMAKE_BINARY_DIR} .)
add_definitions(${JACK_DEFINITIONS} -Wall)

set(SOURCES main.cpp tokeniser.cpp tokens.cpp)

add_custom_command(
    OUTPUT tokens.cpp tokens.h
    DEPENDS tokens
    COMMAND python ${CMAKE_SOURCE_DIR}/gentoks ${CMAKE_SOURCE_DIR}/tokens tokens)

add_executable(jackmix ${SOURCES})

target_link_libraries(jackmix ${JACK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
target_compile_options(jackmix PUBLIC "-pthread")
