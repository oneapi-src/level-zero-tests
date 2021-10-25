/*
 *
 * Copyright (C) 2019-2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "ze_peer.h"

int ZePeer::sendmsg_fd(int socket, int fd) {
  char sendBuf[sizeof(ze_ipc_mem_handle_t)] = {};
  char cmsgBuf[CMSG_SPACE(sizeof(ze_ipc_mem_handle_t))];

  struct iovec msgBuffer;
  msgBuffer.iov_base = sendBuf;
  msgBuffer.iov_len = sizeof(*sendBuf);

  struct msghdr msgHeader = {};
  msgHeader.msg_iov = &msgBuffer;
  msgHeader.msg_iovlen = 1;
  msgHeader.msg_control = cmsgBuf;
  msgHeader.msg_controllen = CMSG_LEN(sizeof(fd));

  struct cmsghdr *controlHeader = CMSG_FIRSTHDR(&msgHeader);
  controlHeader->cmsg_type = SCM_RIGHTS;
  controlHeader->cmsg_level = SOL_SOCKET;
  controlHeader->cmsg_len = CMSG_LEN(sizeof(fd));

  *(int *)CMSG_DATA(controlHeader) = fd;
  ssize_t bytesSent = sendmsg(socket, &msgHeader, 0);
  if (bytesSent < 0) {
    return -1;
  }

  return 0;
}

int ZePeer::recvmsg_fd(int socket) {
  int fd = -1;
  char recvBuf[sizeof(ze_ipc_mem_handle_t)] = {};
  char cmsgBuf[CMSG_SPACE(sizeof(ze_ipc_mem_handle_t))];

  struct iovec msgBuffer;
  msgBuffer.iov_base = recvBuf;
  msgBuffer.iov_len = sizeof(recvBuf);

  struct msghdr msgHeader = {};
  msgHeader.msg_iov = &msgBuffer;
  msgHeader.msg_iovlen = 1;
  msgHeader.msg_control = cmsgBuf;
  msgHeader.msg_controllen = CMSG_LEN(sizeof(fd));

  ssize_t bytesSent = recvmsg(socket, &msgHeader, 0);
  if (bytesSent < 0) {
    return -1;
  }

  struct cmsghdr *controlHeader = CMSG_FIRSTHDR(&msgHeader);
  if (!controlHeader) {
    std::cerr << "Error receiving ipc handle";
    std::terminate();
  }
  memmove(&fd, CMSG_DATA(controlHeader), sizeof(int));
  return fd;
}

void ZePeer::set_up_ipc(int number_buffer_elements, uint32_t device_id,
                        size_t &buffer_size,
                        ze_command_queue_handle_t &command_queue,
                        ze_command_list_handle_t &command_list, bool validate) {

  if (validate) {
    warm_up_iterations = 0;
    number_iterations = 1;
  }

  size_t element_size = sizeof(char);
  buffer_size = element_size * number_buffer_elements;

  void *ze_buffer = nullptr;
  benchmark->memoryAlloc(device_id, buffer_size, &ze_buffer);
  ze_buffers[device_id] = ze_buffer;

  void **host_buffer = reinterpret_cast<void **>(&ze_host_buffer);
  benchmark->memoryAllocHost(buffer_size, host_buffer);
  void **host_validate_buffer =
      reinterpret_cast<void **>(&ze_host_validate_buffer);
  benchmark->memoryAllocHost(buffer_size, host_validate_buffer);

  command_list = ze_peer_devices[device_id].command_lists[0];
  command_queue = ze_peer_devices[device_id].command_queues[0];
}

void ZePeer::bandwidth_latency_ipc(bool bidirectional, peer_test_t test_type,
                                   peer_transfer_t transfer_type,
                                   bool is_server, int commSocket,
                                   int number_buffer_elements,
                                   uint32_t local_device_id,
                                   uint32_t remote_device_id, bool validate) {

  size_t buffer_size = 0;
  ze_command_queue_handle_t command_queue = {};
  ze_command_list_handle_t command_list = {};

  set_up_ipc(number_buffer_elements, local_device_id, buffer_size,
             command_queue, command_list, validate);

  if (is_server == false) {
    if (transfer_type == PEER_READ) {
      initialize_buffers(command_list, command_queue, nullptr, ze_host_buffer,
                         buffer_size);
    } else {
      initialize_buffers(command_list, command_queue,
                         ze_buffers[local_device_id], ze_host_buffer,
                         buffer_size);
    }

    int dma_buf_fd = recvmsg_fd(commSocket);
    if (dma_buf_fd < 0) {
      std::cerr << "Failing to get dma_buf fd from server\n";
      std::terminate();
    }
    ze_ipc_mem_handle_t pIpcHandle;
    memcpy(&pIpcHandle, static_cast<void *>(&dma_buf_fd), sizeof(dma_buf_fd));
    benchmark->memoryOpenIpcHandle(local_device_id, pIpcHandle,
                                   &ze_buffers[remote_device_id]);

  } else {
    if (transfer_type == PEER_READ) {
      initialize_buffers(command_list, command_queue,
                         ze_buffers[local_device_id], ze_host_buffer,
                         buffer_size);
    } else {
      initialize_buffers(command_list, command_queue, nullptr, ze_host_buffer,
                         buffer_size);
    }

    benchmark->getIpcHandle(ze_buffers[local_device_id], &pIpcHandle);
    int dma_buf_fd;
    memcpy(static_cast<void *>(&dma_buf_fd), &pIpcHandle, sizeof(dma_buf_fd));
    if (sendmsg_fd(commSocket, static_cast<int>(dma_buf_fd)) < 0) {
      std::cerr << "Failing to send dma_buf fd to client\n";
      std::terminate();
    }
  }

  if (is_server == false) {
    if (transfer_type == PEER_READ) {
      perform_copy(test_type, command_list, command_queue,
                   ze_buffers[local_device_id], ze_buffers[remote_device_id],
                   buffer_size);

      if (validate) {
        validate_buffer(command_list, command_queue, ze_host_validate_buffer,
                        ze_buffers[local_device_id], ze_host_buffer,
                        buffer_size);
      }
    } else {
      perform_copy(test_type, command_list, command_queue,
                   ze_buffers[remote_device_id], ze_buffers[local_device_id],
                   buffer_size);
    }

  } else {
    // Wait for client to exit
    int child_status;
    pid_t clientPId = wait(&child_status);
    if (clientPId <= 0) {
      std::cerr << "Client terminated abruptly with error code "
                << strerror(errno) << "\n";
      std::terminate();
    }
    if (transfer_type == PEER_WRITE) {
      if (validate) {
        validate_buffer(command_list, command_queue, ze_host_validate_buffer,
                        ze_buffers[local_device_id], ze_host_buffer,
                        buffer_size);
      }
    }
  }

  if (is_server == false) {
    benchmark->closeIpcHandle(ze_buffers[remote_device_id]);
  }

  benchmark->memoryFree(ze_buffers[local_device_id]);
  benchmark->memoryFree(ze_host_buffer);
  benchmark->memoryFree(ze_host_validate_buffer);

  exit(0);
}
