#
#Copyright(C) 2023 Intel Corporation
#
#SPDX - License - Identifier : MIT
#

add_lzt_test(
    NAME test_sysman_init
    GROUP   "/conformance_tests/tools/sysman"
    SOURCES
    src/test_init.cpp
    src/main.cpp
    LINK_LIBRARIES level_zero_tests::logging level_zero_tests::utils)

