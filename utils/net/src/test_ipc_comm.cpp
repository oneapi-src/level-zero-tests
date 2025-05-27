/*
 *
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <chrono>
#include <thread>
#ifdef __linux__
#include <sys/socket.h>
#endif
#include "net/test_ipc_comm.hpp"
#include "logging/logging.hpp"

namespace level_zero_tests {

#ifdef __linux__
int read_fd_from_socket(int unix_socket, char *data) {
  // call recvmsg to receive the descriptor on the unix_socket
  int fd = -1;
  char cmsg_buff[CMSG_SPACE(ZE_MAX_IPC_HANDLE_SIZE)];

  struct iovec msg_buffer;
  msg_buffer.iov_base = data;
  msg_buffer.iov_len = ZE_MAX_IPC_HANDLE_SIZE;

  struct msghdr msg_header = {};
  msg_header.msg_iov = &msg_buffer;
  msg_header.msg_iovlen = 1;
  msg_header.msg_control = cmsg_buff;
  msg_header.msg_controllen = CMSG_LEN(sizeof(fd));

  auto bytes = recvmsg(unix_socket, &msg_header, 0);
  if (bytes == -1) {
    throw std::runtime_error(
        "Client: Error receiving ipc handle (file descriptor)");
  }

  struct cmsghdr *control_header = CMSG_FIRSTHDR(&msg_header);
  if (control_header == nullptr) {
    throw std::runtime_error("Client: Error getting control header");
  }
  memmove(&fd, CMSG_DATA(control_header), sizeof(int));
  return fd;
}

int write_fd_to_socket(int unix_socket, int fd,
                       char *data) { // fd is the ipc handle
  char cmsg_buff[CMSG_SPACE(ZE_MAX_IPC_HANDLE_SIZE)];

  struct iovec msg_buffer;
  msg_buffer.iov_base = data;
  msg_buffer.iov_len = ZE_MAX_IPC_HANDLE_SIZE;

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

#endif

} // namespace level_zero_tests
