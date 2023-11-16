/*
 *
 * Copyright (C) 2019-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <chrono>
#include <level_zero/ze_api.h>
#include "ze_app.hpp"

class ZeBandwidth {
public:
  ZeBandwidth();
  ~ZeBandwidth();
  int parse_arguments(int argc, char **argv);
  void test_host2device(void);
  void test_device2host(void);
  void test_bidir(void);
  void ze_bandwidth_query_engines();

  std::vector<size_t> transfer_size;
  std::vector<uint32_t> device_ids{};
  size_t transfer_lower_limit = 1;
  size_t transfer_upper_limit = (1 << 28);
  bool verify = false;
  bool run_host2dev = true;
  bool run_dev2host = true;
  bool run_bidirectional = false;
  bool use_immediate_command_list = false;
  uint32_t number_iterations = 500;
  uint32_t warmup_iterations = 10;
  bool query_engines = false;
  bool enable_fixed_ordinal_index = false;
  uint32_t command_queue_group_ordinal = 0;
  uint32_t command_queue_index = 0;
  uint32_t command_queue_group_ordinal1 = 0;
  uint32_t command_queue_index1 = 0;
  bool csv_output = false;
  ze_event_pool_handle_t event_pool = {};
  ze_event_handle_t wait_event = {};

  std::vector<ze_event_handle_t> event{};
  std::vector<ze_event_handle_t> event1{};

  std::vector<void *> device_buffers;
  std::vector<void *> device_buffers_bidir;
  std::vector<void *> host_buffers;
  std::vector<void *> host_buffers_bidir;

  std::vector<ze_command_queue_handle_t> command_queue{};
  std::vector<ze_command_queue_handle_t> command_queue1{};
  std::vector<ze_command_list_handle_t> command_list{};
  std::vector<ze_command_list_handle_t> command_list1{};

  ZeApp *benchmark;

private:
  void transfer_size_test(size_t size, std::vector<void *> &destination_buffer,
                          std::vector<void *> &source_buffer,
                          std::vector<long double> &device_times_nsec,
                          long double &total_time_nsec);
  void transfer_bidir_size_test(size_t size,
                                std::vector<void *> &destination_buffer,
                                std::vector<void *> &source_buffer,
                                std::vector<void *> &destination_buffer1,
                                std::vector<void *> &source_buffer1,
                                std::vector<long double> &device_times_nsec,
                                long double &total_time_nsec);
  long double measure_transfer();
  void print_results(size_t buffer_size, long double total_bandwidth,
                     long double total_latency, std::string direction_string);
  void calculate_metrics(long double total_time_nsec, /* Units in nanoseconds */
                         long double total_data_transfer, /* Units in bytes */
                         long double &total_bandwidth,
                         long double &total_latency);

  ze_command_queue_handle_t command_queue_verify{};
  ze_command_list_handle_t command_list_verify{};
  std::vector<ze_command_queue_group_properties_t> queueProperties;

  void *host_buffer_verify;
  void *host_buffer_verify1;
};
