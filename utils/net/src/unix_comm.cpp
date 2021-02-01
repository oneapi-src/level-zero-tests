/*
 *
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifdef __linux__
#include <sys/socket.h>
#endif
#include <stdexcept>
#include <cstring>

#include "net/unix_comm.hpp"

namespace level_zero_tests {

#ifdef __linux__
int read_fd_from_socket(int unix_socket) {
  int fd = -1;
  char recv_buff[sizeof(int)] = {};
  char cmsg_buff[CMSG_SPACE(sizeof(int))];

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
    throw std::runtime_error("Client: Error receiving fd");
  }

  struct cmsghdr *control_header = CMSG_FIRSTHDR(&msg_header);
  if (control_header == nullptr) {
    throw std::runtime_error("Client: Error getting control header");
  }
  memmove(&fd, CMSG_DATA(control_header), sizeof(int));
  return fd;
}

int write_fd_to_socket(int unix_socket, int fd) {
  char send_buff[sizeof(int)];
  char cmsg_buff[CMSG_SPACE(sizeof(int))];

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
  ssize_t bytesSent = sendmsg(unix_socket, &msg_header, 0);
  if (bytesSent < 0) {
    return -1;
  }

  return 0;
}
#endif

} // namespace level_zero_tests
