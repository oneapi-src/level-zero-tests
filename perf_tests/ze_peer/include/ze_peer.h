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
#include <utility>
#include "common.hpp"
#include "ze_app.hpp"
#include <unistd.h>
#include <iostream>
#include <cstdlib>
#include <signal.h>
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
  std::vector<std::pair<ze_command_queue_handle_t, ze_command_list_handle_t>>
      engines;
} ze_peer_device_t;

static const char *usage_str =
    "\nze_peer: Level Zero microbenchmark to analyze the P2P performance\n"
    "of a multi-GPU system.\n"
    "\n"
    "To execute: ze_peer [OPTIONS]\n"
    "\n"
    "By default, unidirectional transfer bandwidth and latency tests \n"
    "are executed, for sizes between 8 B and 256 MB, for write and read \n"
    "operations, between all devices detected in the system, using the \n"
    "first compute engine in the device.\n"
    "\n"
    "\n OPTIONS:"
    "\n  -b                          run bidirectional mode. Default: Not set."
    "\n  -c                          run continuously until hitting CTRL+C. "
    "Default: Not set."
    "\n  -i                          number of iterations to run. Default: 50."
    "\n  -z                          size to run in bytes. Default: 8192(8MB) "
    "to 268435456(256MB)."
    "\n  -v                          validate data (only 1 iteration is "
    "executed). Default: Not set."
    "\n  -t                          type of transfer to measure"
    "\n      transfer_bw             run transfer bandwidth test"
    "\n      latency                 run latency test"
    "\n  -o                          operation to perform"
    "\n      read                    read from remote"
    "\n      write                   write to remote"
    "\n"
    "\n Engine selection:\n"
    "\n  -q                          query for number of engines available"
    "\n  -u                          engine to use (use -q option to see "
    "available values)."
    "\n                              Accepts a comma-separated list of engines "
    "when running "
    "\n                              parallel tests with multiple targets."
    "\n Device selection:\n"
    "\n  -d                          comma separated list of destination "
    "devices"
    "\n  -s                          comma separated list of source devices"
    "\n"
    "\n Tests:"
    "\n  --parallel_single_target    Divide the buffer into the number of "
    "engines passed "
    "\n                              with option -u, and perform parallel "
    "copies using those "
    "\n                              engines from the source passed with "
    "option -s to a single "
    "\n                              target specified with option -d."
    "\n                              By default, copy is made from device 0 to "
    "device 1 using "
    "\n                              all available engines with compute "
    "capability."
    "\n                              Extra options: -x"
    "\n"
    "\n  --parallel_multiple_targets  Perform parallel copies from the source "
    "passed with option -s "
    "\n                              to the targets specified with option -d, "
    "each one using a "
    "\n                              separate engine specified with option -u."
    "\n                              By default, copy is made from device 0 to "
    "all other devices, "
    "\n                              using all available engines with compute "
    "capability."
    "\n                              Extra options: -x"
    "\n  -x                          for unidirectional parallel tests, select "
    "where to place the queue"
    "\n      src                     use queue in source"
    "\n      dst                     use queue in source"
    "\n"
    "\n  --ipc                       perform a copy between two devices, "
    "specified by options -s and -d, "
    "\n                              with each device being managed by a "
    "separate process."
    "\n"
    "\n  --version                   display version"
    "\n  -h, --help                  display help message"
    "\n";

class ZePeer {
public:
  ZePeer(uint32_t *num_devices);
  ZePeer(std::vector<uint32_t> &remote_device_ids,
         std::vector<uint32_t> &local_device_ids,
         std::vector<uint32_t> &queues);
  ~ZePeer();

  void perform_parallel_copy_to_single_target(peer_test_t test_type,
                                              peer_transfer_t transfer_type,
                                              uint32_t remote_device_id,
                                              uint32_t local_device_id,
                                              size_t buffer_size);

  void perform_bidirectional_parallel_copy_to_single_target(
      peer_test_t test_type, peer_transfer_t transfer_type,
      uint32_t remote_device_id, uint32_t local_device_id, size_t buffer_size);

  void bandwidth_latency_parallel_to_single_target(
      peer_test_t test_type, peer_transfer_t transfer_type,
      int number_buffer_elements, uint32_t remote_device_id,
      uint32_t local_device_id);

  void perform_parallel_copy_to_multiple_targets(
      peer_test_t test_type, peer_transfer_t transfer_type,
      std::vector<uint32_t> &remote_device_ids,
      std::vector<uint32_t> &local_device_ids, size_t buffer_size);

  void perform_bidirectional_parallel_copy_to_multiple_targets(
      peer_test_t test_type, peer_transfer_t transfer_type,
      std::vector<uint32_t> &remote_device_ids,
      std::vector<uint32_t> &local_device_ids, size_t buffer_size);

  void bandwidth_latency_parallel_to_multiple_targets(
      peer_test_t test_type, peer_transfer_t transfer_type,
      int number_buffer_elements, std::vector<uint32_t> &remote_device_ids,
      std::vector<uint32_t> &local_device_ids);

  void bandwidth_latency(peer_test_t test_type, peer_transfer_t transfer_type,
                         int number_buffer_elements, uint32_t remote_device_id,
                         uint32_t local_device_id, uint32_t queue_index);

  void bidirectional_bandwidth_latency(peer_test_t test_type,
                                       peer_transfer_t transfer_type,
                                       int number_buffer_elements,
                                       uint32_t dst_device_id,
                                       uint32_t src_device_id,
                                       uint32_t queue_index);

  void perform_copy(peer_test_t test_type,
                    ze_command_list_handle_t command_list,
                    ze_command_queue_handle_t command_queue, void *dst_buffer,
                    void *src_buffer, size_t buffer_size);

  void bidirectional_perform_copy(uint32_t dst_device_id,
                                  uint32_t src_device_id, uint32_t queue_index,
                                  peer_test_t test_type,
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

  void initialize_buffers(std::vector<uint32_t> &remote_device_ids,
                          std::vector<uint32_t> &local_device_ids,
                          char *host_buffer, size_t buffer_size);

  void validate_buffer(ze_command_list_handle_t command_list,
                       ze_command_queue_handle_t command_queue,
                       char *validate_buffer, void *dst_buffer,
                       char *host_buffer, size_t buffer_size);

  void set_up(int number_buffer_elements,
              std::vector<uint32_t> &remote_device_ids,
              std::vector<uint32_t> &local_device_ids, size_t &buffer_size);

  void tear_down(std::vector<uint32_t> &dst_device_ids,
                 std::vector<uint32_t> &src_device_ids);

  void print_results(bool bidirectional, peer_test_t test_type,
                     size_t buffer_size,
                     Timer<std::chrono::microseconds::period> &timer);

  void print_results(bool bidirectional, peer_test_t test_type,
                     size_t buffer_size, long double total_time_us);

  void query_engines();

  // IPC
  void bandwidth_latency_ipc(peer_test_t test_type,
                             peer_transfer_t transfer_type, bool is_server,
                             int commSocket, int number_buffer_elements,
                             uint32_t device_id, uint32_t remote_device_id);

  void set_up_ipc(int number_buffer_elements, uint32_t device_id,
                  size_t &buffer_size, ze_command_queue_handle_t &command_queue,
                  ze_command_list_handle_t &command_list);

  int sendmsg_fd(int socket, int fd);
  int recvmsg_fd(int socket);

  ZeApp *benchmark;

  std::vector<void *> ze_buffers;
  std::vector<void *> ze_ipc_buffers;

  std::vector<void *> ze_src_buffers;
  std::vector<void *> ze_dst_buffers;

  std::vector<uint32_t> queues{};

  char *ze_host_buffer;
  char *ze_host_validate_buffer;

  std::vector<ze_peer_device_t> ze_peer_devices;
  ze_ipc_mem_handle_t pIpcHandle = {};

  uint32_t device_count = 0;

  ze_event_pool_handle_t event_pool = {};
  ze_event_handle_t event = {};

  static bool use_queue_in_destination;
  static bool run_continuously;
  static bool bidirectional;
  static bool validate_results;
  static bool parallel_copy_to_single_target;
  static bool parallel_copy_to_multiple_targets;

  static uint32_t number_iterations;
  uint32_t warm_up_iterations = number_iterations / 5;
};
