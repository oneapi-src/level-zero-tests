/*
 *
 * Copyright (C) 2019-2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */
#pragma once

#include <iomanip>
#include <assert.h>
#include <ctime>
#include <iostream>
#include <math.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
#include <algorithm>
#include "common.hpp"
#include "ze_app.hpp"
#include <level_zero/ze_api.h>

typedef enum _peer_transfer_t {
  PEER_WRITE = 0,
  PEER_READ,
  PEER_TRANSFER_MAX
} peer_transfer_t;
typedef enum _peer_test_t {
  PEER_BANDWIDTH = 0,
  PEER_LATENCY,
  PEER_TEST_MAX
} peer_test_t;

typedef struct _ze_peer_device_t {
  std::vector<ze_command_list_handle_t> command_lists;
  std::vector<ze_command_queue_handle_t> command_queues;
} ze_peer_device_t;

static const char *usage_str =
    "\n ze_peer [OPTIONS]"
    "\n"
    "\n OPTIONS:"
    "\n  -t, string                  selectively run a particular test"
    "\n      transfer_bw             selectively run transfer bandwidth test"
    "\n      latency                 selectively run latency test"
    "\n  -a                          run all above tests [default]"
    "\n  -b                          run birectional mode"
    "\n  -o, string                  operation to perform"
    "\n      read                    read from remote"
    "\n      write                   write to remote"
    "\n  -m                          run tests in multiprocess"
    "\n  -d                          destination device"
    "\n  -s                          source device"
    "\n  -z                          size to run"
    "\n  -v                          validate data (only 1 iteration is "
    "execute)"
    "\n  -q                          query for number of engines available"
    "\n  -g, number                  select engine group (default: 0)"
    "\n  -i, number                  select engine index (default: 0)"
    "\n  -e                          run concurrently using all compute "
    "engines (each size is evenly distributed among engines) "
    "\n  -l                          run concurrently using all copy engines "
    "(each size is evenly distributed among engines) "
    "\n  -h, --help                  display help message"
    "\n";

class ZePeer {
public:
  ZePeer(const uint32_t command_queue_group_ordinal,
         const uint32_t command_queue_index, uint32_t dst_device_id,
         uint32_t src_device_id, bool run_using_all_compute_engines,
         bool run_using_all_copy_engines, uint32_t *num_devices = 0) {

    benchmark = new ZeApp();

    this->run_using_all_compute_engines = run_using_all_compute_engines;
    this->run_using_all_copy_engines = run_using_all_copy_engines;

    uint32_t device_count = benchmark->allDevicesInit();
    *num_devices = device_count;

    if (!benchmark->canAccessPeer(dst_device_id, src_device_id)) {
      std::cerr << "Devices " << src_device_id << " and " << dst_device_id
                << " do not have P2P capabilities " << std::endl;
      std::terminate();
    }

    if (benchmark->_devices.size() <= 1) {
      std::cerr << "ERROR: there are no peer devices among " << device_count
                << " devices found" << std::endl;
      std::terminate();
    }

    if (benchmark->_devices.size() <
        std::max(dst_device_id + 1, src_device_id + 1)) {
      std::cout << "ERROR: Number for source or destination device not "
                << "available: Only " << benchmark->_devices.size()
                << " devices "
                << "detected" << std::endl;
      std::terminate();
    }

    ze_peer_devices.resize(benchmark->_devices.size());

    ze_buffers.resize(benchmark->_devices.size());
    ze_src_buffers.resize(benchmark->_devices.size());
    ze_dst_buffers.resize(benchmark->_devices.size());
    ze_ipc_buffers.resize(benchmark->_devices.size());

    for (uint32_t d = 0; d < benchmark->_devices.size(); d++) {
      if (run_using_all_compute_engines == false &&
          run_using_all_copy_engines == false) {
        ze_command_queue_handle_t command_queue;
        benchmark->commandQueueCreate(d, command_queue_group_ordinal,
                                      command_queue_index, &command_queue);
        ze_peer_devices[d].command_queues.push_back(command_queue);

        ze_command_list_handle_t command_list;
        benchmark->commandListCreate(d, command_queue_group_ordinal,
                                     &command_list);
        ze_peer_devices[d].command_lists.push_back(command_list);
      } else {
        uint32_t numQueueGroups = 0;
        benchmark->deviceGetCommandQueueGroupProperties(d, &numQueueGroups,
                                                        nullptr);

        if (numQueueGroups == 0) {
          std::cout << " No queue groups found\n" << std::endl;
          std::terminate();
        }

        std::vector<ze_command_queue_group_properties_t> queueProperties(
            numQueueGroups);
        benchmark->deviceGetCommandQueueGroupProperties(d, &numQueueGroups,
                                                        queueProperties.data());

        uint32_t queue_ordinal = std::numeric_limits<uint32_t>::max();
        for (uint32_t g = 0; g < numQueueGroups; g++) {
          if (run_using_all_compute_engines &&
              queueProperties[g].flags &
                  ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE) {
            queue_ordinal = g;
            break;
          }
          if (run_using_all_copy_engines &&
              (queueProperties[g].flags &
               ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE) == 0 &&
              (queueProperties[g].flags &
               ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COPY) &&
              queueProperties[g].numQueues > 1) {
            queue_ordinal = g;
            break;
          }
        }

        if (queue_ordinal == std::numeric_limits<uint32_t>::max()) {
          std::cout << " No suitable engine found\n" << std::endl;
          std::terminate();
        }

        for (uint32_t q = 0; q < queueProperties[queue_ordinal].numQueues;
             q++) {
          ze_command_queue_handle_t command_queue;
          benchmark->commandQueueCreate(d, queue_ordinal, q, &command_queue);
          ze_peer_devices[d].command_queues.push_back(command_queue);

          ze_command_list_handle_t command_list;
          benchmark->commandListCreate(d, queue_ordinal, &command_list);
          ze_peer_devices[d].command_lists.push_back(command_list);
        }
      }
    }
  }

  ~ZePeer() {
    for (auto &device : ze_peer_devices) {
      benchmark->commandQueueDestroy(device.command_queues[0]);
      benchmark->commandListDestroy(device.command_lists[0]);
    }
    benchmark->allDevicesCleanup();
    delete benchmark;
  }

  void bandwidth_latency_all_engines(peer_test_t test_type,
                                     peer_transfer_t transfer_type,
                                     int number_buffer_elements,
                                     uint32_t remote_device_id,
                                     uint32_t local_device_id, bool validate);

  void bandwidth_latency(peer_test_t test_type, peer_transfer_t transfer_type,
                         int number_buffer_elements, uint32_t remote_device_id,
                         uint32_t local_device_id, bool validate);

  void bidirectional_bandwidth_latency(peer_test_t test_type,
                                       peer_transfer_t transfer_type,
                                       int number_buffer_elements,
                                       uint32_t dst_device_id,
                                       uint32_t src_device_id, bool validate);

  void bandwidth_latency_ipc(bool bidirectional, peer_test_t test_type,
                             peer_transfer_t transfer_type, bool is_server,
                             int commSocket, int number_buffer_elements,
                             uint32_t device_id, uint32_t remote_device_id,
                             bool validate);
  void query_engines();
  void perform_copy_all_engines(peer_test_t test_type, uint32_t local_device_id,
                                void *dst_buffer, void *src_buffer,
                                size_t buffer_size);
  void perform_copy(peer_test_t test_type,
                    ze_command_list_handle_t command_list,
                    ze_command_queue_handle_t command_queue, void *dst_buffer,
                    void *src_buffer, size_t buffer_size);
  void bidirectional_perform_copy(uint32_t dst_device_id,
                                  uint32_t src_device_id, peer_test_t test_type,
                                  peer_transfer_t transfer_type,
                                  size_t buffer_size);
  void initialize_src_buffer(ze_command_list_handle_t command_list,
                             ze_command_queue_handle_t command_queue,
                             void *local_buffer, char *host_buffer,
                             size_t buffer_size);
  void initialize_buffers(ze_command_list_handle_t command_list,
                          ze_command_queue_handle_t command_queue,
                          void *src_buffer, char *host_buffer,
                          size_t buffer_size);
  void initialize_buffers(uint32_t dst_device_id, uint32_t src_device_id,
                          char *host_buffer, size_t buffer_size);
  void validate_buffer(ze_command_list_handle_t command_list,
                       ze_command_queue_handle_t command_queue,
                       char *validate_buffer, void *dst_buffer,
                       char *host_buffer, size_t buffer_size);
  void set_up_ipc(int number_buffer_elements, uint32_t device_id,
                  size_t &buffer_size, ze_command_queue_handle_t &command_queue,
                  ze_command_list_handle_t &command_list, bool validate);
  void set_up(int number_buffer_elements, uint32_t remote_device_id,
              uint32_t local_device_id, size_t &buffer_size, bool validate);

  void tear_down(uint32_t dst_device_id, uint32_t src_device_id);
  void print_results(bool bidirectional, peer_test_t test_type,
                     size_t buffer_size,
                     Timer<std::chrono::microseconds::period> &timer);
  int sendmsg_fd(int socket, int fd);
  int recvmsg_fd(int socket);

  ZeApp *benchmark;
  int number_iterations = 5;
  int warm_up_iterations = 1;
  std::vector<void *> ze_buffers;
  std::vector<void *> ze_src_buffers;
  std::vector<void *> ze_dst_buffers;
  std::vector<void *> ze_ipc_buffers;

  char *ze_host_buffer;
  char *ze_host_validate_buffer;

  std::vector<ze_peer_device_t> ze_peer_devices;
  ze_ipc_mem_handle_t pIpcHandle = {};

  bool run_using_all_compute_engines = false;
  bool run_using_all_copy_engines = false;
};
