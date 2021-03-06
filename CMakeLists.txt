## Copyright 2009-2021 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.15)

project(ospray_studio VERSION 0.7.0 LANGUAGES CXX)

include(GNUInstallDirs)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set (CMAKE_CXX_STANDARD 14 CACHE STRING
     "C++ standard for preference (11, 14, 17, 20, etc.)")
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_EXTENSIONS OFF)
message (STATUS "This build is for C++${CMAKE_CXX_STANDARD}")

# option(OSPSTUDIO_USE_CXX11_ABI "Use the 2011 C++ standard ABI" OFF)
# mark_as_advanced(OSPSTUDIO_USE_CXX11_ABI)
# if(NOT OSPSTUDIO_USE_CXX11_ABI)
#   if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
#     # Currently, OSPRay is compiled with C++11, so we have ensure a compatible ABI when using gcc
#     set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GLIBCXX_USE_CXX11_ABI=0")
#     message (STATUS "...with ${CMAKE_CXX_FLAGS}")
#   endif()
# endif()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo")
if (WIN32)
  if (NOT OSPRAY_DEFAULT_CMAKE_CONFIGURATION_TYPES_SET)
    set(CMAKE_CONFIGURATION_TYPES "${CONFIGURATION_TYPES}"
        CACHE STRING "List of generated configurations." FORCE)
    set(OSPRAY_DEFAULT_CMAKE_CONFIGURATION_TYPES_SET ON
        CACHE INTERNAL "Default CMake configuration types set.")
  endif()
else()
  if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the build type." FORCE)
  endif()
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${CONFIGURATION_TYPES})
endif()

## globals
set(OSPRAY_STUDIO_RESOURCE_FILE "${PROJECT_SOURCE_DIR}/resources/ospray_studio.rc")
if(APPLE)
  ## ensures that build-time executables contain install-time rpaths, ensuring
  ## they are identical from a code-signing POV
  set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
endif()

list(APPEND CMAKE_MODULE_PATH
  ${PROJECT_SOURCE_DIR}/cmake
)
include(functions)

## Find dependencies ##

set(OpenGL_GL_PREFERENCE "LEGACY")
find_package(OpenGL 2 REQUIRED)

include(glfw)
include(ospray)

## Build application ##

set(OSPRAY_DEFAULT_COMPONENT apps)

add_subdirectory(external)
add_subdirectory(sg)

## make pysg module optional

option(USE_PYSG "Enable PYSG module" ON)

if(USE_PYSG)
  include(cmake/pybind11.cmake)
  add_subdirectory(pysg)
endif()

option(BUILD_APPS "Build OSPRay Studio Apps" ON)

if (BUILD_APPS)
  add_subdirectory(app)
endif()

## Build plugins ##

option(BUILD_PLUGINS "Build plugins subtree" OFF)

if (BUILD_APPS AND BUILD_PLUGINS)
  add_subdirectory(plugins)
endif()


include(package)
# must be last
include(CPack)
