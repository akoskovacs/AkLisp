project(AkLisp)
cmake_minimum_required(VERSION 2.8)
set(VER_MAJOR 0)
set(VER_MINOR 1)
set(VER_ADDITIONAL "alpha")
SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)

set(SDIR "./src/")
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH})
#if (CMAKE_BUILD_TYPE EQUAL DEBUG)
#    message("Debug build")
#    add_definitions(-DAKL_DEBUG)
#endif(CMAKE_BUILD_TYPE EQUAL DEBUG)
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DAKL_DEBUG")

if (UNIX)
    set(PLATSRC os_unix.c)
    SITE_NAME(HOSTNAME)
    add_definitions(-DAKL_USER_INFO)
    add_definitions(-DAKL_USER_NAME="$ENV{USER}")
    add_definitions(-DAKL_HOST_NAME="${HOSTNAME}")
    find_package(Readline)
    find_package(DL)
    if (READLINE_FOUND)
        add_definitions(-DHAVE_READLINE)
    endif()
    add_definitions(-DAKL_MODULE_SEARCH_PATH="${CMAKE_INSTALL_PREFIX}/share/AkLisp/modules/" )
    add_custom_target(modules WORKING_DIRECTORY modules COMMAND ${CMAKE_BUILD_TOOL})
    add_custom_target(clean-modules WORKING_DIRECTORY modules COMMAND ${CMAKE_BUILD_TOOL} clean)
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
    ${SDIR}list.c
    ${SDIR}lib_file.c
    ${SDIR}types.c
    ${SDIR}lexer.c)

option(USE_COLORS "Use standard terminal colors" ON)
option(LINK_SHARED "Link the interpreter with the shared library" OFF)
if (USE_COLORS)
    add_definitions(-DUSE_COLORS)
endif()

set(TARGET aklisp)
include(CheckIncludeFiles)
check_include_files(ucontext.h HAVE_UCONTEXT_H)
check_include_files(execinfo.h HAVE_EXECINFO_H)
check_include_files(dlfcn.h HAVE_DLFCN_H)
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

if (LINK_SHARED)
    set(AKL_SHLIB ${TARGET}_shared)
else()
    set(AKL_SHLIB ${TARGET}_static)
endif()

target_link_libraries(${TARGET} ${AKL_SHLIB} ${READLINE_LIBRARY} ${DL_LIBRARY})
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

INSTALL(FILES ${SDIR}/akl_tree.h
        DESTINATION include
        PERMISSIONS OWNER_READ GROUP_READ WORLD_READ)

INSTALL(FILES etc/libaklisp.pc
        DESTINATION share/pkgconfig
        PERMISSIONS OWNER_READ GROUP_READ WORLD_READ)

INSTALL(FILES etc/libaklisp-static.pc
        DESTINATION share/pkgconfig
        PERMISSIONS OWNER_READ GROUP_READ WORLD_READ)

INSTALL(FILES etc/aklisp-module.pc
        DESTINATION share/pkgconfig
        PERMISSIONS OWNER_READ GROUP_READ WORLD_READ)

file(GLOB MODULES modules/*.alm)
INSTALL(FILES ${MODULES}
        DESTINATION share/AkLisp/modules/
        PERMISSIONS OWNER_READ GROUP_READ WORLD_READ)

# Generate Debian package
SET(CPACK_GENERATOR "DEB")
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "Ákos Kovács <akoskovacs@gmx.com>")
set(CPACK_PACKAGE_VERSION_MAJOR ${VER_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${VER_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH "0")
SET(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6, libgcc1, libreadline5")
SET(CPACK_DEBIAN_PACKAGE_SECTION "interpreters")
SET(CPACK_DEBIAN_PACKAGE_BUILDS_DEPENDS "libreadline5-dev")
SET(CPACK_DEBIAN_PACKAGE_DESCRIPTION_SUMMARY "A tiny Lisp-style language")
SET(CPACK_DEBIAN_PACKAGE_DESCRIPTION "AkLisp is a tiny, hobby Lisp dialect. It is embeddable, user-friendly and relatively fast.")
 
INCLUDE(CPack)
