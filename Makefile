# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 2.8

# Default target executed when no arguments are given to make.
default_target: all
.PHONY : default_target

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canoncical targets will work.
.SUFFIXES:

# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list

# Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# The program to use to edit the cache.
CMAKE_EDIT_COMMAND = /usr/bin/ccmake

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/akos/devel/AkLisp

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/akos/devel/AkLisp

#=============================================================================
# Targets provided globally by CMake.

# Special rule for the target edit_cache
edit_cache:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --cyan "Running CMake cache editor..."
	/usr/bin/ccmake -H$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR)
.PHONY : edit_cache

# Special rule for the target edit_cache
edit_cache/fast: edit_cache
.PHONY : edit_cache/fast

# Special rule for the target install
install: preinstall
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --cyan "Install the project..."
	/usr/bin/cmake -P cmake_install.cmake
.PHONY : install

# Special rule for the target install
install/fast: preinstall/fast
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --cyan "Install the project..."
	/usr/bin/cmake -P cmake_install.cmake
.PHONY : install/fast

# Special rule for the target install/local
install/local: preinstall
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --cyan "Installing only the local directory..."
	/usr/bin/cmake -DCMAKE_INSTALL_LOCAL_ONLY=1 -P cmake_install.cmake
.PHONY : install/local

# Special rule for the target install/local
install/local/fast: install/local
.PHONY : install/local/fast

# Special rule for the target install/strip
install/strip: preinstall
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --cyan "Installing the project stripped..."
	/usr/bin/cmake -DCMAKE_INSTALL_DO_STRIP=1 -P cmake_install.cmake
.PHONY : install/strip

# Special rule for the target install/strip
install/strip/fast: install/strip
.PHONY : install/strip/fast

# Special rule for the target list_install_components
list_install_components:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --cyan "Available install components are: \"Unspecified\""
.PHONY : list_install_components

# Special rule for the target list_install_components
list_install_components/fast: list_install_components
.PHONY : list_install_components/fast

# Special rule for the target rebuild_cache
rebuild_cache:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --cyan "Running CMake to regenerate build system..."
	/usr/bin/cmake -H$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR)
.PHONY : rebuild_cache

# Special rule for the target rebuild_cache
rebuild_cache/fast: rebuild_cache
.PHONY : rebuild_cache/fast

# The main all target
all: cmake_check_build_system
	$(CMAKE_COMMAND) -E cmake_progress_start /home/akos/devel/AkLisp/CMakeFiles /home/akos/devel/AkLisp/CMakeFiles/progress.marks
	$(MAKE) -f CMakeFiles/Makefile2 all
	$(CMAKE_COMMAND) -E cmake_progress_start /home/akos/devel/AkLisp/CMakeFiles 0
.PHONY : all

# The main clean target
clean:
	$(MAKE) -f CMakeFiles/Makefile2 clean
.PHONY : clean

# The main clean target
clean/fast: clean
.PHONY : clean/fast

# Prepare targets for installation.
preinstall: all
	$(MAKE) -f CMakeFiles/Makefile2 preinstall
.PHONY : preinstall

# Prepare targets for installation.
preinstall/fast:
	$(MAKE) -f CMakeFiles/Makefile2 preinstall
.PHONY : preinstall/fast

# clear depends
depend:
	$(CMAKE_COMMAND) -H$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR) --check-build-system CMakeFiles/Makefile.cmake 1
.PHONY : depend

#=============================================================================
# Target rules for targets named aklisp

# Build rule for target.
aklisp: cmake_check_build_system
	$(MAKE) -f CMakeFiles/Makefile2 aklisp
.PHONY : aklisp

# fast build rule for target.
aklisp/fast:
	$(MAKE) -f CMakeFiles/aklisp.dir/build.make CMakeFiles/aklisp.dir/build
.PHONY : aklisp/fast

#=============================================================================
# Target rules for targets named aklisp_shared

# Build rule for target.
aklisp_shared: cmake_check_build_system
	$(MAKE) -f CMakeFiles/Makefile2 aklisp_shared
.PHONY : aklisp_shared

# fast build rule for target.
aklisp_shared/fast:
	$(MAKE) -f CMakeFiles/aklisp_shared.dir/build.make CMakeFiles/aklisp_shared.dir/build
.PHONY : aklisp_shared/fast

#=============================================================================
# Target rules for targets named aklisp_static

# Build rule for target.
aklisp_static: cmake_check_build_system
	$(MAKE) -f CMakeFiles/Makefile2 aklisp_static
.PHONY : aklisp_static

# fast build rule for target.
aklisp_static/fast:
	$(MAKE) -f CMakeFiles/aklisp_static.dir/build.make CMakeFiles/aklisp_static.dir/build
.PHONY : aklisp_static/fast

src/aklisp.o: src/aklisp.c.o
.PHONY : src/aklisp.o

# target to build an object file
src/aklisp.c.o:
	$(MAKE) -f CMakeFiles/aklisp_shared.dir/build.make CMakeFiles/aklisp_shared.dir/src/aklisp.c.o
	$(MAKE) -f CMakeFiles/aklisp_static.dir/build.make CMakeFiles/aklisp_static.dir/src/aklisp.c.o
.PHONY : src/aklisp.c.o

src/aklisp.i: src/aklisp.c.i
.PHONY : src/aklisp.i

# target to preprocess a source file
src/aklisp.c.i:
	$(MAKE) -f CMakeFiles/aklisp_shared.dir/build.make CMakeFiles/aklisp_shared.dir/src/aklisp.c.i
	$(MAKE) -f CMakeFiles/aklisp_static.dir/build.make CMakeFiles/aklisp_static.dir/src/aklisp.c.i
.PHONY : src/aklisp.c.i

src/aklisp.s: src/aklisp.c.s
.PHONY : src/aklisp.s

# target to generate assembly for a file
src/aklisp.c.s:
	$(MAKE) -f CMakeFiles/aklisp_shared.dir/build.make CMakeFiles/aklisp_shared.dir/src/aklisp.c.s
	$(MAKE) -f CMakeFiles/aklisp_static.dir/build.make CMakeFiles/aklisp_static.dir/src/aklisp.c.s
.PHONY : src/aklisp.c.s

src/gc.o: src/gc.c.o
.PHONY : src/gc.o

# target to build an object file
src/gc.c.o:
	$(MAKE) -f CMakeFiles/aklisp_shared.dir/build.make CMakeFiles/aklisp_shared.dir/src/gc.c.o
	$(MAKE) -f CMakeFiles/aklisp_static.dir/build.make CMakeFiles/aklisp_static.dir/src/gc.c.o
.PHONY : src/gc.c.o

src/gc.i: src/gc.c.i
.PHONY : src/gc.i

# target to preprocess a source file
src/gc.c.i:
	$(MAKE) -f CMakeFiles/aklisp_shared.dir/build.make CMakeFiles/aklisp_shared.dir/src/gc.c.i
	$(MAKE) -f CMakeFiles/aklisp_static.dir/build.make CMakeFiles/aklisp_static.dir/src/gc.c.i
.PHONY : src/gc.c.i

src/gc.s: src/gc.c.s
.PHONY : src/gc.s

# target to generate assembly for a file
src/gc.c.s:
	$(MAKE) -f CMakeFiles/aklisp_shared.dir/build.make CMakeFiles/aklisp_shared.dir/src/gc.c.s
	$(MAKE) -f CMakeFiles/aklisp_static.dir/build.make CMakeFiles/aklisp_static.dir/src/gc.c.s
.PHONY : src/gc.c.s

src/lexer.o: src/lexer.c.o
.PHONY : src/lexer.o

# target to build an object file
src/lexer.c.o:
	$(MAKE) -f CMakeFiles/aklisp_shared.dir/build.make CMakeFiles/aklisp_shared.dir/src/lexer.c.o
	$(MAKE) -f CMakeFiles/aklisp_static.dir/build.make CMakeFiles/aklisp_static.dir/src/lexer.c.o
.PHONY : src/lexer.c.o

src/lexer.i: src/lexer.c.i
.PHONY : src/lexer.i

# target to preprocess a source file
src/lexer.c.i:
	$(MAKE) -f CMakeFiles/aklisp_shared.dir/build.make CMakeFiles/aklisp_shared.dir/src/lexer.c.i
	$(MAKE) -f CMakeFiles/aklisp_static.dir/build.make CMakeFiles/aklisp_static.dir/src/lexer.c.i
.PHONY : src/lexer.c.i

src/lexer.s: src/lexer.c.s
.PHONY : src/lexer.s

# target to generate assembly for a file
src/lexer.c.s:
	$(MAKE) -f CMakeFiles/aklisp_shared.dir/build.make CMakeFiles/aklisp_shared.dir/src/lexer.c.s
	$(MAKE) -f CMakeFiles/aklisp_static.dir/build.make CMakeFiles/aklisp_static.dir/src/lexer.c.s
.PHONY : src/lexer.c.s

src/lib.o: src/lib.c.o
.PHONY : src/lib.o

# target to build an object file
src/lib.c.o:
	$(MAKE) -f CMakeFiles/aklisp_shared.dir/build.make CMakeFiles/aklisp_shared.dir/src/lib.c.o
	$(MAKE) -f CMakeFiles/aklisp_static.dir/build.make CMakeFiles/aklisp_static.dir/src/lib.c.o
.PHONY : src/lib.c.o

src/lib.i: src/lib.c.i
.PHONY : src/lib.i

# target to preprocess a source file
src/lib.c.i:
	$(MAKE) -f CMakeFiles/aklisp_shared.dir/build.make CMakeFiles/aklisp_shared.dir/src/lib.c.i
	$(MAKE) -f CMakeFiles/aklisp_static.dir/build.make CMakeFiles/aklisp_static.dir/src/lib.c.i
.PHONY : src/lib.c.i

src/lib.s: src/lib.c.s
.PHONY : src/lib.s

# target to generate assembly for a file
src/lib.c.s:
	$(MAKE) -f CMakeFiles/aklisp_shared.dir/build.make CMakeFiles/aklisp_shared.dir/src/lib.c.s
	$(MAKE) -f CMakeFiles/aklisp_static.dir/build.make CMakeFiles/aklisp_static.dir/src/lib.c.s
.PHONY : src/lib.c.s

src/list.o: src/list.c.o
.PHONY : src/list.o

# target to build an object file
src/list.c.o:
	$(MAKE) -f CMakeFiles/aklisp_shared.dir/build.make CMakeFiles/aklisp_shared.dir/src/list.c.o
	$(MAKE) -f CMakeFiles/aklisp_static.dir/build.make CMakeFiles/aklisp_static.dir/src/list.c.o
.PHONY : src/list.c.o

src/list.i: src/list.c.i
.PHONY : src/list.i

# target to preprocess a source file
src/list.c.i:
	$(MAKE) -f CMakeFiles/aklisp_shared.dir/build.make CMakeFiles/aklisp_shared.dir/src/list.c.i
	$(MAKE) -f CMakeFiles/aklisp_static.dir/build.make CMakeFiles/aklisp_static.dir/src/list.c.i
.PHONY : src/list.c.i

src/list.s: src/list.c.s
.PHONY : src/list.s

# target to generate assembly for a file
src/list.c.s:
	$(MAKE) -f CMakeFiles/aklisp_shared.dir/build.make CMakeFiles/aklisp_shared.dir/src/list.c.s
	$(MAKE) -f CMakeFiles/aklisp_static.dir/build.make CMakeFiles/aklisp_static.dir/src/list.c.s
.PHONY : src/list.c.s

src/main.o: src/main.c.o
.PHONY : src/main.o

# target to build an object file
src/main.c.o:
	$(MAKE) -f CMakeFiles/aklisp.dir/build.make CMakeFiles/aklisp.dir/src/main.c.o
.PHONY : src/main.c.o

src/main.i: src/main.c.i
.PHONY : src/main.i

# target to preprocess a source file
src/main.c.i:
	$(MAKE) -f CMakeFiles/aklisp.dir/build.make CMakeFiles/aklisp.dir/src/main.c.i
.PHONY : src/main.c.i

src/main.s: src/main.c.s
.PHONY : src/main.s

# target to generate assembly for a file
src/main.c.s:
	$(MAKE) -f CMakeFiles/aklisp.dir/build.make CMakeFiles/aklisp.dir/src/main.c.s
.PHONY : src/main.c.s

src/os_unix.o: src/os_unix.c.o
.PHONY : src/os_unix.o

# target to build an object file
src/os_unix.c.o:
	$(MAKE) -f CMakeFiles/aklisp_shared.dir/build.make CMakeFiles/aklisp_shared.dir/src/os_unix.c.o
	$(MAKE) -f CMakeFiles/aklisp_static.dir/build.make CMakeFiles/aklisp_static.dir/src/os_unix.c.o
.PHONY : src/os_unix.c.o

src/os_unix.i: src/os_unix.c.i
.PHONY : src/os_unix.i

# target to preprocess a source file
src/os_unix.c.i:
	$(MAKE) -f CMakeFiles/aklisp_shared.dir/build.make CMakeFiles/aklisp_shared.dir/src/os_unix.c.i
	$(MAKE) -f CMakeFiles/aklisp_static.dir/build.make CMakeFiles/aklisp_static.dir/src/os_unix.c.i
.PHONY : src/os_unix.c.i

src/os_unix.s: src/os_unix.c.s
.PHONY : src/os_unix.s

# target to generate assembly for a file
src/os_unix.c.s:
	$(MAKE) -f CMakeFiles/aklisp_shared.dir/build.make CMakeFiles/aklisp_shared.dir/src/os_unix.c.s
	$(MAKE) -f CMakeFiles/aklisp_static.dir/build.make CMakeFiles/aklisp_static.dir/src/os_unix.c.s
.PHONY : src/os_unix.c.s

src/parser.o: src/parser.c.o
.PHONY : src/parser.o

# target to build an object file
src/parser.c.o:
	$(MAKE) -f CMakeFiles/aklisp_shared.dir/build.make CMakeFiles/aklisp_shared.dir/src/parser.c.o
	$(MAKE) -f CMakeFiles/aklisp_static.dir/build.make CMakeFiles/aklisp_static.dir/src/parser.c.o
.PHONY : src/parser.c.o

src/parser.i: src/parser.c.i
.PHONY : src/parser.i

# target to preprocess a source file
src/parser.c.i:
	$(MAKE) -f CMakeFiles/aklisp_shared.dir/build.make CMakeFiles/aklisp_shared.dir/src/parser.c.i
	$(MAKE) -f CMakeFiles/aklisp_static.dir/build.make CMakeFiles/aklisp_static.dir/src/parser.c.i
.PHONY : src/parser.c.i

src/parser.s: src/parser.c.s
.PHONY : src/parser.s

# target to generate assembly for a file
src/parser.c.s:
	$(MAKE) -f CMakeFiles/aklisp_shared.dir/build.make CMakeFiles/aklisp_shared.dir/src/parser.c.s
	$(MAKE) -f CMakeFiles/aklisp_static.dir/build.make CMakeFiles/aklisp_static.dir/src/parser.c.s
.PHONY : src/parser.c.s

# Help Target
help:
	@echo "The following are some of the valid targets for this Makefile:"
	@echo "... all (the default if no target is provided)"
	@echo "... clean"
	@echo "... depend"
	@echo "... aklisp"
	@echo "... aklisp_shared"
	@echo "... aklisp_static"
	@echo "... edit_cache"
	@echo "... install"
	@echo "... install/local"
	@echo "... install/strip"
	@echo "... list_install_components"
	@echo "... rebuild_cache"
	@echo "... src/aklisp.o"
	@echo "... src/aklisp.i"
	@echo "... src/aklisp.s"
	@echo "... src/gc.o"
	@echo "... src/gc.i"
	@echo "... src/gc.s"
	@echo "... src/lexer.o"
	@echo "... src/lexer.i"
	@echo "... src/lexer.s"
	@echo "... src/lib.o"
	@echo "... src/lib.i"
	@echo "... src/lib.s"
	@echo "... src/list.o"
	@echo "... src/list.i"
	@echo "... src/list.s"
	@echo "... src/main.o"
	@echo "... src/main.i"
	@echo "... src/main.s"
	@echo "... src/os_unix.o"
	@echo "... src/os_unix.i"
	@echo "... src/os_unix.s"
	@echo "... src/parser.o"
	@echo "... src/parser.i"
	@echo "... src/parser.s"
.PHONY : help



#=============================================================================
# Special targets to cleanup operation of make.

# Special rule to run CMake to check the build system integrity.
# No rule that depends on this can have commands that come from listfiles
# because they might be regenerated.
cmake_check_build_system:
	$(CMAKE_COMMAND) -H$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR) --check-build-system CMakeFiles/Makefile.cmake 0
.PHONY : cmake_check_build_system

