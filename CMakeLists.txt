project(AkLisp)
cmake_minimum_required(VERSION 2.8)

if (unix)
    set(PLATSRC os_unix.c)
else (unix)
    set(PLATSRC os_other.c)
endif (unix)

set(SOURCES 
    ${PLATSRC}
    aklisp.c
    lib.c
    gc.c
    parser.c
    lexer.c)

set(HEADERS tree.h aklisp.h)
set(TARGET aklisp)
include(CheckIncludeFiles)
check_include_files(ucontext.h HAVE_UCONTEXT_H)
check_include_files(execinfo.h HAVE_EXECINFO_H)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
add_library(${TARGET}_static STATIC ${SOURCES})
add_library(${TARGET}_shared SHARED ${SOURCES})
set_target_properties(${TARGET}_static PROPERTIES VERSION 0.1.0 SOVERSION 1)
set_target_properties(${TARGET}_shared PROPERTIES VERSION 0.1.0 SOVERSION 1)

add_executable(${TARGET} main.c)
target_link_libraries(${TARGET} ${TARGET}_static)
set_target_properties(${TARGET} PROPERTIES OUTPUT_NAME aklisp)
INSTALL(TARGETS ${TARGET}
        RUNTIME
        DESTINATION bin
        PERMISSIONS OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
INSTALL(TARGETS ${TARGET}_static
        DESTINATION lib
        PERMISSIONS OWNER_READ GROUP_READ WORLD_READ)
INSTALL(TARGETS ${TARGET}_shared
        DESTINATION bin
        PERMISSIONS OWNER_READ GROUP_READ WORLD_READ)
