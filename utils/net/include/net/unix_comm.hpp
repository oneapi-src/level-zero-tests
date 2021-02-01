/*
 *
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef __UNIX_COMM_HPP__
#define __UNIX_COMM_HPP__

#include <level_zero/ze_api.h>
#include <chrono>

namespace level_zero_tests {

#ifdef __linux__
int read_fd_from_socket(int socket);
int write_fd_to_socket(int socket, int fd);
#endif

const std::chrono::milliseconds CONNECTION_WAIT =
    std::chrono::milliseconds(1000);
const std::chrono::milliseconds CONNECTION_TIMEOUT = CONNECTION_WAIT * 10;

} // namespace level_zero_tests

#endif
