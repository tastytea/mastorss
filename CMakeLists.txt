# Support version 3.9 and above, but use policy settings up to 3.14.
# 3.9 is needed for project description.
cmake_minimum_required(VERSION 3.9...3.14)
# Ranges are supported from 3.12, set policy to current for < 3.12.
if(${CMAKE_VERSION} VERSION_LESS 3.12)
  cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

# Global build options.
set(CMAKE_BUILD_TYPE "Release" CACHE STRING "The type of build.")
option(BUILD_SHARED_LIBS "Build shared libraries." YES)

project (mastorss
  VERSION 0.11.1
  DESCRIPTION "Another RSS to Mastodon bot."
  LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# Project build options.
option(WITH_MAN "Compile and install manpage." YES)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(debug_flags)

include(GNUInstallDirs)

add_subdirectory(src)

if(WITH_MAN)
  add_subdirectory(man)
endif()

install(FILES watchwords.json
  DESTINATION "${CMAKE_INSTALL_DATADIR}/mastorss")

# include(cmake/packages.cmake)
