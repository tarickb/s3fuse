cmake_minimum_required(VERSION 3.10.0)
cmake_policy(VERSION 3.0)

execute_process(
  COMMAND head -n 1 ChangeLog
  COMMAND sed -e "s/ .*//"
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE CHANGELOG_NAME
  OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(
  COMMAND head -n 1 ChangeLog
  COMMAND sed -e "s/.*(//" -e "s/).*//"
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE CHANGELOG_VERSION
  OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(
  COMMAND git rev-parse --short --verify HEAD
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_VERSION
  OUTPUT_STRIP_TRAILING_WHITESPACE)

if (NOT (${GIT_VERSION} EQUAL ""))
  set(GIT_VERSION_SUFFIX " (${GIT_VERSION})")
else()
  set(GIT_VERSION_SUFFIX "")
endif()

project(
  ${CHANGELOG_NAME}
  VERSION ${CHANGELOG_VERSION}
  LANGUAGES CXX)

include(GNUInstallDirs)

option(EnableTests "Enable tests." ON)
option(EnableMacOSBundle "Enable macOS app bundle features." OFF)

if(EnableTests)
  enable_testing()
endif()

option(EnableAwsS3 "Enable AWS S3." ON)
option(EnableFvs "Enable FV/S." ON)
option(EnableGcs "Enable Google Cloud Storage." ON)

math(EXPR service_count 0)
set(last_service)

if(EnableAwsS3)
  math(EXPR service_count ${service_count}+1)
  add_definitions(-DWITH_AWS)
  set(last_service aws)
endif()

if(EnableFvs)
  math(EXPR service_count ${service_count}+1)
  add_definitions(-DWITH_FVS)
  set(last_service fvs)
endif()

if(EnableGcs)
  math(EXPR service_count ${service_count}+1)
  add_definitions(-DWITH_GS)
  set(last_service gs)
endif()

if(service_count LESS 1)
  message(FATAL_ERROR "Need at least one service enabled!")
endif()

if(service_count EQUAL 1)
  add_definitions("-DFIXED_SERVICE=${last_service}")
endif()

add_subdirectory(config)
add_subdirectory(man)
add_subdirectory(src)
