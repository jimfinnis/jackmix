cmake_minimum_required(VERSION 2.6)
project(jackmix)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
find_package(Jack REQUIRED)
find_package(Threads REQUIRED)
find_package(Curses REQUIRED)

include_directories(${JACK_INCLUDE_DIR} ${CURSES_INCLUDE_DIR} 
    ${CMAKE_BINARY_DIR} .)
add_definitions(${JACK_DEFINITIONS} -Wall)

set(SOURCES main.cpp tokeniser.cpp tokens.cpp ctrl.cpp value.cpp
    channel.cpp diamond.cpp fx.cpp plugins.cpp monitor.cpp)

add_custom_command(
    OUTPUT tokens.cpp tokens.h
    DEPENDS tokens
    COMMAND python ${CMAKE_SOURCE_DIR}/gentoks ${CMAKE_SOURCE_DIR}/tokens tokens)

add_executable(jackmix ${SOURCES})

target_link_libraries(jackmix ${JACK_LIBRARIES} ${CURSES_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT} -lm -ldiamondapparatus -lpthread dl)
target_compile_options(jackmix PUBLIC "-pthread")
