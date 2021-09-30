# Copyright (C) 2021 Intel Corporation
# SPDX-License-Identifier: MIT

add_core_library(net
    SOURCE
    "include/net/unix_comm.hpp"
    "include/net/test_ipc_comm.hpp"
    "src/unix_comm.cpp"
    "src/test_ipc_comm.cpp"
)

target_link_libraries(net
    PUBLIC
    level_zero_tests::logging
    LevelZero::LevelZero
)
