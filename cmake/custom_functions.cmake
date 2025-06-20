# Copyright (C) 2019 Intel Corporation
# SPDX-License-Identifier: MIT

function(add_core_library name)
    cmake_parse_arguments(F "" "" "SOURCE" ${ARGN})
    add_library(${name} ${F_SOURCE})
    add_library(level_zero_tests::${name} ALIAS ${name})

    target_include_directories(${name}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    )
endfunction()

function(add_core_library_test project_name)
    cmake_parse_arguments(F "" "" "SOURCE" ${ARGN})
    set(name "${project_name}_tests")
    add_executable(${name} ${F_SOURCE})
    add_executable(level_zero_tests::${name} ALIAS ${name})

    target_link_libraries(${name}
        PUBLIC
        level_zero_tests::${project_name}
        GTest::GTest
        level_zero_tests::utils
    )

    target_include_directories(${name}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${LevelZero_INCLUDE_DIRS}
    )

    add_test(NAME ${name} COMMAND ${name} WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
endfunction()


function(add_check_resources target)
    cmake_parse_arguments(PARSED_ARGS "" "" "FILES" ${ARGN})
    foreach(resource ${PARSED_ARGS_FILES})
        file(COPY "${resource}" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
    endforeach()
endfunction()

function(add_core_ze_library name)
    cmake_parse_arguments(F "" "" "SOURCE" ${ARGN})
    add_library(${name} ${F_SOURCE})
    add_library(level_zero_tests::${name} ALIAS ${name})

    target_include_directories(${name}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    )

    if(CLANG_TIDY_EXE)
        set_target_properties(
            ${name}
            PROPERTIES
                 CXX_CLANG_TIDY ${CLANG_TIDY_EXE}
        )
    endif()
endfunction()

function(add_core_ze_library_test project_name)
    cmake_parse_arguments(F "" "" "SOURCE" ${ARGN})
    set(name "${project_name}_tests")
    add_executable(${name} ${F_SOURCE})
    add_executable(level_zero_tests::${name} ALIAS ${name})

    target_link_libraries(${name}
        PUBLIC
        level_zero_tests::${project_name}
        GTest::GTest
    )

    if(CLANG_TIDY_EXE)
        set_target_properties(
            ${name}
            PROPERTIES
                 CXX_CLANG_TIDY ${CLANG_TIDY_EXE}
        )
    endif()

    target_include_directories(${name}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${LevelZero_INCLUDE_DIRS}
    )

    add_test(NAME ${name} COMMAND ${name} WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
endfunction()
