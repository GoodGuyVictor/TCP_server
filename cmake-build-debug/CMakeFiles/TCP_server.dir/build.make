# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.9

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
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
CMAKE_COMMAND = /opt/clion-2017.3.3/bin/cmake/bin/cmake

# The command to remove a file.
RM = /opt/clion-2017.3.3/bin/cmake/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/victor/githubRepos/TCP_server

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/victor/githubRepos/TCP_server/cmake-build-debug

# Include any dependencies generated for this target.
include CMakeFiles/TCP_server.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/TCP_server.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/TCP_server.dir/flags.make

CMakeFiles/TCP_server.dir/server.cpp.o: CMakeFiles/TCP_server.dir/flags.make
CMakeFiles/TCP_server.dir/server.cpp.o: ../server.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/victor/githubRepos/TCP_server/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/TCP_server.dir/server.cpp.o"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/TCP_server.dir/server.cpp.o -c /home/victor/githubRepos/TCP_server/server.cpp

CMakeFiles/TCP_server.dir/server.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/TCP_server.dir/server.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/victor/githubRepos/TCP_server/server.cpp > CMakeFiles/TCP_server.dir/server.cpp.i

CMakeFiles/TCP_server.dir/server.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/TCP_server.dir/server.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/victor/githubRepos/TCP_server/server.cpp -o CMakeFiles/TCP_server.dir/server.cpp.s

CMakeFiles/TCP_server.dir/server.cpp.o.requires:

.PHONY : CMakeFiles/TCP_server.dir/server.cpp.o.requires

CMakeFiles/TCP_server.dir/server.cpp.o.provides: CMakeFiles/TCP_server.dir/server.cpp.o.requires
	$(MAKE) -f CMakeFiles/TCP_server.dir/build.make CMakeFiles/TCP_server.dir/server.cpp.o.provides.build
.PHONY : CMakeFiles/TCP_server.dir/server.cpp.o.provides

CMakeFiles/TCP_server.dir/server.cpp.o.provides.build: CMakeFiles/TCP_server.dir/server.cpp.o


# Object files for target TCP_server
TCP_server_OBJECTS = \
"CMakeFiles/TCP_server.dir/server.cpp.o"

# External object files for target TCP_server
TCP_server_EXTERNAL_OBJECTS =

TCP_server: CMakeFiles/TCP_server.dir/server.cpp.o
TCP_server: CMakeFiles/TCP_server.dir/build.make
TCP_server: CMakeFiles/TCP_server.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/victor/githubRepos/TCP_server/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable TCP_server"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/TCP_server.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/TCP_server.dir/build: TCP_server

.PHONY : CMakeFiles/TCP_server.dir/build

CMakeFiles/TCP_server.dir/requires: CMakeFiles/TCP_server.dir/server.cpp.o.requires

.PHONY : CMakeFiles/TCP_server.dir/requires

CMakeFiles/TCP_server.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/TCP_server.dir/cmake_clean.cmake
.PHONY : CMakeFiles/TCP_server.dir/clean

CMakeFiles/TCP_server.dir/depend:
	cd /home/victor/githubRepos/TCP_server/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/victor/githubRepos/TCP_server /home/victor/githubRepos/TCP_server /home/victor/githubRepos/TCP_server/cmake-build-debug /home/victor/githubRepos/TCP_server/cmake-build-debug /home/victor/githubRepos/TCP_server/cmake-build-debug/CMakeFiles/TCP_server.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/TCP_server.dir/depend

