# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.22

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
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
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /mnt/d/work/OS_PROJ

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /mnt/d/work/OS_PROJ/build

# Include any dependencies generated for this target.
include CMakeFiles/Ludo.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/Ludo.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/Ludo.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/Ludo.dir/flags.make

CMakeFiles/Ludo.dir/Ludo_autogen/mocs_compilation.cpp.o: CMakeFiles/Ludo.dir/flags.make
CMakeFiles/Ludo.dir/Ludo_autogen/mocs_compilation.cpp.o: Ludo_autogen/mocs_compilation.cpp
CMakeFiles/Ludo.dir/Ludo_autogen/mocs_compilation.cpp.o: CMakeFiles/Ludo.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/mnt/d/work/OS_PROJ/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/Ludo.dir/Ludo_autogen/mocs_compilation.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/Ludo.dir/Ludo_autogen/mocs_compilation.cpp.o -MF CMakeFiles/Ludo.dir/Ludo_autogen/mocs_compilation.cpp.o.d -o CMakeFiles/Ludo.dir/Ludo_autogen/mocs_compilation.cpp.o -c /mnt/d/work/OS_PROJ/build/Ludo_autogen/mocs_compilation.cpp

CMakeFiles/Ludo.dir/Ludo_autogen/mocs_compilation.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/Ludo.dir/Ludo_autogen/mocs_compilation.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /mnt/d/work/OS_PROJ/build/Ludo_autogen/mocs_compilation.cpp > CMakeFiles/Ludo.dir/Ludo_autogen/mocs_compilation.cpp.i

CMakeFiles/Ludo.dir/Ludo_autogen/mocs_compilation.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/Ludo.dir/Ludo_autogen/mocs_compilation.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /mnt/d/work/OS_PROJ/build/Ludo_autogen/mocs_compilation.cpp -o CMakeFiles/Ludo.dir/Ludo_autogen/mocs_compilation.cpp.s

CMakeFiles/Ludo.dir/main.cpp.o: CMakeFiles/Ludo.dir/flags.make
CMakeFiles/Ludo.dir/main.cpp.o: ../main.cpp
CMakeFiles/Ludo.dir/main.cpp.o: CMakeFiles/Ludo.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/mnt/d/work/OS_PROJ/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/Ludo.dir/main.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/Ludo.dir/main.cpp.o -MF CMakeFiles/Ludo.dir/main.cpp.o.d -o CMakeFiles/Ludo.dir/main.cpp.o -c /mnt/d/work/OS_PROJ/main.cpp

CMakeFiles/Ludo.dir/main.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/Ludo.dir/main.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /mnt/d/work/OS_PROJ/main.cpp > CMakeFiles/Ludo.dir/main.cpp.i

CMakeFiles/Ludo.dir/main.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/Ludo.dir/main.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /mnt/d/work/OS_PROJ/main.cpp -o CMakeFiles/Ludo.dir/main.cpp.s

# Object files for target Ludo
Ludo_OBJECTS = \
"CMakeFiles/Ludo.dir/Ludo_autogen/mocs_compilation.cpp.o" \
"CMakeFiles/Ludo.dir/main.cpp.o"

# External object files for target Ludo
Ludo_EXTERNAL_OBJECTS =

Ludo: CMakeFiles/Ludo.dir/Ludo_autogen/mocs_compilation.cpp.o
Ludo: CMakeFiles/Ludo.dir/main.cpp.o
Ludo: CMakeFiles/Ludo.dir/build.make
Ludo: /usr/lib/x86_64-linux-gnu/libQt5Widgets.so.5.15.3
Ludo: /usr/lib/x86_64-linux-gnu/libQt5Gui.so.5.15.3
Ludo: /usr/lib/x86_64-linux-gnu/libQt5Network.so.5.15.3
Ludo: /usr/lib/x86_64-linux-gnu/libQt5Core.so.5.15.3
Ludo: CMakeFiles/Ludo.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/mnt/d/work/OS_PROJ/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking CXX executable Ludo"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/Ludo.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/Ludo.dir/build: Ludo
.PHONY : CMakeFiles/Ludo.dir/build

CMakeFiles/Ludo.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/Ludo.dir/cmake_clean.cmake
.PHONY : CMakeFiles/Ludo.dir/clean

CMakeFiles/Ludo.dir/depend:
	cd /mnt/d/work/OS_PROJ/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /mnt/d/work/OS_PROJ /mnt/d/work/OS_PROJ /mnt/d/work/OS_PROJ/build /mnt/d/work/OS_PROJ/build /mnt/d/work/OS_PROJ/build/CMakeFiles/Ludo.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/Ludo.dir/depend

