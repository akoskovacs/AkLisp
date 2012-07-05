project(AkLisp)
cmake_minimum_required(VERSION 2.8)

set(SOURCES
    aklisp.c
    lib.c
    memory.c
    parser.c
    lexer.c)

set(HEADERS tree.h aklisp.h)

add_executable(aklisp ${SOURCES})
