# Copyright (C) 2019 Intel Corporation
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.8.0 FATAL_ERROR)
if(POLICY CMP0074)
  cmake_policy(SET CMP0074 NEW)
endif()
project(level-zero-tests)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(clang_tools)
include(custom_functions)
include(vs2019_missing_redist_workaround)

find_package(PNG REQUIRED)
find_package(Git)

if(NOT TARGET GTest::GTest AND NOT TARGET GMock::GMock)
    if(NOT DEFINED GTEST_ROOT)
        set(GTEST_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/googletest")
        if(NOT EXISTS "${GTEST_ROOT}/CMakeLists.txt")
          if(Git_FOUND AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
            set(SUBMODULE_INIT_CMD submodule update --init --recursive)
            set(SUBMODULE_INIT_CMD_STRING "submodule update --init --recursive")
            message(WARNING "Unable to locate third_party/googletest submodule in checkout - Trying to fetch with ${GIT_EXECUTABLE} ${SUBMODULE_INIT_CMD_STRING}")
            execute_process(
              COMMAND ${GIT_EXECUTABLE} ${SUBMODULE_INIT_CMD}
              WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
              RESULT_VARIABLE SUBMODULE_INIT_RESULT)
            if(NOT SUBMODULE_INIT_RESULT EQUAL "0")
              message(FATAL_ERROR "Google test submodule checkout failed with ${SUBMODULE_INIT_RESULT}")
            endif()
          endif()
        endif()
    endif()
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    add_subdirectory("${GTEST_ROOT}" "${GTEST_ROOT}/build" EXCLUDE_FROM_ALL)
    add_library(GTest::GTest ALIAS gtest)
    add_library(GMock::GMock ALIAS gmock)
endif()
enable_testing()

if(NOT BUILD_ZE_PERF_TESTS_ONLY)
  set(Boost_USE_STATIC_LIBS ON)
  set(Boost_USE_MULTITHREADED ON)
  set(Boost_USE_STATIC_RUNTIME OFF)
  find_package(Boost 1.65 REQUIRED COMPONENTS log program_options timer chrono system)
endif()

find_package(LevelZero REQUIRED COMPONENTS ${LevelZero_COMPONENTS})

option(REQUIRE_OPENCL_BENCHMARKS
  "Ensures that the OpenCL benchmarks are also built (requires OpenCL)"
  NO
)
if(REQUIRE_OPENCL_BENCHMARKS)
  find_package(OpenCL 2.1 REQUIRED)
  if(NOT OpenCL_FOUND)
    message(ERROR "OpenCL Benchmarks were requested but OpenCL was not found.")
  endif()
endif()

set(MEDIA_ROOT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/mediadata")
set(MEDIA_DIRECTORY "${MEDIA_ROOT_DIRECTORY}/merged")
set(MEDIADATA_ROOT "${MEDIA_ROOT_DIRECTORY}/external")
file(COPY "${MEDIA_ROOT_DIRECTORY}/internal/bmp" DESTINATION "${MEDIA_DIRECTORY}")
file(COPY "${MEDIA_ROOT_DIRECTORY}/internal/png" DESTINATION "${MEDIA_DIRECTORY}")

if(NOT DEFINED GROUP)
    set(GROUP "/")
endif()

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/out" CACHE PATH "..." FORCE)
endif()

function(assert_valid_test_group_name group_name)
    set(regex "^([/]([A-Za-z0-9_]*))*$")
    string(REGEX MATCH "${regex}" valid_name "${group_name}")
    if(NOT "${group_name}" STREQUAL "${valid_name}")
        message(FATAL_ERROR "${group_name} is not a valid test group name, valid regex: ${regex}")
    endif()
endfunction()

# Cmake 3.8 doesn't support the list(SUBLIST ...) command
function(list_sublist list_var start_index length result_var)
    if(length EQUAL 0)
        set("${result_var}" "" PARENT_SCOPE)
        return()
    endif()
    math(EXPR end_index "${start_index}+${length}-1")
    set("${result_var}" "")
    foreach(index RANGE ${start_index} ${end_index})
        list(GET "${list_var}" "${index}" item)
        list(APPEND "${result_var}" "${item}")
    endforeach()
    set("${result_var}" "${${result_var}}" PARENT_SCOPE)
endfunction()

function(is_test_group_member group1 group2 result_var)
    assert_valid_test_group_name("${group1}")
    assert_valid_test_group_name("${group2}")
    string(REGEX MATCHALL "([A-Za-z0-9_])+" group1_list "${group1}")
    string(REGEX MATCHALL "([A-Za-z0-9_])+" group2_list "${group2}")
    list(LENGTH group1_list group1_list_length)
    list(LENGTH group2_list group2_list_length)
    if(group1_list_length GREATER group2_list_length)
        set(list_length ${group2_list_length})
    else()
        set(list_length ${group1_list_length})
    endif()
    # list(SUBLIST group2_list 0 "${group1_list_length}" group2_common)
    list_sublist(group2_list 0 "${list_length}" group2_common)
    if(group2_common STREQUAL group1_list)
        set("${result_var}" "TRUE" PARENT_SCOPE)
    else()
        set("${result_var}" "FALSE" PARENT_SCOPE)
    endif()
endfunction()

function(add_lzt_test_executable)
    set(options "")
    set(oneValueArgs NAME PREFIX GROUP EXTENDED)
    set(multiValueArgs SOURCES INCLUDE_DIRECTORIES LINK_LIBRARIES KERNELS MEDIA)
    cmake_parse_arguments(ADD_LZT_TEST_EXECUTABLE
      "${options}" "${oneValueArgs}" "${multiValueArgs}"
      ${ARGN}
    )

    assert_valid_test_group_name("${ADD_LZT_TEST_EXECUTABLE_GROUP}")
  
    if(DEFINED EXTENDED_BUILD)
        if(NOT DEFINED ADD_LZT_TEST_EXECUTABLE_EXTENDED)
          message(STATUS "Ignoring ${ADD_LZT_TEST_EXECUTABLE_NAME} because it is not an extended test")
          return()
        endif()
    endif()

    is_test_group_member("${GROUP}" "${ADD_LZT_TEST_EXECUTABLE_GROUP}" in_scope)
    if(NOT in_scope)
        return()
    endif()

    is_test_group_member("/conformance_tests" "${ADD_LZT_TEST_EXECUTABLE_GROUP}" is_conformance_test)
    is_test_group_member("/perf_tests" "${ADD_LZT_TEST_EXECUTABLE_GROUP}" is_perf_test)
    is_test_group_member("/negative_tests" "${ADD_LZT_TEST_EXECUTABLE_GROUP}" is_negative_test)
    is_test_group_member("/layer_tests" "${ADD_LZT_TEST_EXECUTABLE_GROUP}" is_layer_test)
    is_test_group_member("/stress_tests" "${ADD_LZT_TEST_EXECUTABLE_GROUP}" is_stress_test)
    if(is_conformance_test)
        list(APPEND ADD_LZT_TEST_EXECUTABLE_LINK_LIBRARIES
          level_zero_tests::test_harness
          GMock::GMock
          GTest::GTest
        )
        set(component "conformance-tests")
    elseif(is_negative_test)
        list(APPEND ADD_LZT_TEST_EXECUTABLE_LINK_LIBRARIES
          level_zero_tests::test_harness
          GMock::GMock
          GTest::GTest
        )
        set(component "negative-tests")
    elseif(is_perf_test)
        list(APPEND ADD_LZT_TEST_EXECUTABLE_INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_LIST_DIR}/../common/include
        )
        set(component "perf-tests")
    elseif(is_layer_test)
        list(APPEND ADD_LZT_TEST_EXECUTABLE_LINK_LIBRARIES
          level_zero_tests::test_harness
          GMock::GMock
          GTest::GTest
        )
        set(component "layer-tests")
    elseif(is_stress_test)
        list(APPEND ADD_LZT_TEST_EXECUTABLE_LINK_LIBRARIES
          level_zero_tests::test_harness
          GMock::GMock
          GTest::GTest
        )
        set(component "stress-tests")
    endif()

    add_executable(${ADD_LZT_TEST_EXECUTABLE_NAME} ${ADD_LZT_TEST_EXECUTABLE_SOURCES})

    target_link_libraries(${ADD_LZT_TEST_EXECUTABLE_NAME}
      PRIVATE
        LevelZero::LevelZero
        ${ADD_LZT_TEST_EXECUTABLE_LINK_LIBRARIES}
    )

    target_include_directories(${ADD_LZT_TEST_EXECUTABLE_NAME}
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${ADD_LZT_TEST_EXECUTABLE_INCLUDE_DIRECTORIES}
    )

    set(destination "${CMAKE_INSTALL_PREFIX}/${ADD_LZT_TEST_EXECUTABLE_PREFIX}")

    foreach(kernel ${ADD_LZT_TEST_EXECUTABLE_KERNELS})
        install(
          FILES "${CMAKE_CURRENT_SOURCE_DIR}/kernels/${kernel}.spv"
          DESTINATION ${destination}
        )
    endforeach()

    foreach(media ${ADD_LZT_TEST_EXECUTABLE_MEDIA})
        install(
          FILES "${MEDIA_DIRECTORY}/${media}"
          DESTINATION ${destination}
        )
    endforeach()

    install(
      TARGETS ${ADD_LZT_TEST_EXECUTABLE_NAME}
      DESTINATION ${destination}
      COMPONENT ${component}
    )

    if(MSVC)
        set_target_properties(${ADD_LZT_TEST_EXECUTABLE_NAME}
          PROPERTIES
            VS_DEBUGGER_COMMAND_ARGUMENTS ""
            VS_DEBUGGER_WORKING_DIRECTORY "$(OutDir)"
        )
    endif()
endfunction()

if(NOT DEFINED MANIFEST_BUILD_PATH)
  set(MANIFEST_BUILD_PATH "${PROJECT_BINARY_DIR}/manifest.txt")
endif()
install(FILES "${MANIFEST_BUILD_PATH}" DESTINATION "${CMAKE_INSTALL_PREFIX}")

function(add_lzt_test)
    set(oneValueArgs NAME GROUP EXTENDED)
    set(multiValueArgs SOURCES INCLUDE_DIRECTORIES LINK_LIBRARIES KERNELS MEDIA)
    cmake_parse_arguments(ADD_LZT_TEST
      "${options}" "${oneValueArgs}" "${multiValueArgs}"
      ${ARGN}
    )

    assert_valid_test_group_name("${ADD_LZT_TEST_GROUP}")

    if(DEFINED EXTENDED_BUILD)
      if(NOT DEFINED ADD_LZT_TEST_EXTENDED)
        message(STATUS "Ignoring ${ADD_LZT_TEST_NAME} because it is not an extended test")
        return()
      endif()
    endif()

    is_test_group_member("${GROUP}" "${ADD_LZT_TEST_GROUP}" in_scope)
    if(NOT in_scope)
        message(STATUS "Ignoring ${ADD_LZT_TEST_NAME} because it is not in ${GROUP}")
        return()
    else()
        message(STATUS "Adding ${ADD_LZT_TEST_NAME}")
    endif()


    add_lzt_test_executable(
      NAME ${ADD_LZT_TEST_NAME}
      GROUP ${ADD_LZT_TEST_GROUP}
      SOURCES ${ADD_LZT_TEST_SOURCES}
      INCLUDE_DIRECTORIES ${ADD_LZT_TEST_INCLUDE_DIRECTORIES}
      LINK_LIBRARIES ${ADD_LZT_TEST_LINK_LIBRARIES}
      KERNELS ${ADD_LZT_TEST_KERNELS}
      MEDIA ${ADD_LZT_TEST_MEDIA}
      EXTENDED  ${ADD_LZT_TEST_EXTENDED}
    )

    file(APPEND
      "${MANIFEST_BUILD_PATH}"
      "${ADD_LZT_TEST_NAME}${CMAKE_NEWLINE}"
    )
endfunction()

if(WIN32)
    set(CMAKE_NEWLINE "\r\n")
else()
    set(CMAKE_NEWLINE "\n")
endif()


option(ENABLE_ZESYSMAN
  "Enables build of zesysman"
  NO
)

if(ENABLE_ZESYSMAN)
  add_subdirectory(zesysman)
endif()

if(BUILD_ZE_PERF_TESTS_ONLY)
  add_subdirectory(perf_tests)
else()
  add_subdirectory(utils)
  add_subdirectory(perf_tests)
  add_subdirectory(conformance_tests)
  add_subdirectory(negative_tests)
  add_subdirectory(layer_tests)
  add_subdirectory(stress_tests)
endif()


set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION "${CMAKE_BINARY_DIR}/out")
include(InstallRequiredSystemLibraries)
