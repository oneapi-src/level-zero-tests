# Copyright (C) 2019-2025 Intel Corporation
# SPDX-License-Identifier: MIT
find_program(CLANG_FORMAT NAMES clang-format-14)
if(CLANG_FORMAT)
    message(STATUS "Found clang-format: ${CLANG_FORMAT}")
    add_custom_target(clang-format
      COMMENT "Checking code formatting and fixing issues"
      COMMAND
        ${CMAKE_SOURCE_DIR}/clang-format-patch.sh ${CMAKE_SOURCE_DIR} |
        git -C ${CMAKE_SOURCE_DIR} apply -
    )
    add_custom_target(clang-format-check
      COMMENT "Checking code formatting"
      COMMAND
        ${CMAKE_SOURCE_DIR}/clang-format-patch.sh ${CMAKE_SOURCE_DIR}
        | tee ${CMAKE_BINARY_DIR}/clang_format_results.patch
      COMMAND ! [ -s ${CMAKE_BINARY_DIR}/clang_format_results.patch ]
    )
else()
    message(WARNING "clang-format not found. clang-format and clang-format-check targets will be disabled.")
endif()
