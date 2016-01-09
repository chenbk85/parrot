CMAKE_MINIMUM_REQUIRED(VERSION 2.8.11)

# Use customized compiler. The apple clang doesn't support thread_local storage.
IF(CMAKE_HOST_APPLE)
  SET(CMAKE_CXX_COMPILER /Users/leopold/Workspace/tools/llvm/bin/clang++)
  SET(CMAKE_C_COMPILER /Users/leopold/Workspace/tools/llvm/bin/clang)
ENDIF(CMAKE_HOST_APPLE)

PROJECT(PARROT)

# Disable in source build.
IF(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR AND NOT MSVC_IDE )
  message(FATAL_ERROR "In-source builds are not allowed.
CMake would overwrite the makefiles distributed with PARROT.
Please create a directory and run cmake from there, passing the path
to this source directory as the last argument.
This process created the file `CMakeCache.txt' and the directory `CMakeFiles'.
Please delete them.")
ENDIF(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR AND NOT MSVC_IDE )

# Set build type to DEBUG.
IF (NOT CMAKE_BUILD_TYPE)
  MESSAGE(STATUS "No build type selected, default to DEBUG'")
  SET(CMAKE_BUILD_TYPE "DEBUG")
ENDIF()

# Compiler and its flags.
IF(CMAKE_HOST_APPLE)
  SET(CMAKE_CXX_FLAGS "-std=c++11 -stdlib=libc++ -Wall -Wextra -Wnon-virtual-dtor -fno-strict-aliasing -pedantic")
  SET(CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG -Werror -Wshadow")
  SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -DRELEASE")
ELSE(CMAKE_HOST_APPLE)
  SET(CMAKE_CXX_FLAGS "-std=c++11 -Wall -Wextra -Wnon-virtual-dtor -fno-strict-aliasing -pedantic")
  SET(CMAKE_CXX_FLAGS_DEBUG "-g -DDEBUG -Werror -Wshadow")
  SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -DRELEASE")
ENDIF(CMAKE_HOST_APPLE)

# Add definitions.
ADD_DEFINITIONS(-DRAPIDJSON_HAS_STDSTRING -DRAPIDJSON_SSE2)

# Set module direcotry.
SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

# Packages.
FIND_PACKAGE(OpenSSL)
IF(NOT OPENSSL_FOUND)
  MESSAGE(STATUS "Failed to find system OpenSSL, trying to find local OpenSSL.")
  FIND_PACKAGE(LocalOpenSSL)
  IF(NOT OPENSSL_FOUND)
    MESSAGE(FATAL_ERROR "Failed to find OpenSSL.")
  ENDIF(NOT OPENSSL_FOUND)
ENDIF(NOT OPENSSL_FOUND)

FIND_PACKAGE(RapidJson)
IF(NOT RAPIDJSON_FOUND)
  MESSAGE(FATAL_ERROR "Failed to find rapidjson.")
ENDIF(NOT RAPIDJSON_FOUND)

# Add subdirectory in base.
ADD_SUBDIRECTORY(base/util)
ADD_SUBDIRECTORY(base/sys)

# Add subdirectory in component.
ADD_SUBDIRECTORY(component/json)
ADD_SUBDIRECTORY(component/websocket)
ADD_SUBDIRECTORY(component/servergear)

# Add project dir.
ADD_SUBDIRECTORY(project/chat)
