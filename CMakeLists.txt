project(AkLisp)
cmake_minimum_required(VERSION 2.8)
set(VER_MAJOR 0)
set(VER_MINOR 1)
set(VER_ADDITIONAL "alpha")

set(SDIR "./src/")

if (UNIX)
    set(PLATSRC os_unix.c)
endif()

if (!UNIX)
    set(PLATSRC os_other.c)
endif ()

set(SOURCES 
    ${SDIR}${PLATSRC}
    ${SDIR}aklisp.c
    ${SDIR}lib.c
    ${SDIR}gc.c
    ${SDIR}parser.c
    ${SDIR}lexer.c)

set(TARGET aklisp)
include(CheckIncludeFiles)
check_include_files(ucontext.h HAVE_UCONTEXT_H)
check_include_files(execinfo.h HAVE_EXECINFO_H)
configure_file(config.h.in config.h)
add_library(${TARGET}_static STATIC ${SOURCES})
add_library(${TARGET}_shared SHARED ${SOURCES})

add_definitions(-DVER_MAJOR=${VER_MAJOR})
add_definitions(-DVER_MINOR=${VER_MINOR})
add_definitions(-DVER_ADDITIONAL="${VER_ADDITIONAL}")
add_definitions(-DAKL_SYSTEM_INFO)
add_definitions(-DAKL_SYSTEM_NAME="${CMAKE_SYSTEM_NAME}")
add_definitions(-DAKL_SYSTEM_VERSION="${CMAKE_SYSTEM_VERSION}")
add_definitions(-DAKL_PROCESSOR="${CMAKE_SYSTEM_PROCESSOR}")

set_target_properties(${TARGET}_static PROPERTIES VERSION ${VER_MAJOR}.${VER_MINOR}.0 SOVERSION 1)
set_target_properties(${TARGET}_shared PROPERTIES VERSION ${VER_MAJOR}.${VER_MINOR}.0 SOVERSION 1)

add_executable(${TARGET} ${SDIR}/main.c)
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
        DESTINATION lib
        PERMISSIONS OWNER_READ GROUP_READ WORLD_READ)
INSTALL(FILES ${SDIR}/aklisp.h
        DESTINATION include
        PERMISSIONS OWNER_READ GROUP_READ WORLD_READ)
