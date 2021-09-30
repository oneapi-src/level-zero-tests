/*
 *
 * Copyright (C) 2021 Intel Corporation
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
#include <thread>
#ifdef __linux__
#include <sys/socket.h>
#endif

#include "logging/logging.hpp"

namespace boost_ip = boost::asio::ip;

namespace level_zero_tests {

const std::chrono::milliseconds CONNECTION_WAIT =
    std::chrono::milliseconds(1000);
const std::chrono::milliseconds CONNECTION_TIMEOUT = CONNECTION_WAIT * 10;

#ifdef __linux__

// declaration
int read_fd_from_socket(int socket, size_t buffer_size);
template <typename T> int receive_ipc_handle();
int write_fd_to_socket(int socket, int fd, size_t buffer_size);
template <typename T> void send_ipc_handle(const T &ipc_handle);

// definition
template <typename T> int receive_ipc_handle() {
  const char *socket_path = "ipc_socket";

  struct sockaddr_un local_addr, remote_addr;
  local_addr.sun_family = AF_UNIX;
  strcpy(local_addr.sun_path, socket_path);
  unlink(local_addr.sun_path);

  int unix_rcv_socket = socket(AF_UNIX, SOCK_STREAM, 0);
  if (unix_rcv_socket == -1) {
    perror("Server Connection Error");
    throw std::runtime_error("[Server] Could not create socket");
  }

  if (bind(unix_rcv_socket, (struct sockaddr *)&local_addr,
           strlen(local_addr.sun_path) + sizeof(local_addr.sun_family)) == -1) {
    perror("Server Bind Error");
    close(unix_rcv_socket);
    throw std::runtime_error("[Server] Could not bind to socket");
  }

  LOG_DEBUG << "[Server] Unix Socket Listening...";
  if (listen(unix_rcv_socket, 1) == -1) {
    perror("Server Listen Error");
    close(unix_rcv_socket);
    throw std::runtime_error("[Server] Could not listen on socket");
  }

  int len = sizeof(struct sockaddr_un);
  int other_socket = accept(unix_rcv_socket, (struct sockaddr *)&remote_addr,
                            (socklen_t *)&len);
  if (other_socket == -1) {
    close(unix_rcv_socket);
    perror("Server Accept Error");
    throw std::runtime_error("[Server] Could not accept connection");
  }
  LOG_DEBUG << "[Server] Connection accepted";

  int ipc_descriptor;
  if ((ipc_descriptor = read_fd_from_socket(other_socket, sizeof(T))) < 0) {
    close(other_socket);
    close(unix_rcv_socket);
    perror("Server Connection Error");
    throw std::runtime_error("[Server] Failed to read IPC handle");
  }
  LOG_DEBUG << "[Server] Received IPC handle descriptor from client";

  close(other_socket);
  close(unix_rcv_socket);
  return ipc_descriptor;
}

template <typename T> void send_ipc_handle(const T &ipc_handle) {
  const char *socket_path = "ipc_socket";

  struct sockaddr_un remote_addr;
  remote_addr.sun_family = AF_UNIX;
  strcpy(remote_addr.sun_path, socket_path);
  int unix_send_socket = socket(AF_UNIX, SOCK_STREAM, 0);
  if (unix_send_socket == -1) {
    perror("Client Connection Error");
    throw std::runtime_error(
        "[Client] IPC process could not create UNIX socket");
  }

  std::chrono::milliseconds wait = std::chrono::milliseconds(0);
  while (connect(unix_send_socket, (struct sockaddr *)&remote_addr,
                 sizeof(remote_addr)) == -1) {

    LOG_DEBUG << "Connection error, sleeping and retrying..." << std::endl;
    std::this_thread::sleep_for(CONNECTION_WAIT);
    wait += CONNECTION_WAIT;
    if (wait > CONNECTION_TIMEOUT) {
      close(unix_send_socket);
      perror("Error: ");
      throw std::runtime_error("[Client] Timed out waiting to send ipc handle");
    }
  }
  LOG_DEBUG << "[Client] Connected to server";

  int ipc_handle_id;
  memcpy(static_cast<void *>(&ipc_handle_id), &ipc_handle,
         sizeof(ipc_handle_id));
  if (write_fd_to_socket(unix_send_socket, static_cast<int>(ipc_handle_id),
                         sizeof(T)) < 0) {
    close(unix_send_socket);
    perror("Error: ");
    throw std::runtime_error("[Client] Error sending ipc handle");
  }
  LOG_DEBUG << "[Client] Wrote ipc descriptor to socket";

  close(unix_send_socket);
}

#endif

} // namespace level_zero_tests

#endif
