# Copyright (C) 2026 Intel Corporation
# SPDX-License-Identifier: MIT

add_core_library(command_line
    SOURCE
    "include/command_line/command_line.hpp"
    "src/command_line.cpp"
)

if (NOT BUILD_ZE_PERF_TESTS_ONLY)
    add_core_library_test(command_line
        SOURCE
        "test/main.cpp"
        "test/command_line_tests.cpp"
    )
endif()
