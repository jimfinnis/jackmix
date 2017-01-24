cmake_minimum_required(VERSION 2.6)

project(JACKMIXLOCALS)

set(LOCALSFILES)
macro(add_locals_files)
    foreach(file ${ARGV})
        add_custom_command(
            OUTPUT ${file}.c
            DEPENDS ${file}
            COMMAND pwd
            COMMAND perl ${CMAKE_SOURCE_DIR}/locals/makestublocal.pl ${CMAKE_SOURCE_DIR}/locals/${file}>${file}.c
        )
        set(LOCALSFILES ${LOCALSFILES} ${file}.c)
        message(${LOCALSFILES})
    endforeach(file)
endmacro(add_locals_files)

# so we can get the ladspa.h
include_directories(..)

add_locals_files(mix.xml)

# I'd love to generate the list automatically, but argh.

add_library(locals ${LOCALSFILES} filelist.cpp)

