cmake_minimum_required(VERSION 2.6)

project(JACKMIXLOCALS)

set(LOCALSFILES)
macro(add_locals_files)
    foreach(file ${ARGV})
        add_custom_command(
            OUTPUT ${file}.c
            DEPENDS ${file}
            COMMAND pwd
            COMMAND perl ../makestublocal.pl ${CMAKE_SOURCE_DIR}/${file}>${file}.c
        )
        set(LOCALSFILES ${LOCALSFILES} ${file}.c)
    endforeach(file)
endmacro(add_locals_files)

add_locals_files(mix.xml)

add_library(locals ${LOCALSFILES})

