# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 2.8

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

# Include any dependencies generated for this target.
include CMakeFiles/aklisp.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/aklisp.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/aklisp.dir/flags.make

CMakeFiles/aklisp.dir/src/main.c.o: CMakeFiles/aklisp.dir/flags.make
CMakeFiles/aklisp.dir/src/main.c.o: src/main.c
	$(CMAKE_COMMAND) -E cmake_progress_report /home/akos/devel/AkLisp/CMakeFiles $(CMAKE_PROGRESS_1)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building C object CMakeFiles/aklisp.dir/src/main.c.o"
	/usr/bin/gcc  $(C_DEFINES) $(C_FLAGS) -o CMakeFiles/aklisp.dir/src/main.c.o   -c /home/akos/devel/AkLisp/src/main.c

CMakeFiles/aklisp.dir/src/main.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/aklisp.dir/src/main.c.i"
	/usr/bin/gcc  $(C_DEFINES) $(C_FLAGS) -E /home/akos/devel/AkLisp/src/main.c > CMakeFiles/aklisp.dir/src/main.c.i

CMakeFiles/aklisp.dir/src/main.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/aklisp.dir/src/main.c.s"
	/usr/bin/gcc  $(C_DEFINES) $(C_FLAGS) -S /home/akos/devel/AkLisp/src/main.c -o CMakeFiles/aklisp.dir/src/main.c.s

CMakeFiles/aklisp.dir/src/main.c.o.requires:
.PHONY : CMakeFiles/aklisp.dir/src/main.c.o.requires

CMakeFiles/aklisp.dir/src/main.c.o.provides: CMakeFiles/aklisp.dir/src/main.c.o.requires
	$(MAKE) -f CMakeFiles/aklisp.dir/build.make CMakeFiles/aklisp.dir/src/main.c.o.provides.build
.PHONY : CMakeFiles/aklisp.dir/src/main.c.o.provides

CMakeFiles/aklisp.dir/src/main.c.o.provides.build: CMakeFiles/aklisp.dir/src/main.c.o
.PHONY : CMakeFiles/aklisp.dir/src/main.c.o.provides.build

# Object files for target aklisp
aklisp_OBJECTS = \
"CMakeFiles/aklisp.dir/src/main.c.o"

# External object files for target aklisp
aklisp_EXTERNAL_OBJECTS =

aklisp: CMakeFiles/aklisp.dir/src/main.c.o
aklisp: libaklisp_static.a
aklisp: CMakeFiles/aklisp.dir/build.make
aklisp: CMakeFiles/aklisp.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --red --bold "Linking C executable aklisp"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/aklisp.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/aklisp.dir/build: aklisp
.PHONY : CMakeFiles/aklisp.dir/build

CMakeFiles/aklisp.dir/requires: CMakeFiles/aklisp.dir/src/main.c.o.requires
.PHONY : CMakeFiles/aklisp.dir/requires

CMakeFiles/aklisp.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/aklisp.dir/cmake_clean.cmake
.PHONY : CMakeFiles/aklisp.dir/clean

CMakeFiles/aklisp.dir/depend:
	cd /home/akos/devel/AkLisp && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/akos/devel/AkLisp /home/akos/devel/AkLisp /home/akos/devel/AkLisp /home/akos/devel/AkLisp /home/akos/devel/AkLisp/CMakeFiles/aklisp.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/aklisp.dir/depend

