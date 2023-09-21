# Copyright (C) 2023 Intel Corporation
# SPDX-License-Identifier: MIT

add_lzt_test(
  NAME test_loader_api
  GROUP "/layer_tests/loader_api"
  SOURCES
    src/test_loader_api.cpp
    src/main.cpp
  LINK_LIBRARIES
    level_zero_tests::logging
    level_zero_tests::utils
)


