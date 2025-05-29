# Copyright (C) 2025 Intel Corporation
# SPDX-License-Identifier: MIT

add_compile_definitions(GTEST_DONT_DEFINE_TEST_F=1)
add_compile_definitions(GTEST_DONT_DEFINE_TEST=1)
add_compile_definitions(GTEST_HAS_EXCEPTIONS=0)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
