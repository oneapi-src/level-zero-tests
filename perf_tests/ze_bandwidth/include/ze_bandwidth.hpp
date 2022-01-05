/*
 *
 * Copyright (C) 2019 Intel Corporation
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
  void ze_bandwidth_query_engines();

  std::vector<size_t> transfer_size;
  size_t transfer_lower_limit = 1;
  size_t transfer_upper_limit = (1 << 28);
  bool verify = false;
  bool run_host2dev = true;
  bool run_dev2host = true;
  uint32_t number_iterations = 500;
  bool query_engines = false;
  bool enable_fixed_ordinal_index = false;
  uint32_t command_queue_group_ordinal = 0;
  uint32_t command_queue_index = 0;
  bool csv_output = false;

private:
  void transfer_size_test(size_t size, void *destination_buffer,
                          void *source_buffer, long double &total_time_nsec);
  void transfer_size_test_verify(size_t size, long double &host2dev_time_nsec,
                                 long double &dev2host_time_nsec);
  long double measure_transfer(uint32_t num_transfer);
  void measure_transfer_verify(size_t buffer_size, uint32_t num_transfer,
                               long double &host2dev_time_nsec,
                               long double &dev2host_time_nsec);
  void print_results_host2device(size_t buffer_size,
                                 long double total_bandwidth,
                                 long double total_latency);
  void print_results_device2host(size_t buffer_size,
                                 long double total_bandwidth,
                                 long double total_latency);
  void calculate_metrics(long double total_time_nsec, /* Units in nanoseconds */
                         long double total_data_transfer, /* Units in bytes */
                         long double &total_bandwidth,
                         long double &total_latency);
  ZeApp *benchmark;
  ze_command_queue_handle_t command_queue;
  ze_command_list_handle_t command_list;
  ze_command_list_handle_t command_list_verify;
  std::vector<ze_command_queue_group_properties_t> queueProperties;

  void *device_buffer;
  void *host_buffer;
  void *host_buffer_verify;
};
