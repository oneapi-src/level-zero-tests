/*
 *
 * Copyright (C) 2019-2026 Intel Corporation
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
  std::vector<bool> compute_engines;
} ze_peer_device_t;

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
                    void *src_buffer, size_t buffer_size, uint32_t device_id);

  void perform_copy_immediate(peer_test_t test_type,
                              ze_command_list_handle_t command_list,
                              void *dst_buffer, void *src_buffer,
                              size_t buffer_size, uint32_t device_id);

  void append_copy(ze_command_list_handle_t command_list, uint32_t device_id,
                   void *dst_buffer, void *src_buffer, size_t buffer_size,
                   ze_event_handle_t signal_event, uint32_t num_wait_events,
                   ze_event_handle_t *wait_events);

  void create_copy_kernel(uint32_t device_id, size_t buffer_size);

  void set_up_kernel_copy(uint32_t device_id, uint32_t queue_index,
                          size_t buffer_size);

  void create_timestamp_event(uint32_t device_id,
                              ze_event_pool_handle_t &timestamp_pool,
                              ze_event_handle_t &timestamp_event);

  long double timestamp_event_time_us(ze_event_handle_t timestamp_event,
                                      uint32_t device_id);

  void bidirectional_perform_copy(uint32_t dst_device_id,
                                  uint32_t src_device_id, uint32_t queue_index,
                                  peer_test_t test_type,
                                  peer_transfer_t transfer_type,
                                  size_t buffer_size);

  void bidirectional_perform_copy_immediate(
      uint32_t remote_device_id, uint32_t local_device_id, uint32_t queue_index,
      peer_test_t test_type, peer_transfer_t transfer_type, size_t buffer_size);

  void copy_and_wait(ze_command_list_handle_t command_list,
                     ze_command_queue_handle_t command_queue, void *dst,
                     const void *src, size_t size);

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

  void check_sizes(size_t buffer_size,
                   const std::unordered_map<uint32_t, size_t> &device_buffers);

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
  std::vector<ze_module_handle_t> copy_modules;
  std::vector<ze_kernel_handle_t> copy_kernels;
  std::vector<uint32_t> copy_kernel_group_count;
  std::vector<uint32_t> copy_kernel_group_size;
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
  static bool use_event_timer;
  static bool use_kernel;

  // Size of the host buffers that initialize and validate the device
  // buffers, chunk by chunk.
  static constexpr size_t host_chunk_size = 64 * 1024 * 1024;

  static uint32_t number_iterations;
  uint32_t warm_up_iterations = number_iterations / 5;
};
