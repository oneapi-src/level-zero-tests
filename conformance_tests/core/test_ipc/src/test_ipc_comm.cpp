/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <chrono>
#include <thread>
#ifdef __linux__
#include <sys/socket.h>
#endif
#include "test_ipc_comm.hpp"
#include "logging/logging.hpp"
#include "gtest/gtest.h"
#include "test_harness/test_harness.hpp"

namespace level_zero_tests {

#ifdef __linux__
const char *socket_path = "ipc_socket";

int read_fd_from_socket(int unix_socket) {
  // call recvmsg to receive the descriptor on the unix_socket
  int fd = -1;
  char recv_buff[sizeof(ze_ipc_mem_handle_t)] = {};
  char cmsg_buff[CMSG_SPACE(sizeof(ze_ipc_mem_handle_t))];

  struct iovec msg_buffer;
  msg_buffer.iov_base = recv_buff;
  msg_buffer.iov_len = sizeof(recv_buff);

  struct msghdr msg_header = {};
  msg_header.msg_iov = &msg_buffer;
  msg_header.msg_iovlen = 1;
  msg_header.msg_control = cmsg_buff;
  msg_header.msg_controllen = CMSG_LEN(sizeof(fd));

  size_t bytes = recvmsg(unix_socket, &msg_header, 0);
  if (bytes < 0) {
    throw std::runtime_error("Client: Error receiving ipc handle");
  }

  struct cmsghdr *control_header = CMSG_FIRSTHDR(&msg_header);
  if (control_header == nullptr) {
    throw std::runtime_error("Client: Error getting control header");
  }
  memmove(&fd, CMSG_DATA(control_header), sizeof(int));
  return fd;
}

int receive_ipc_handle() {
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
  if ((ipc_descriptor = lzt::read_fd_from_socket(other_socket)) < 0) {
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

int write_fd_to_socket(int unix_socket, int fd) { // fd is the ipc handle
  char send_buff[sizeof(ze_ipc_mem_handle_t)];
  char cmsg_buff[CMSG_SPACE(sizeof(ze_ipc_mem_handle_t))];

  struct iovec msg_buffer;
  msg_buffer.iov_base = send_buff;
  msg_buffer.iov_len = sizeof(*send_buff);

  // build a msghdr containing the desriptor (fd)
  // fd is sent as ancillary data, i.e. msg_control
  // member of msghdr
  struct msghdr msg_header = {};
  msg_header.msg_iov = &msg_buffer;
  msg_header.msg_iovlen = 1;
  msg_header.msg_control = cmsg_buff;
  msg_header.msg_controllen = CMSG_LEN(sizeof(fd));

  struct cmsghdr *control_header = CMSG_FIRSTHDR(&msg_header);
  control_header->cmsg_type = SCM_RIGHTS;
  control_header->cmsg_level = SOL_SOCKET;
  control_header->cmsg_len = CMSG_LEN(sizeof(fd));

  *(int *)CMSG_DATA(control_header) = fd;
  // call sendmsg to send descriptor across socket
  ssize_t bytesSent = sendmsg(unix_socket, &msg_header, 0);
  if (bytesSent < 0) {
    return -1;
  }

  return 0;
}

void send_ipc_handle(const ze_ipc_mem_handle_t &ipc_handle) {
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
    std::this_thread::sleep_for(lzt::CONNECTION_WAIT);
    wait += lzt::CONNECTION_WAIT;
    if (wait > lzt::CONNECTION_TIMEOUT) {
      close(unix_send_socket);
      perror("Error: ");
      throw std::runtime_error("[Client] Timed out waiting to send ipc handle");
    }
  }
  LOG_DEBUG << "[Client] Connected to server";

  int ipc_handle_id;
  memcpy(static_cast<void *>(&ipc_handle_id), &ipc_handle,
         sizeof(ipc_handle_id));
  if (lzt::write_fd_to_socket(unix_send_socket,
                              static_cast<int>(ipc_handle_id)) < 0) {
    close(unix_send_socket);
    perror("Error: ");
    throw std::runtime_error("[Client] Error sending ipc handle");
  }
  LOG_DEBUG << "[Client] Wrote ipc descriptor to socket";

  close(unix_send_socket);
}

#endif

} // namespace level_zero_tests
