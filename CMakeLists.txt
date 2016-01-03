CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

PROJECT(PARROT)

# Disable in source build.
if( CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR AND NOT MSVC_IDE )
  message(FATAL_ERROR "In-source builds are not allowed.
CMake would overwrite the makefiles distributed with PARROT.
Please create a directory and run cmake from there, passing the path
to this source directory as the last argument.
This process created the file `CMakeCache.txt' and the directory `CMakeFiles'.
Please delete them.")
endif()

# Set build type to DEBUG.
IF (NOT CMAKE_BUILD_TYPE)
   MESSAGE(STATUS "No build type selected, default to DEBUG'")
   SET(CMAKE_BUILD_TYPE "DEBUG")
ENDIF()

# Compiler flags.
SET(CMAKE_CXX_FLAGS "-std=c++11 -Wall -Wextra -Werror -Wnon-virtual-dtor -fno-strict-aliasing")
SET(CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG")
SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -DRELEASE")

# Set module direcotry.
SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

# Add subdirectory in base.
ADD_SUBDIRECTORY(base/util)
ADD_SUBDIRECTORY(base/sys)

#
