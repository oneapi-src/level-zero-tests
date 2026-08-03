# Copyright (C) 2026 Intel Corporation
# SPDX-License-Identifier: MIT

if(TARGET level_zero_tests::level_zero_loader)
  return()
endif()

set(LZT_LEVEL_ZERO_SOURCE_DIR "" CACHE PATH
  "Existing Level Zero source checkout. If empty, the loader is fetched.")
set(LZT_LEVEL_ZERO_GIT_REPOSITORY "https://github.com/oneapi-src/level-zero.git"
  CACHE STRING "Level Zero loader repository fetched when no checkout is provided")
set(LZT_LEVEL_ZERO_GIT_TAG "" CACHE STRING
  "Pin the fetched loader to a revision (tag or commit). Empty = fetch the latest release tag.")

set(BUILD_STATIC ON CACHE BOOL "Build the upstream Level Zero loader statically" FORCE)
set(BUILD_L0_LOADER_TESTS OFF CACHE BOOL "Do not build upstream loader tests" FORCE)
set(BUILD_INSTALLER OFF CACHE BOOL "" FORCE)

set(MSVC_BUILD_L0_DYNAMIC_VCRUNTIME ON CACHE BOOL
  "Build the static loader against the dynamic MSVC CRT (/MD) to match the tests"
  FORCE)

set(_lzt_l0_third_party "${CMAKE_SOURCE_DIR}/third_party/level-zero")

if(LZT_LEVEL_ZERO_SOURCE_DIR AND EXISTS "${LZT_LEVEL_ZERO_SOURCE_DIR}/CMakeLists.txt")
  set(_lzt_l0_source "${LZT_LEVEL_ZERO_SOURCE_DIR}")
  set(_lzt_l0_origin "provided checkout")
elseif(EXISTS "${_lzt_l0_third_party}/CMakeLists.txt")
  set(_lzt_l0_source "${_lzt_l0_third_party}")
  set(_lzt_l0_origin "third_party checkout")
else()
  set(_lzt_l0_ref "${LZT_LEVEL_ZERO_GIT_TAG}")
  if(NOT _lzt_l0_ref)
    execute_process(
      COMMAND git ls-remote --tags --refs --sort=-version:refname
              "${LZT_LEVEL_ZERO_GIT_REPOSITORY}" "v*"
      OUTPUT_VARIABLE _lzt_l0_ls_out
      RESULT_VARIABLE _lzt_l0_ls_rc
      ERROR_VARIABLE _lzt_l0_ls_err
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT _lzt_l0_ls_rc EQUAL 0 OR _lzt_l0_ls_out STREQUAL "")
      message(FATAL_ERROR
        "Static loader: could not query release tags from "
        "'${LZT_LEVEL_ZERO_GIT_REPOSITORY}' (git ls-remote rc=${_lzt_l0_ls_rc}).\n"
        "Pin a revision with -DLZT_LEVEL_ZERO_GIT_TAG=<tag> or provide a "
        "checkout via -DLZT_LEVEL_ZERO_SOURCE_DIR=<path>.\n${_lzt_l0_ls_err}")
    endif()
    string(REGEX MATCH "refs/tags/([^\t\r\n]+)" _lzt_l0_match "${_lzt_l0_ls_out}")
    set(_lzt_l0_ref "${CMAKE_MATCH_1}")
    if(NOT _lzt_l0_ref)
      message(FATAL_ERROR
        "Static loader: could not parse a release tag from git ls-remote output.")
    endif()
    set(_lzt_l0_origin "cloned latest release ${_lzt_l0_ref}")
  else()
    set(_lzt_l0_origin "cloned pinned ${_lzt_l0_ref}")
  endif()

  message(STATUS
    "Static loader: cloning Level Zero ${_lzt_l0_ref} into '${_lzt_l0_third_party}'")
  execute_process(
    COMMAND git clone --depth 1 --branch "${_lzt_l0_ref}"
            "${LZT_LEVEL_ZERO_GIT_REPOSITORY}" "${_lzt_l0_third_party}"
    RESULT_VARIABLE _lzt_l0_clone_rc
    ERROR_VARIABLE _lzt_l0_clone_err)
  if(NOT _lzt_l0_clone_rc EQUAL 0)
    # --branch only accepts a tag/branch name; retry for a raw commit SHA.
    file(REMOVE_RECURSE "${_lzt_l0_third_party}")
    execute_process(
      COMMAND git clone "${LZT_LEVEL_ZERO_GIT_REPOSITORY}" "${_lzt_l0_third_party}"
      RESULT_VARIABLE _lzt_l0_clone_rc
      ERROR_VARIABLE _lzt_l0_clone_err)
    if(_lzt_l0_clone_rc EQUAL 0)
      execute_process(
        COMMAND git -C "${_lzt_l0_third_party}" checkout --quiet "${_lzt_l0_ref}"
        RESULT_VARIABLE _lzt_l0_clone_rc
        ERROR_VARIABLE _lzt_l0_clone_err)
    endif()
  endif()
  if(NOT _lzt_l0_clone_rc EQUAL 0)
    message(FATAL_ERROR
      "Static loader: failed to clone ${LZT_LEVEL_ZERO_GIT_REPOSITORY} @ "
      "${_lzt_l0_ref}.\n${_lzt_l0_clone_err}")
  endif()
  set(_lzt_l0_source "${_lzt_l0_third_party}")
endif()

set(_lzt_l0_binary "${CMAKE_BINARY_DIR}/level-zero-static")

message(STATUS "Static loader: using ${_lzt_l0_origin} from '${_lzt_l0_source}'")

add_subdirectory("${_lzt_l0_source}" "${_lzt_l0_binary}" EXCLUDE_FROM_ALL)

if(NOT TARGET ze_loader)
  message(FATAL_ERROR
    "The Level Zero source at '${_lzt_l0_source}' did not define the "
    "'ze_loader' target; that revision may be incompatible with "
    "cmake/static_loader.cmake.")
endif()

set_target_properties(ze_loader PROPERTIES POSITION_INDEPENDENT_CODE ON)

# The Level Zero source ships the API headers flat in include/; stage a
# <level_zero/...>-prefixed view since every consumer includes <level_zero/...>.
set(LZT_LEVEL_ZERO_STAGED_INCLUDE "${CMAKE_BINARY_DIR}/level-zero-include")
file(COPY "${_lzt_l0_source}/include/"
  DESTINATION "${LZT_LEVEL_ZERO_STAGED_INCLUDE}/level_zero")

add_library(level_zero_tests_level_zero_loader INTERFACE)
add_library(level_zero_tests::level_zero_loader
  ALIAS level_zero_tests_level_zero_loader)

target_include_directories(level_zero_tests_level_zero_loader
  INTERFACE "${LZT_LEVEL_ZERO_STAGED_INCLUDE}")

target_link_libraries(level_zero_tests_level_zero_loader INTERFACE ze_loader)

add_library(level_zero_tests::level_zero
  ALIAS level_zero_tests_level_zero_loader)

set(LevelZero_INCLUDE_DIR "${LZT_LEVEL_ZERO_STAGED_INCLUDE}"
  CACHE PATH "Level Zero headers (staged from the source checkout)" FORCE)
set(LevelZero_INCLUDE_DIRS "${LZT_LEVEL_ZERO_STAGED_INCLUDE}")

message(STATUS "Static loader: headers staged at '${LZT_LEVEL_ZERO_STAGED_INCLUDE}'")
