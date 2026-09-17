/*
 *
 * Copyright (C) 2019-2023 Intel Corporation
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
#include <unordered_map>
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

// clang-format off
static const char *usage_str = R"HELP(
ze_peer: Level Zero microbenchmark to analyze the P2P performance
of a multi-GPU system.

To execute: ze_peer [OPTIONS]

By default, unidirectional transfer bandwidth and latency tests 
are executed, for sizes between 8 B and 256 MB, for write and read 
operations, between all devices detected in the system, using the 
first compute engine in the device.


 OPTIONS:
  -b                          run bidirectional mode. Default: Not set.
  -c                          run continuously until hitting CTRL+C. Default: Not set.
  -i                          number of iterations to run. Default: 50.
  -z                          size to run in bytes. Default: 8192(8MB) to 268435456(256MB).
  -v                          validate data (only 1 iteration is executed). Default: Not set.
  -t                          type of transfer to measure
      transfer_bw             run transfer bandwidth test
      latency                 run latency test
  -o                          operation to perform
      read                    read from remote
      write                   write to remote

 Engine selection:

  -q                          query for number of engines available
  -u                          engine to use (use -q option to see available values).
                              Accepts a comma-separated list of engines when running 
                              parallel tests with multiple targets.
 Device selection:

  -d                          comma separated list of destination devices
  -s                          comma separated list of source devices

 Tests:
  --parallel_single_target    Divide the buffer into the number of engines passed 
                              with option -u, and perform parallel copies using those 
                              engines from the source passed with option -s to a single 
                              target specified with option -d.
                              By default, copy is made from device 0 to device 1 using 
                              all available engines with compute capability.
                              Extra options: -x

  --parallel_multiple_targets Perform parallel copies from the source passed with option -s 
                              to the targets specified with option -d, each one using a 
                              separate engine specified with option -u.
                              By default, copy is made from device 0 to all other devices, 
                              using all available engines with compute capability.
                              Extra options: -x, --divide_buffers

  --parallel_pair_targets     (Experimental) Perform parallel copies from pairs of source 
                              and targets each one using a separate engine specified with 
                              option -u. Accepts a comma-separated list of pair devices, 
                              with source and destination pairs defined with a colon.
                              Example pair list: src1:dst1,src2:dst2 
                              Extra options: -x, --divide_buffers

  --divide_buffers            for parallel multiple targets test, divide buffers across available
                              engines specified with option -u.

  -x                          for unidirectional parallel tests, select where to place the queue
      src                     use queue in source
      dst                     use queue in source

  --ipc                       perform a copy between two devices, specified by options -s and -d, 
                              with each device being managed by a separate process.

  --regular_cmdlist           use regular command list instead of immediate

  --version                   display version
  -h, --help                  display help message
)HELP";
// clang-format on

class ZePeer {
public:
  ZePeer(uint32_t *num_devices);
  ZePeer(std::vector<uint32_t> &remote_device_ids,
         std::vector<uint32_t> &local_device_ids,
         std::vector<std::pair<uint32_t, uint32_t>> &pair_device_ids,
         std::vector<uint32_t> &queues);
  ~ZePeer();

  void perform_parallel_copy_to_single_target(peer_test_t test_type,
                                              peer_transfer_t transfer_type,
                                              uint32_t remote_device_id,
                                              uint32_t local_device_id,
                                              size_t buffer_size);

  void perform_parallel_copy_to_single_target_immediate(
      peer_test_t test_type, peer_transfer_t transfer_type,
      uint32_t remote_device_id, uint32_t local_device_id, size_t buffer_size);

  void perform_bidirectional_parallel_copy_to_single_target(
      peer_test_t test_type, peer_transfer_t transfer_type,
      uint32_t remote_device_id, uint32_t local_device_id, size_t buffer_size);

  void perform_bidirectional_parallel_copy_to_single_target_immediate(
      peer_test_t test_type, peer_transfer_t transfer_type,
      uint32_t remote_device_id, uint32_t local_device_id, size_t buffer_size);

  void bandwidth_latency_parallel_to_single_target(
      peer_test_t test_type, peer_transfer_t transfer_type,
      size_t number_buffer_elements, uint32_t remote_device_id,
      uint32_t local_device_id);

  void perform_parallel_copy_to_multiple_targets(
      peer_test_t test_type, peer_transfer_t transfer_type,
      std::vector<uint32_t> &remote_device_ids,
      std::vector<uint32_t> &local_device_ids, size_t buffer_size,
      bool divide_buffers);

  void perform_parallel_copy_to_multiple_targets_immediate(
      peer_test_t test_type, peer_transfer_t transfer_type,
      std::vector<uint32_t> &remote_device_ids,
      std::vector<uint32_t> &local_device_ids, size_t buffer_size,
      bool divide_buffers);

  void perform_bidirectional_parallel_copy_to_multiple_targets(
      peer_test_t test_type, peer_transfer_t transfer_type,
      std::vector<uint32_t> &remote_device_ids,
      std::vector<uint32_t> &local_device_ids, size_t buffer_size,
      bool divide_buffers);

  void perform_bidirectional_parallel_copy_to_multiple_targets_immediate(
      peer_test_t test_type, peer_transfer_t transfer_type,
      std::vector<uint32_t> &remote_device_ids,
      std::vector<uint32_t> &local_device_ids, size_t buffer_size,
      bool divide_buffers);

  void perform_parallel_copy_to_pair_targets(
      peer_test_t test_type, peer_transfer_t transfer_type,
      std::vector<std::pair<uint32_t, uint32_t>> &pair_device_ids,
      size_t buffer_size, bool divide_buffers);

  void perform_parallel_copy_to_pair_targets_immediate(
      peer_test_t test_type, peer_transfer_t transfer_type,
      std::vector<std::pair<uint32_t, uint32_t>> &pair_device_ids,
      size_t buffer_size, bool divide_buffers);

  void perform_bidirectional_parallel_copy_to_pair_targets(
      peer_test_t test_type, peer_transfer_t transfer_type,
      std::vector<std::pair<uint32_t, uint32_t>> &pair_device_ids,
      size_t buffer_size, bool divide_buffers);

  void perform_bidirectional_parallel_copy_to_pair_targets_immediate(
      peer_test_t test_type, peer_transfer_t transfer_type,
      std::vector<std::pair<uint32_t, uint32_t>> &pair_device_ids,
      size_t buffer_size, bool divide_buffers);

  void bandwidth_latency_parallel_to_pair_targets(
      peer_test_t test_type, peer_transfer_t transfer_type,
      size_t number_buffer_elements,
      std::vector<std::pair<uint32_t, uint32_t>> &pair_device_ids,
      bool divide_buffers);

  void bandwidth_latency_parallel_to_multiple_targets(
      peer_test_t test_type, peer_transfer_t transfer_type,
      size_t number_buffer_elements, std::vector<uint32_t> &remote_device_ids,
      std::vector<uint32_t> &local_device_ids, bool divide_buffers);

  void bandwidth_latency(peer_test_t test_type, peer_transfer_t transfer_type,
                         size_t number_buffer_elements,
                         uint32_t remote_device_id, uint32_t local_device_id,
                         uint32_t queue_index);

  void bidirectional_bandwidth_latency(peer_test_t test_type,
                                       peer_transfer_t transfer_type,
                                       size_t number_buffer_elements,
                                       uint32_t dst_device_id,
                                       uint32_t src_device_id,
                                       uint32_t queue_index);

  void perform_copy(peer_test_t test_type,
                    ze_command_list_handle_t command_list,
                    ze_command_queue_handle_t command_queue, void *dst_buffer,
                    void *src_buffer, size_t buffer_size);

  void perform_copy_immediate(peer_test_t test_type,
                              ze_command_list_handle_t command_list,
                              void *dst_buffer, void *src_buffer,
                              size_t buffer_size);

  void bidirectional_perform_copy(uint32_t dst_device_id,
                                  uint32_t src_device_id, uint32_t queue_index,
                                  peer_test_t test_type,
                                  peer_transfer_t transfer_type,
                                  size_t buffer_size);

  void bidirectional_perform_copy_immediate(
      uint32_t remote_device_id, uint32_t local_device_id, uint32_t queue_index,
      peer_test_t test_type, peer_transfer_t transfer_type, size_t buffer_size);

  void initialize_src_buffer(ze_command_list_handle_t command_list,
                             ze_command_queue_handle_t command_queue,
                             void *local_buffer, char *host_buffer,
                             size_t buffer_size);

  void initialize_src_buffer_immediate(ze_command_list_handle_t command_list,
                                       void *src_buffer, char *host_buffer,
                                       size_t buffer_size);

  void initialize_buffers(ze_command_list_handle_t command_list,
                          ze_command_queue_handle_t command_queue,
                          void *src_buffer, char *host_buffer,
                          size_t buffer_size);

  void initialize_buffers(std::vector<uint32_t> &remote_device_ids,
                          std::vector<uint32_t> &local_device_ids,
                          char *host_buffer, size_t buffer_size);

  bool validate_buffer(ze_command_list_handle_t command_list,
                       ze_command_queue_handle_t command_queue,
                       char *validate_buffer, void *dst_buffer,
                       char *host_buffer, size_t buffer_size);

  bool validate_buffer_immediate(ze_command_list_handle_t command_list,
                                 char *validate_buffer, void *dst_buffer,
                                 char *host_buffer, size_t buffer_size);

  void set_up(size_t number_buffer_elements,
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
                             int commSocket, size_t number_buffer_elements,
                             uint32_t device_id, uint32_t remote_device_id);

  void set_up_ipc(size_t number_buffer_elements, uint32_t device_id,
                  size_t &buffer_size, ze_command_queue_handle_t &command_queue,
                  ze_command_list_handle_t &command_list);

  int sendmsg_fd(int socket, int fd);
  int recvmsg_fd(int socket);

  ze_result_t try_copy(ze_command_list_handle_t command_list,
                       ze_command_queue_handle_t command_queue,
                       void *dst_buffer, void *src_buffer, size_t buffer_size);

  ZeApp *benchmark;

  std::vector<void *> ze_buffers;
  std::vector<void *> ze_ipc_buffers;

  std::vector<void *> ze_src_buffers;
  std::vector<void *> ze_dst_buffers;

  std::vector<uint32_t> queues{};

  char *ze_host_buffer = nullptr;
  char *ze_host_validate_buffer = nullptr;

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
  static bool parallel_copy_to_pair_targets;
  static bool parallel_divide_buffers;
  static bool use_immediate_cmdlist;

  static uint32_t number_iterations;
  uint32_t warm_up_iterations = number_iterations / 5;
};
