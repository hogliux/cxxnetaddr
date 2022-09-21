##
##  CMakeLists.txt
##  cxxnetaddr - https://github.com/hogliux/cxxnetaddr
##
##  Created by Fabian Renn-Giles, fabian@fieldingdsp.com on 18th September 2022.
##  Copyright © 2022 Fielding DSP GmbH, All rights reserved.
##
##  Fielding DSP GmbH
##  Jägerstr. 36
##  14467 Potsdam, Germany
##
cmake_minimum_required(VERSION 3.22)

project(cxxnetaddr)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_FLAGS_RELEASE -Ofast)

set(THREADS_PREFER_PTHREAD_FLAG ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

find_package(Threads REQUIRED)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Set a default build type if none was specified
set(default_build_type "Release")
if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
  set(default_build_type "Debug")
endif()

if(NOT CMAKE_BUILD_TYPE)
  message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
      STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

if(CMAKE_BUILD_TYPE MATCHES "Debug")
  add_definitions("-DDEBUG=1" "-D_DEBUG=1")
  if ((NOT ANDROID) AND (("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang") OR ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")))
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer")
    set(CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer")
else()
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    set(CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_LINKER_FLAGS_DEBUG}")
  endif()
elseif(CMAKE_BUILD_TYPE MATCHES "Release")
  add_definitions("-DNDEBUG=1")
else()
  message(FATAL_ERROR "Unknown build configuration \"${CMAKE_BUILD_TYPE}\": must be Debug or Release")
endif()

#increase warning level
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-missing-field-initializers -Wshadow-all -Wshorten-64-to-32 -Wstrict-aliasing -Wuninitialized -Wunused-parameter -Wsign-compare -Wint-conversion -Wconditional-uninitialized -Wreorder -Wconstant-conversion -Wunused-private-field -Wbool-conversion -Wextra-semi -Wno-ignored-qualifiers -Wunreachable-code")

  # clang on Linux/android has a bug where it can't link with -Wl,--no-undefined when sanitizers are enabled :-(
  if (NOT (CMAKE_SYSTEM_NAME MATCHES Linux))
    string(APPEND CMAKE_SHARED_LINKER_FLAGS " -Wl,--no-undefined")
  endif()
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-missing-field-initializers -Wshadow -Wstrict-aliasing -Wuninitialized -Wunused-parameter -Wsign-compare -Wreorder -Wno-ignored-qualifiers -Wunreachable-code")
  string(APPEND CMAKE_SHARED_LINKER_FLAGS " -Wl,--no-undefined")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
endif()

# Actual Library
add_library(cxxnetaddr OBJECT NetworkAddress.cpp NetworkAddress.hpp NetworkInterface.cpp NetworkInterface.hpp)
target_compile_definitions(cxxnetaddr PRIVATE)
target_include_directories(cxxnetaddr PRIVATE)
target_link_libraries(cxxnetaddr PRIVATE)
