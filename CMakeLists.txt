project(AkLisp)
cmake_minimum_required(VERSION 2.8)

set(SOURCES
    aklisp.c
    lib.c
    gc.c
    parser.c
    lexer.c)

set(HEADERS tree.h aklisp.h)
set(TARGET aklisp)

add_executable(${TARGET} ${SOURCES})
set_target_properties(${TARGET} PROPERTIES OUTPUT_NAME aklisp)
INSTALL(TARGETS ${TARGET}
        RUNTIME
        DESTINATION bin
        PERMISSIONS OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
