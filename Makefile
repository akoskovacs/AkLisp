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

aklisp.o: aklisp.c.o
.PHONY : aklisp.o

# target to build an object file
aklisp.c.o:
	$(MAKE) -f CMakeFiles/aklisp.dir/build.make CMakeFiles/aklisp.dir/aklisp.c.o
.PHONY : aklisp.c.o

aklisp.i: aklisp.c.i
.PHONY : aklisp.i

# target to preprocess a source file
aklisp.c.i:
	$(MAKE) -f CMakeFiles/aklisp.dir/build.make CMakeFiles/aklisp.dir/aklisp.c.i
.PHONY : aklisp.c.i

aklisp.s: aklisp.c.s
.PHONY : aklisp.s

# target to generate assembly for a file
aklisp.c.s:
	$(MAKE) -f CMakeFiles/aklisp.dir/build.make CMakeFiles/aklisp.dir/aklisp.c.s
.PHONY : aklisp.c.s

gc.o: gc.c.o
.PHONY : gc.o

# target to build an object file
gc.c.o:
	$(MAKE) -f CMakeFiles/aklisp.dir/build.make CMakeFiles/aklisp.dir/gc.c.o
.PHONY : gc.c.o

gc.i: gc.c.i
.PHONY : gc.i

# target to preprocess a source file
gc.c.i:
	$(MAKE) -f CMakeFiles/aklisp.dir/build.make CMakeFiles/aklisp.dir/gc.c.i
.PHONY : gc.c.i

gc.s: gc.c.s
.PHONY : gc.s

# target to generate assembly for a file
gc.c.s:
	$(MAKE) -f CMakeFiles/aklisp.dir/build.make CMakeFiles/aklisp.dir/gc.c.s
.PHONY : gc.c.s

lexer.o: lexer.c.o
.PHONY : lexer.o

# target to build an object file
lexer.c.o:
	$(MAKE) -f CMakeFiles/aklisp.dir/build.make CMakeFiles/aklisp.dir/lexer.c.o
.PHONY : lexer.c.o

lexer.i: lexer.c.i
.PHONY : lexer.i

# target to preprocess a source file
lexer.c.i:
	$(MAKE) -f CMakeFiles/aklisp.dir/build.make CMakeFiles/aklisp.dir/lexer.c.i
.PHONY : lexer.c.i

lexer.s: lexer.c.s
.PHONY : lexer.s

# target to generate assembly for a file
lexer.c.s:
	$(MAKE) -f CMakeFiles/aklisp.dir/build.make CMakeFiles/aklisp.dir/lexer.c.s
.PHONY : lexer.c.s

lib.o: lib.c.o
.PHONY : lib.o

# target to build an object file
lib.c.o:
	$(MAKE) -f CMakeFiles/aklisp.dir/build.make CMakeFiles/aklisp.dir/lib.c.o
.PHONY : lib.c.o

lib.i: lib.c.i
.PHONY : lib.i

# target to preprocess a source file
lib.c.i:
	$(MAKE) -f CMakeFiles/aklisp.dir/build.make CMakeFiles/aklisp.dir/lib.c.i
.PHONY : lib.c.i

lib.s: lib.c.s
.PHONY : lib.s

# target to generate assembly for a file
lib.c.s:
	$(MAKE) -f CMakeFiles/aklisp.dir/build.make CMakeFiles/aklisp.dir/lib.c.s
.PHONY : lib.c.s

parser.o: parser.c.o
.PHONY : parser.o

# target to build an object file
parser.c.o:
	$(MAKE) -f CMakeFiles/aklisp.dir/build.make CMakeFiles/aklisp.dir/parser.c.o
.PHONY : parser.c.o

parser.i: parser.c.i
.PHONY : parser.i

# target to preprocess a source file
parser.c.i:
	$(MAKE) -f CMakeFiles/aklisp.dir/build.make CMakeFiles/aklisp.dir/parser.c.i
.PHONY : parser.c.i

parser.s: parser.c.s
.PHONY : parser.s

# target to generate assembly for a file
parser.c.s:
	$(MAKE) -f CMakeFiles/aklisp.dir/build.make CMakeFiles/aklisp.dir/parser.c.s
.PHONY : parser.c.s

# Help Target
help:
	@echo "The following are some of the valid targets for this Makefile:"
	@echo "... all (the default if no target is provided)"
	@echo "... clean"
	@echo "... depend"
	@echo "... aklisp"
	@echo "... edit_cache"
	@echo "... install"
	@echo "... install/local"
	@echo "... install/strip"
	@echo "... list_install_components"
	@echo "... rebuild_cache"
	@echo "... aklisp.o"
	@echo "... aklisp.i"
	@echo "... aklisp.s"
	@echo "... gc.o"
	@echo "... gc.i"
	@echo "... gc.s"
	@echo "... lexer.o"
	@echo "... lexer.i"
	@echo "... lexer.s"
	@echo "... lib.o"
	@echo "... lib.i"
	@echo "... lib.s"
	@echo "... parser.o"
	@echo "... parser.i"
	@echo "... parser.s"
.PHONY : help



#=============================================================================
# Special targets to cleanup operation of make.

# Special rule to run CMake to check the build system integrity.
# No rule that depends on this can have commands that come from listfiles
# because they might be regenerated.
cmake_check_build_system:
	$(CMAKE_COMMAND) -H$(CMAKE_SOURCE_DIR) -B$(CMAKE_BINARY_DIR) --check-build-system CMakeFiles/Makefile.cmake 0
.PHONY : cmake_check_build_system

