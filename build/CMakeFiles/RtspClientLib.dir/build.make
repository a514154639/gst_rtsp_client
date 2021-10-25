# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.13

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
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/toybrick/Develop/CProject/gst_rtsp

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/toybrick/Develop/CProject/gst_rtsp/build

# Include any dependencies generated for this target.
include CMakeFiles/RtspClientLib.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/RtspClientLib.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/RtspClientLib.dir/flags.make

CMakeFiles/RtspClientLib.dir/src/gst_rtsp_client.cpp.o: CMakeFiles/RtspClientLib.dir/flags.make
CMakeFiles/RtspClientLib.dir/src/gst_rtsp_client.cpp.o: ../src/gst_rtsp_client.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/toybrick/Develop/CProject/gst_rtsp/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/RtspClientLib.dir/src/gst_rtsp_client.cpp.o"
	g++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/RtspClientLib.dir/src/gst_rtsp_client.cpp.o -c /home/toybrick/Develop/CProject/gst_rtsp/src/gst_rtsp_client.cpp

CMakeFiles/RtspClientLib.dir/src/gst_rtsp_client.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/RtspClientLib.dir/src/gst_rtsp_client.cpp.i"
	g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/toybrick/Develop/CProject/gst_rtsp/src/gst_rtsp_client.cpp > CMakeFiles/RtspClientLib.dir/src/gst_rtsp_client.cpp.i

CMakeFiles/RtspClientLib.dir/src/gst_rtsp_client.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/RtspClientLib.dir/src/gst_rtsp_client.cpp.s"
	g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/toybrick/Develop/CProject/gst_rtsp/src/gst_rtsp_client.cpp -o CMakeFiles/RtspClientLib.dir/src/gst_rtsp_client.cpp.s

CMakeFiles/RtspClientLib.dir/src/setup.cpp.o: CMakeFiles/RtspClientLib.dir/flags.make
CMakeFiles/RtspClientLib.dir/src/setup.cpp.o: ../src/setup.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/toybrick/Develop/CProject/gst_rtsp/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/RtspClientLib.dir/src/setup.cpp.o"
	g++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/RtspClientLib.dir/src/setup.cpp.o -c /home/toybrick/Develop/CProject/gst_rtsp/src/setup.cpp

CMakeFiles/RtspClientLib.dir/src/setup.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/RtspClientLib.dir/src/setup.cpp.i"
	g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/toybrick/Develop/CProject/gst_rtsp/src/setup.cpp > CMakeFiles/RtspClientLib.dir/src/setup.cpp.i

CMakeFiles/RtspClientLib.dir/src/setup.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/RtspClientLib.dir/src/setup.cpp.s"
	g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/toybrick/Develop/CProject/gst_rtsp/src/setup.cpp -o CMakeFiles/RtspClientLib.dir/src/setup.cpp.s

# Object files for target RtspClientLib
RtspClientLib_OBJECTS = \
"CMakeFiles/RtspClientLib.dir/src/gst_rtsp_client.cpp.o" \
"CMakeFiles/RtspClientLib.dir/src/setup.cpp.o"

# External object files for target RtspClientLib
RtspClientLib_EXTERNAL_OBJECTS =

libRtspClientLib.so: CMakeFiles/RtspClientLib.dir/src/gst_rtsp_client.cpp.o
libRtspClientLib.so: CMakeFiles/RtspClientLib.dir/src/setup.cpp.o
libRtspClientLib.so: CMakeFiles/RtspClientLib.dir/build.make
libRtspClientLib.so: /usr/lib/aarch64-linux-gnu/libgstreamer-1.0.so.0
libRtspClientLib.so: /usr/lib/aarch64-linux-gnu/libgobject-2.0.so.0
libRtspClientLib.so: /usr/lib/aarch64-linux-gnu/libglib-2.0.so.0
libRtspClientLib.so: /usr/lib/aarch64-linux-gnu/libgstbase-1.0.so.0
libRtspClientLib.so: /usr/lib/aarch64-linux-gnu/libgstvideo-1.0.so.0
libRtspClientLib.so: /usr/lib/aarch64-linux-gnu/libgstapp-1.0.so.0
libRtspClientLib.so: /lib/aarch64-linux-gnu/libc.so.6
libRtspClientLib.so: /usr/lib/ld-linux-aarch64.so.1
libRtspClientLib.so: /lib/aarch64-linux-gnu/libdl.so.2
libRtspClientLib.so: /lib/aarch64-linux-gnu/libpthread.so.0
libRtspClientLib.so: /usr/lib/aarch64-linux-gnu/libgmodule-2.0.so.0
libRtspClientLib.so: /lib/aarch64-linux-gnu/libm.so.6
libRtspClientLib.so: /usr/lib/aarch64-linux-gnu/libffi.so.6
libRtspClientLib.so: /lib/aarch64-linux-gnu/libpcre.so.3
libRtspClientLib.so: /usr/lib/aarch64-linux-gnu/librockchip_rga.so.0
libRtspClientLib.so: /usr/local/lib/libopencv_imgproc.so.3.4
libRtspClientLib.so: /home/toybrick/EdgeComputing/sq_main_/rtsp/librga.so
libRtspClientLib.so: CMakeFiles/RtspClientLib.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/toybrick/Develop/CProject/gst_rtsp/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking CXX shared library libRtspClientLib.so"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/RtspClientLib.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/RtspClientLib.dir/build: libRtspClientLib.so

.PHONY : CMakeFiles/RtspClientLib.dir/build

CMakeFiles/RtspClientLib.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/RtspClientLib.dir/cmake_clean.cmake
.PHONY : CMakeFiles/RtspClientLib.dir/clean

CMakeFiles/RtspClientLib.dir/depend:
	cd /home/toybrick/Develop/CProject/gst_rtsp/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/toybrick/Develop/CProject/gst_rtsp /home/toybrick/Develop/CProject/gst_rtsp /home/toybrick/Develop/CProject/gst_rtsp/build /home/toybrick/Develop/CProject/gst_rtsp/build /home/toybrick/Develop/CProject/gst_rtsp/build/CMakeFiles/RtspClientLib.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/RtspClientLib.dir/depend

