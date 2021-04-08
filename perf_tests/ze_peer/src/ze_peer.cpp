/*
 *
 * Copyright (C) 2019-2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <level_zero/ze_api.h>

#include "common.hpp"
#include "ze_app.hpp"
#include "ze_peer.h"

#include <assert.h>
#include <stdlib.h>
#include <iomanip>
#include <iostream>
#include <math.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

static const char *usage_str =
    "\n ze_peer [OPTIONS]"
    "\n"
    "\n OPTIONS:"
    "\n  -t, string                  selectively run a particular test"
    "\n      ipc                     selectively run IPC tests"
    "\n      ipc_bw                  selectively run IPC bandwidth test"
    "\n      ipc_latency             selectively run IPC latency test"
    "\n      transfer_bw             selectively run transfer bandwidth test"
    "\n      latency                 selectively run latency test"
    "\n  -a                          run all above tests [default]"
    "\n  -q                          query for number of engines available"
    "\n  -g, number                  select engine group (default: 0)"
    "\n  -i, number                  select engine index (default: 0)"
    "\n  -h, --help                  display help message"
    "\n";

class ZePeer {
public:
  ZePeer(const uint32_t command_queue_group_ordinal,
         const uint32_t command_queue_index) {

    benchmark = new ZeApp("ze_peer_benchmarks.spv");

    uint32_t device_count = benchmark->allDevicesInit();
    for (uint32_t i = 0;
         (i < device_count - 1) && (device_environs.size() == 0); i++) {
      for (uint32_t j = i + 1; j < device_count; j++) {
        if (benchmark->canAccessPeer(i, j)) {
          if (device_environs.size() == 0) {
            device_environs.emplace_back(i);
          }
          device_environs.emplace_back(j);
        }
      }
    }

    if (device_environs.size() == 0) {
      std::cerr << "ERROR: there are no peer devices among " << device_count
                << " devices found" << std::endl;
      std::terminate();
    }

    if (device_environs.size() > 2) {
      std::cout << "WARNING: detected " << device_environs.size()
                << "devices, ze_peer currently limited to running "
                   "latency/bandwidth test with first 2 devices"
                << std::endl;
    }

    for (uint32_t i = 0; i < device_environs.size(); i++) {
      benchmark->commandQueueCreate(
          device_environs[i].device_index, command_queue_group_ordinal,
          command_queue_index, &device_environs[i].command_queue);
      benchmark->commandListCreate(device_environs[i].device_index,
                                   command_queue_group_ordinal,
                                   &device_environs[i].command_list);
    }
  }

  ~ZePeer() {
    for (uint32_t i = 0; i < device_environs.size(); i++) {
      benchmark->commandQueueDestroy(device_environs[i].command_queue);
      benchmark->commandListDestroy(device_environs[i].command_list);
    }
    benchmark->allDevicesCleanup();
    delete benchmark;
  }

  void bandwidth(bool bidirectional, peer_transfer_t transfer_type);
  void latency(bool bidirectional, peer_transfer_t transfer_type);
  void ipc_bandwidth_latency(bool bidirectional, peer_test_t test_type,
                             peer_transfer_t transfer_type, bool is_server,
                             int commSocket, int number_of_elements,
                             int device_id);
  void query_engines();

  int sendmsg_fd(int socket, int fd);
  int recvmsg_fd(int socket);

private:
  ZeApp *benchmark;
  std::vector<device_environ_t> device_environs;

  void _copy_function_setup(const uint32_t device_index,
                            ze_kernel_handle_t &function,
                            const char *function_name, uint32_t globalSizeX,
                            uint32_t globalSizeY, uint32_t globalSizeZ,
                            uint32_t &group_size_x, uint32_t &group_size_y,
                            uint32_t &group_size_z);
  void _copy_function_cleanup(ze_kernel_handle_t function);
};

void ZePeer::query_engines() {
  for (auto device : device_environs) {
    std::cout << "Device " << device.device_index << ":" << std::endl;

    uint32_t numQueueGroups = 0;
    benchmark->deviceGetCommandQueueGroupProperties(device.device_index,
                                                    &numQueueGroups, nullptr);

    if (numQueueGroups == 0) {
      std::cout << " No queue groups found\n" << std::endl;
      continue;
    }

    std::vector<ze_command_queue_group_properties_t> queueProperties(
        numQueueGroups);
    benchmark->deviceGetCommandQueueGroupProperties(
        device.device_index, &numQueueGroups, queueProperties.data());

    for (uint32_t i = 0; i < numQueueGroups; i++) {
      if (queueProperties[i].flags &
          ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE) {
        std::cout << " Group " << i
                  << " (compute): " << queueProperties[i].numQueues
                  << " queues\n";
      } else if ((queueProperties[i].flags &
                  ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE) == 0 &&
                 (queueProperties[i].flags &
                  ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COPY)) {
        std::cout << " Group " << i
                  << " (copy): " << queueProperties[i].numQueues << " queues\n";
      }
    }
    std::cout << std::endl;
  }
}

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
  memmove(&fd, CMSG_DATA(controlHeader), sizeof(int));
  return fd;
}

void ZePeer::_copy_function_setup(const uint32_t device_index,
                                  ze_kernel_handle_t &function,
                                  const char *function_name,
                                  uint32_t globalSizeX, uint32_t globalSizeY,
                                  uint32_t globalSizeZ, uint32_t &group_size_x,
                                  uint32_t &group_size_y,
                                  uint32_t &group_size_z) {
  group_size_x = 0;
  group_size_y = 0;
  group_size_z = 0;

  benchmark->functionCreate(device_index, &function, function_name);

  SUCCESS_OR_TERMINATE(
      zeKernelSuggestGroupSize(function, globalSizeX, globalSizeY, globalSizeZ,
                               &group_size_x, &group_size_y, &group_size_z));
  SUCCESS_OR_TERMINATE(
      zeKernelSetGroupSize(function, group_size_x, group_size_y, group_size_z));
}

void ZePeer::_copy_function_cleanup(ze_kernel_handle_t function) {
  benchmark->functionDestroy(function);
}

void ZePeer::bandwidth(bool bidirectional, peer_transfer_t transfer_type) {
  int number_iterations = 5;
  int warm_up_iterations = 5;
  int number_buffer_elements = 10000000;
  std::vector<void *> buffers(device_environs.size(), nullptr);
  size_t element_size = sizeof(unsigned long int);
  size_t buffer_size = element_size * number_buffer_elements;

  for (uint32_t i = 0; i < device_environs.size(); i++) {
    benchmark->memoryAlloc(device_environs[i].device_index, buffer_size,
                           &buffers[i]);
  }

  for (uint32_t i = 0; i < device_environs.size(); i++) {
    ze_kernel_handle_t function_a = nullptr;
    ze_kernel_handle_t function_b = nullptr;
    void *buffer_i = buffers.at(i);
    ze_command_list_handle_t command_list_a = device_environs[i].command_list;
    ze_command_queue_handle_t command_queue_a =
        device_environs[i].command_queue;
    ze_group_count_t thread_group_dimensions;
    uint32_t group_size_x;
    uint32_t group_size_y;
    uint32_t group_size_z;

    _copy_function_setup(device_environs[i].device_index, function_a,
                         "single_copy_peer_to_peer", number_buffer_elements, 1,
                         1, group_size_x, group_size_y, group_size_z);
    if (bidirectional) {
      _copy_function_setup(device_environs[i].device_index, function_b,
                           "single_copy_peer_to_peer", number_buffer_elements,
                           1, 1, group_size_x, group_size_y, group_size_z);
    }

    const int group_count_x = number_buffer_elements / group_size_x;
    thread_group_dimensions.groupCountX = group_count_x;
    thread_group_dimensions.groupCountY = 1;
    thread_group_dimensions.groupCountZ = 1;

    for (uint32_t j = 0; j < device_environs.size(); j++) {
      long double total_time_usec;
      long double total_time_s;
      long double total_data_transfer;
      long double total_bandwidth;
      Timer<std::chrono::microseconds::period> timer;
      void *buffer_j = buffers.at(j);

      if (bidirectional) {
        /* PEER_WRITE */
        SUCCESS_OR_TERMINATE(
            zeKernelSetArgumentValue(function_a, 0, /* Destination buffer*/
                                     sizeof(buffer_j), &buffer_j));
        SUCCESS_OR_TERMINATE(
            zeKernelSetArgumentValue(function_a, 1, /* Source buffer */
                                     sizeof(buffer_i), &buffer_i));
        /* PEER_READ */
        SUCCESS_OR_TERMINATE(
            zeKernelSetArgumentValue(function_b, 0, /* Destination buffer*/
                                     sizeof(buffer_i), &buffer_i));
        SUCCESS_OR_TERMINATE(
            zeKernelSetArgumentValue(function_b, 1, /* Source buffer */
                                     sizeof(buffer_j), &buffer_j));

        zeCommandListAppendLaunchKernel(command_list_a, function_a,
                                        &thread_group_dimensions, nullptr, 0,
                                        nullptr);
        zeCommandListAppendLaunchKernel(command_list_a, function_b,
                                        &thread_group_dimensions, nullptr, 0,
                                        nullptr);
      } else { /* unidirectional */
        if (transfer_type == PEER_WRITE) {
          SUCCESS_OR_TERMINATE(
              zeKernelSetArgumentValue(function_a, 0, /* Destination buffer*/
                                       sizeof(buffer_j), &buffer_j));
          SUCCESS_OR_TERMINATE(
              zeKernelSetArgumentValue(function_a, 1, /* Source buffer */
                                       sizeof(buffer_i), &buffer_i));
        } else if (transfer_type == PEER_READ) {
          SUCCESS_OR_TERMINATE(
              zeKernelSetArgumentValue(function_a, 0, /* Destination buffer*/
                                       sizeof(buffer_i), &buffer_i));
          SUCCESS_OR_TERMINATE(
              zeKernelSetArgumentValue(function_a, 1, /* Source buffer */
                                       sizeof(buffer_j), &buffer_j));
        } else {
          std::cerr
              << "ERROR: Bandwidth test - transfer type parameter is invalid"
              << std::endl;
          std::terminate();
        }

        zeCommandListAppendLaunchKernel(command_list_a, function_a,
                                        &thread_group_dimensions, nullptr, 0,
                                        nullptr);
      }
      benchmark->commandListClose(command_list_a);

      /* Warm up */
      for (int i = 0; i < warm_up_iterations; i++) {
        benchmark->commandQueueExecuteCommandList(command_queue_a, 1,
                                                  &command_list_a);
        benchmark->commandQueueSynchronize(command_queue_a);
      }

      timer.start();
      for (int i = 0; i < number_iterations; i++) {
        benchmark->commandQueueExecuteCommandList(command_queue_a, 1,
                                                  &command_list_a);
        benchmark->commandQueueSynchronize(command_queue_a);
      }
      timer.end();

      total_time_usec = timer.period_minus_overhead();
      total_time_s = total_time_usec / 1e6;

      total_data_transfer =
          (buffer_size * number_iterations) /
          static_cast<long double>(1e9); /* Units in Gigabytes */
      if (bidirectional) {
        total_data_transfer = 2 * total_data_transfer;
      }
      total_bandwidth = total_data_transfer / total_time_s;

      if (bidirectional) {
        std::cout << std::setprecision(11) << std::setw(8) << " Device(" << i
                  << ")<->Device(" << j << "): "
                  << " GBPS " << total_bandwidth << std::endl;
      } else {
        if (transfer_type == PEER_WRITE) {
          std::cout << std::setprecision(11) << std::setw(8) << " Device(" << i
                    << ")->Device(" << j << "): "
                    << " GBPS " << total_bandwidth << std::endl;
        } else {
          std::cout << std::setprecision(11) << std::setw(8) << " Device(" << i
                    << ")<-Device(" << j << "): "
                    << " GBPS " << total_bandwidth << std::endl;
        }
      }
      benchmark->commandListReset(command_list_a);
    }
    _copy_function_cleanup(function_a);
    if (bidirectional) {
      _copy_function_cleanup(function_b);
    }
  }

  for (void *buffer : buffers) {
    benchmark->memoryFree(buffer);
  }
}

void ZePeer::latency(bool bidirectional, peer_transfer_t transfer_type) {
  int number_iterations = 100;
  int warm_up_iterations = 5;
  int number_buffer_elements = 1;
  std::vector<void *> buffers(device_environs.size(), nullptr);
  size_t element_size = sizeof(unsigned long int);
  size_t buffer_size = element_size * number_buffer_elements;

  for (uint32_t i = 0; i < device_environs.size(); i++) {
    benchmark->memoryAlloc(device_environs[i].device_index, buffer_size,
                           &buffers[i]);
  }

  for (uint32_t i = 0; i < device_environs.size(); i++) {
    ze_kernel_handle_t function_a = nullptr;
    ze_kernel_handle_t function_b = nullptr;
    void *buffer_i = buffers.at(i);
    ze_command_list_handle_t command_list_a = device_environs[i].command_list;
    ze_command_queue_handle_t command_queue_a =
        device_environs[i].command_queue;
    ze_group_count_t thread_group_dimensions;
    uint32_t group_size_x;
    uint32_t group_size_y;
    uint32_t group_size_z;

    _copy_function_setup(device_environs[i].device_index, function_a,
                         "single_copy_peer_to_peer", number_buffer_elements, 1,
                         1, group_size_x, group_size_y, group_size_z);
    if (bidirectional) {
      _copy_function_setup(device_environs[i].device_index, function_b,
                           "single_copy_peer_to_peer", number_buffer_elements,
                           1, 1, group_size_x, group_size_y, group_size_z);
    }

    const int group_count_x = number_buffer_elements / group_size_x;
    thread_group_dimensions.groupCountX = group_count_x;
    thread_group_dimensions.groupCountY = 1;
    thread_group_dimensions.groupCountZ = 1;

    for (uint32_t j = 0; j < device_environs.size(); j++) {
      long double total_time_usec;
      Timer<std::chrono::microseconds::period> timer;
      void *buffer_j = buffers.at(j);

      if (bidirectional) {
        /* PEER_WRITE */
        SUCCESS_OR_TERMINATE(
            zeKernelSetArgumentValue(function_a, 0, /* Destination buffer*/
                                     sizeof(buffer_j), &buffer_j));
        SUCCESS_OR_TERMINATE(
            zeKernelSetArgumentValue(function_a, 1, /* Source buffer */
                                     sizeof(buffer_i), &buffer_i));
        /* PEER_READ */
        SUCCESS_OR_TERMINATE(
            zeKernelSetArgumentValue(function_b, 0, /* Destination buffer*/
                                     sizeof(buffer_i), &buffer_i));
        SUCCESS_OR_TERMINATE(
            zeKernelSetArgumentValue(function_b, 1, /* Source buffer */
                                     sizeof(buffer_j), &buffer_j));

        zeCommandListAppendLaunchKernel(command_list_a, function_a,
                                        &thread_group_dimensions, nullptr, 0,
                                        nullptr);
        zeCommandListAppendLaunchKernel(command_list_a, function_b,
                                        &thread_group_dimensions, nullptr, 0,
                                        nullptr);
      } else { /* unidirectional */
        if (transfer_type == PEER_WRITE) {
          SUCCESS_OR_TERMINATE(
              zeKernelSetArgumentValue(function_a, 0, /* Destination buffer*/
                                       sizeof(buffer_j), &buffer_j));
          SUCCESS_OR_TERMINATE(
              zeKernelSetArgumentValue(function_a, 1, /* Source buffer */
                                       sizeof(buffer_i), &buffer_i));
        } else if (transfer_type == PEER_READ) {
          SUCCESS_OR_TERMINATE(
              zeKernelSetArgumentValue(function_a, 0, /* Destination buffer*/
                                       sizeof(buffer_i), &buffer_i));
          SUCCESS_OR_TERMINATE(
              zeKernelSetArgumentValue(function_a, 1, /* Source buffer */
                                       sizeof(buffer_j), &buffer_j));
        } else {
          std::cerr
              << "ERROR: Latency test - transfer type parameter is invalid"
              << std::endl;
          std::terminate();
        }

        zeCommandListAppendLaunchKernel(command_list_a, function_a,
                                        &thread_group_dimensions, nullptr, 0,
                                        nullptr);
      }
      benchmark->commandListClose(command_list_a);

      /* Warm up */
      for (int i = 0; i < warm_up_iterations; i++) {
        benchmark->commandQueueExecuteCommandList(command_queue_a, 1,
                                                  &command_list_a);
        benchmark->commandQueueSynchronize(command_queue_a);
      }

      timer.start();
      for (int i = 0; i < number_iterations; i++) {
        benchmark->commandQueueExecuteCommandList(command_queue_a, 1,
                                                  &command_list_a);
        benchmark->commandQueueSynchronize(command_queue_a);
      }
      timer.end();

      total_time_usec = timer.period_minus_overhead() /
                        static_cast<long double>(number_iterations);

      if (bidirectional) {
        std::cout << std::setprecision(11) << std::setw(8) << " Device(" << i
                  << ")<->Device(" << j << "): " << total_time_usec << " uS"
                  << std::endl;
      } else {
        if (transfer_type == PEER_WRITE) {
          std::cout << std::setprecision(11) << std::setw(8) << " Device(" << i
                    << ")->Device(" << j << "): " << total_time_usec << " uS"
                    << std::endl;
        } else {
          std::cout << std::setprecision(11) << std::setw(8) << " Device(" << i
                    << ")<-Device(" << j << "): " << total_time_usec << " uS"
                    << std::endl;
        }
      }
      benchmark->commandListReset(command_list_a);
    }
    _copy_function_cleanup(function_a);
    if (bidirectional) {
      _copy_function_cleanup(function_b);
    }
  }

  for (void *buffer : buffers) {
    benchmark->memoryFree(buffer);
  }
}

void ZePeer::ipc_bandwidth_latency(bool bidirectional, peer_test_t test_type,
                                   peer_transfer_t transfer_type,
                                   bool is_server, int commSocket,
                                   int number_buffer_elements, int device_id) {
  int number_iterations;
  int warm_up_iterations = 5;
  void *zeBuffer = nullptr;
  void *zeIpcBuffer = nullptr;
  size_t element_size = sizeof(char);

  if (test_type == PEER_BANDWIDTH) {
    number_iterations = 5;
  } else if (test_type == PEER_LATENCY) {
    number_iterations = 100;
  } else {
    std::cerr << "ERROR: test type parameter is invalid" << std::endl;
    std::terminate();
  }

  size_t buffer_size = element_size * number_buffer_elements;

  benchmark->memoryAlloc(device_environs[device_id].device_index, buffer_size,
                         &zeBuffer);

  ze_command_list_handle_t command_list =
      device_environs[device_id].command_list;
  ze_command_queue_handle_t command_queue =
      device_environs[device_id].command_queue;

  if (bidirectional) {
    // TODO: Add bidirectional path
  } else { /* unidirectional */
    if (is_server) {
      ze_ipc_mem_handle_t pIpcHandle;
      SUCCESS_OR_TERMINATE(
          zeMemGetIpcHandle(benchmark->context, zeBuffer, &pIpcHandle));

      int dma_buf_fd;
      memcpy(static_cast<void *>(&dma_buf_fd), &pIpcHandle, sizeof(dma_buf_fd));
      if (sendmsg_fd(commSocket, static_cast<int>(dma_buf_fd)) < 0) {
        std::cerr << "Failing to send dma_buf fd to client\n";
        std::terminate();
      }

      // Wait for client to exit
      int child_status;
      pid_t clientPId = wait(&child_status);
      if (clientPId <= 0) {
        std::cerr << "Client terminated abruptly with error code "
                  << strerror(errno) << "\n";
        std::terminate();
      }
    } else {
      int dma_buf_fd = recvmsg_fd(commSocket);
      if (dma_buf_fd < 0) {
        std::cerr << "Failing to get dma_buf fd from server\n";
        std::terminate();
      }
      ze_ipc_mem_handle_t pIpcHandle;
      memcpy(&pIpcHandle, static_cast<void *>(&dma_buf_fd), sizeof(dma_buf_fd));

      benchmark->memoryOpenIpcHandle(device_environs[device_id].device_index,
                                     pIpcHandle, &zeIpcBuffer);

      if (transfer_type == PEER_WRITE) {
        SUCCESS_OR_TERMINATE(
            zeCommandListAppendMemoryCopy(command_list, zeIpcBuffer, zeBuffer,
                                          buffer_size, nullptr, 0, nullptr));
      } else if (transfer_type == PEER_READ) {
        SUCCESS_OR_TERMINATE(
            zeCommandListAppendMemoryCopy(command_list, zeBuffer, zeIpcBuffer,
                                          buffer_size, nullptr, 0, nullptr));
      } else {
        std::cerr
            << "ERROR: Unidirectional test - transfer type parameter is invalid"
            << std::endl;
        std::terminate();
      }

      SUCCESS_OR_TERMINATE(zeCommandListClose(command_list));
    }
  }

  if (is_server == false) {
    long double total_time_usec;
    long double total_time_s;
    long double total_data_transfer;
    long double total_bandwidth;
    Timer<std::chrono::microseconds::period> timer;

    /* Warm up */
    for (int i = 0; i < warm_up_iterations; i++) {
      SUCCESS_OR_TERMINATE(zeCommandQueueExecuteCommandLists(
          command_queue, 1, &command_list, nullptr));
      SUCCESS_OR_TERMINATE(zeCommandQueueSynchronize(
          command_queue, std::numeric_limits<uint64_t>::max()));
    }

    timer.start();
    for (int i = 0; i < number_iterations; i++) {
      SUCCESS_OR_TERMINATE(zeCommandQueueExecuteCommandLists(
          command_queue, 1, &command_list, nullptr));
      SUCCESS_OR_TERMINATE(zeCommandQueueSynchronize(
          command_queue, std::numeric_limits<uint64_t>::max()));
    }
    timer.end();

    SUCCESS_OR_TERMINATE(zeMemCloseIpcHandle(benchmark->context, zeIpcBuffer));

    if (test_type == PEER_BANDWIDTH) {
      total_time_usec = timer.period_minus_overhead();
      total_time_s = total_time_usec / 1e6;

      total_data_transfer =
          (buffer_size * number_iterations) /
          static_cast<long double>(1e9); /* Units in Gigabytes */
      if (bidirectional) {
        total_data_transfer = 2 * total_data_transfer;
      }
      total_bandwidth = total_data_transfer / total_time_s;

      int buffer_size_formatted;
      std::string buffer_format_str;

      if (buffer_size >= (1024 * 1024)) {
        buffer_size_formatted =
            buffer_size / (1024 * 1024); /* Units in Megabytes */
        buffer_format_str = " MB";
      } else if (buffer_size >= 1024) {
        buffer_size_formatted = buffer_size / 1024; /* Units in Kilobytes */
        buffer_format_str = " KB";
      } else {
        buffer_size_formatted = buffer_size; /* Units in Bytes */
        buffer_format_str = " B";
      }

      if (bidirectional) {

      } else {
        std::cout << std::setprecision(8) << std::setw(8)
                  << " Size: " << buffer_size_formatted << buffer_format_str
                  << " - BW: " << total_bandwidth << " GBPS " << std::endl;
      }
    } else {
      total_time_usec = timer.period_minus_overhead() /
                        static_cast<long double>(number_iterations);

      std::cout << std::setprecision(8) << std::setw(8)
                << " Latency: " << total_time_usec << " us " << std::endl;
    }
  }

  benchmark->commandListReset(command_list);
  benchmark->memoryFree(zeBuffer);
  exit(0);
}

void run_ipc_tests(size_t max_number_of_elements, bool run_ipc_bw,
                   bool run_ipc_latency,
                   const uint32_t command_queue_group_ordinal,
                   const uint32_t command_queue_index) {
  pid_t pid;
  int sv[2];
  if (socketpair(PF_UNIX, SOCK_STREAM, 0, sv) < 0) {
    perror("socketpair");
    exit(1);
  }

  if (run_ipc_bw) {
    for (int i = 0; i < 2; i++) {
      for (int j = 0; j < 2; j++) {
        std::cout << "Unidirectional Bandwidth IPC P2P Write: Device(" << i
                  << ")-->Device(" << j << ")" << std::endl;
        for (int number_of_elements = 8;
             number_of_elements <= max_number_of_elements;
             number_of_elements *= 2) {
          pid = fork();
          if (pid == 0) {
            pid_t test_pid = fork();
            if (test_pid == 0) {
              ZePeer peer(command_queue_group_ordinal, command_queue_index);
              peer.ipc_bandwidth_latency(
                  false /* unidirectional */, PEER_BANDWIDTH, PEER_WRITE,
                  false /* client */, sv[1], number_of_elements, i);
            } else {
              ZePeer peer(command_queue_group_ordinal, command_queue_index);
              peer.ipc_bandwidth_latency(
                  false /* unidirectional */, PEER_BANDWIDTH, PEER_WRITE,
                  true /* server */, sv[0], number_of_elements, j);
            }
          } else {
            int child_status;
            pid_t child_pid = wait(&child_status);
            if (child_pid <= 0) {
              std::cerr << "Client terminated abruptly with error code "
                        << strerror(errno) << "\n";
              std::terminate();
            }
          }
        }
        std::cout << std::endl;

        std::cout << "Unidirectional Bandwidth IPC P2P Read: Device(" << i
                  << ")<--Device(" << j << ")" << std::endl;
        for (int number_of_elements = 8;
             number_of_elements <= max_number_of_elements;
             number_of_elements *= 2) {
          pid = fork();
          if (pid == 0) {
            pid_t test_pid = fork();
            if (test_pid == 0) {
              ZePeer peer(command_queue_group_ordinal, command_queue_index);
              peer.ipc_bandwidth_latency(
                  false /* unidirectional */, PEER_BANDWIDTH, PEER_READ,
                  false /* client */, sv[1], number_of_elements, i);
            } else {
              ZePeer peer(command_queue_group_ordinal, command_queue_index);
              peer.ipc_bandwidth_latency(
                  false /* unidirectional */, PEER_BANDWIDTH, PEER_READ,
                  true /* server */, sv[0], number_of_elements, j);
            }
          } else {
            int child_status;
            pid_t child_pid = wait(&child_status);
            if (child_pid <= 0) {
              std::cerr << "Client terminated abruptly with error code "
                        << strerror(errno) << "\n";
              std::terminate();
            }
          }
        }
        std::cout << std::endl;
      }
    }
  }

  if (run_ipc_latency) {
    for (int i = 0; i < 2; i++) {
      for (int j = 0; j < 2; j++) {
        std::cout << "Unidirectional Latency IPC P2P Write: Device(" << i
                  << ")-->Device(" << j << ")" << std::endl;
        pid = fork();
        if (pid == 0) {
          pid_t test_pid = fork();
          if (test_pid == 0) {
            ZePeer peer(command_queue_group_ordinal, command_queue_index);
            peer.ipc_bandwidth_latency(false /* unidirectional */, PEER_LATENCY,
                                       PEER_WRITE, false /* client */, sv[1],
                                       1 /* number of elements */, i);
          } else {
            ZePeer peer(command_queue_group_ordinal, command_queue_index);
            peer.ipc_bandwidth_latency(false /* unidirectional */, PEER_LATENCY,
                                       PEER_WRITE, true /* server */, sv[0],
                                       1 /* number of elements */, j);
          }
        } else {
          int child_status;
          pid_t child_pid = wait(&child_status);
          if (child_pid <= 0) {
            std::cerr << "Client terminated abruptly with error code "
                      << strerror(errno) << "\n";
            std::terminate();
          }
        }
        std::cout << std::endl;

        std::cout << "Unidirectional Latency IPC P2P Read: Device(" << i
                  << ")<--Device(" << j << ")" << std::endl;
        pid = fork();
        if (pid == 0) {
          pid_t test_pid = fork();
          if (test_pid == 0) {
            ZePeer peer(command_queue_group_ordinal, command_queue_index);
            peer.ipc_bandwidth_latency(false /* unidirectional */, PEER_LATENCY,
                                       PEER_READ, false /* client */, sv[1],
                                       1 /* number of elements */, i);
          } else {
            ZePeer peer(command_queue_group_ordinal, command_queue_index);
            peer.ipc_bandwidth_latency(false /* unidirectional */, PEER_LATENCY,
                                       PEER_READ, true /* server */, sv[0],
                                       1 /* number of elements */, j);
          }
        } else {
          int child_status;
          pid_t child_pid = wait(&child_status);
          if (child_pid <= 0) {
            std::cerr << "Client terminated abruptly with error code "
                      << strerror(errno) << "\n";
            std::terminate();
          }
        }
        std::cout << std::endl;
      }
    }
  }

  close(sv[0]);
  close(sv[1]);
}

int main(int argc, char **argv) {
  bool run_ipc_bw = true;
  bool run_ipc_latency = true;
  bool run_transfer_bw = true;
  bool run_latency = true;
  uint32_t command_queue_group_ordinal = 0;
  uint32_t command_queue_index = 0;

  size_t max_number_of_elements = 1024 * 1024 * 256; /* 256 MB */

  for (int i = 1; i < argc; i++) {
    if ((strcmp(argv[i], "-h") == 0) || (strcmp(argv[i], "--help") == 0)) {
      std::cout << usage_str;
      exit(0);
    } else if ((strcmp(argv[i], "-q") == 0)) {
      ZePeer peer(command_queue_group_ordinal, command_queue_index);
      peer.query_engines();
      exit(0);
    } else if ((strcmp(argv[i], "-g") == 0)) {
      if (isdigit(argv[i + 1][0])) {
        command_queue_group_ordinal = atoi(argv[i + 1]);
      } else {
        std::cout << usage_str;
        exit(-1);
      }
      i++;
    } else if ((strcmp(argv[i], "-i") == 0)) {
      if (isdigit(argv[i + 1][0])) {
        command_queue_index = atoi(argv[i + 1]);
      } else {
        std::cout << usage_str;
        exit(-1);
      }
      i++;
    } else if ((strcmp(argv[i], "-t") == 0)) {
      run_ipc_bw = false;
      run_ipc_latency = false;
      run_transfer_bw = false;
      run_latency = false;
      if ((i + 1) >= argc) {
        std::cout << usage_str;
        exit(-1);
      }
      if (strcmp(argv[i + 1], "ipc") == 0) {
        run_ipc_bw = true;
        run_ipc_latency = true;
        i++;
      } else if (strcmp(argv[i + 1], "ipc_bw") == 0) {
        run_ipc_bw = true;
        i++;
      } else if (strcmp(argv[i + 1], "ipc_latency") == 0) {
        run_ipc_latency = true;
        i++;
      } else if (strcmp(argv[i + 1], "transfer_bw") == 0) {
        run_transfer_bw = true;
        i++;
      } else if (strcmp(argv[i + 1], "latency") == 0) {
        run_latency = true;
        i++;
      } else {
        std::cout << usage_str;
        exit(-1);
      }
    } else if (strcmp(argv[i], "-a") == 0) {
      run_ipc_bw = run_ipc_latency = run_transfer_bw = run_latency = true;
    } else {
      std::cout << usage_str;
      exit(-1);
    }
  }

  if (run_ipc_bw || run_ipc_latency) {
    run_ipc_tests(max_number_of_elements, run_ipc_bw, run_ipc_latency,
                  command_queue_group_ordinal, command_queue_index);
  }

  ZePeer peer(command_queue_group_ordinal, command_queue_index);
  if (run_transfer_bw) {
    std::cout << "Unidirectional Bandwidth P2P Write" << std::endl;
    peer.bandwidth(false /* unidirectional */, PEER_WRITE);
    std::cout << std::endl;

    std::cout << "Unidirectional Bandwidth P2P Read" << std::endl;
    peer.bandwidth(false /* unidirectional */, PEER_READ);
    std::cout << std::endl;

    std::cout << "Bidirectional Bandwidth P2P Write" << std::endl;
    peer.bandwidth(true /* bidirectional */, PEER_NONE);
    std::cout << std::endl;
  }

  if (run_latency) {
    std::cout << "Unidirectional Latency P2P Write" << std::endl;
    peer.latency(false /* unidirectional */, PEER_WRITE);
    std::cout << std::endl;

    std::cout << "Unidirectional Latency P2P Read" << std::endl;
    peer.latency(false /* unidirectional */, PEER_READ);
    std::cout << std::endl;

    std::cout << "Bidirectional Latency P2P Write" << std::endl;
    peer.latency(true /* bidirectional */, PEER_NONE);
    std::cout << std::endl;
  }

  std::cout << std::flush;

  return 0;
}
