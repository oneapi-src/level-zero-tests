/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef __TEST_IPC_COMM_HPP__

#define __TEST_IPC_COMM_HPP__

#include <level_zero/ze_api.h>
#include <cstddef>
#include <boost/asio.hpp>
#include <chrono>

namespace boost_ip = boost::asio::ip;

namespace level_zero_tests {

#ifdef __linux__
int read_fd_from_socket(int socket);
int receive_ipc_handle();
int write_fd_to_socket(int socket, int fd);
void send_ipc_handle(const ze_ipc_mem_handle_t &ipc_handle);
#endif

const std::chrono::milliseconds CONNECTION_WAIT =
    std::chrono::milliseconds(1000);
const std::chrono::milliseconds CONNECTION_TIMEOUT = CONNECTION_WAIT * 10;

} // namespace level_zero_tests

#endif
