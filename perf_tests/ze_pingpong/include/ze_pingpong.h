/*
 *
 * Copyright (c) 2020 Intel Corporation
 * Copyright (C) 2019 Elias
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef ZE_PINGPONG_H
#define ZE_PINGPONG_H

#include <chrono>
#include <cstdint>
#include <ctype.h>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <math.h>
#include <numeric>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>

/* ze includes */
#include <level_zero/ze_api.h>

enum TestType {
  DEVICE_MEM_KERNEL_ONLY,
  DEVICE_MEM_XFER,
  HOST_MEM_KERNEL_ONLY,
  HOST_MEM_NO_XFER,
  SHARED_MEM_KERNEL_ONLY,
  SHARED_MEM_MAP
};

struct L0Context {
  ze_command_queue_handle_t command_queue = nullptr;
  ze_command_list_handle_t command_list = nullptr;
  ze_module_handle_t module = nullptr;
  ze_context_handle_t context = nullptr;
  ze_driver_handle_t driver = nullptr;
  ze_device_handle_t device = nullptr;
  ze_kernel_handle_t function = nullptr;
  ze_group_count_t thread_group_dimensions = {1, 1, 1};
  void *device_input = nullptr;
  void *host_output = nullptr;
  void *shared_output = nullptr;
  uint32_t device_count = 0;
  const uint32_t default_device = 0;
  const uint32_t command_queue_id = 0;
  ze_device_properties_t device_property = {};
  ze_device_compute_properties_t device_compute_property = {};
  void init();
  void destroy();
  void print_ze_device_properties(const ze_device_properties_t &props);
  std::vector<uint8_t> load_binary_file(const std::string &file_path);
};

class ZePingPong {
public:
  int num_execute = 20000;
  /* Helper Functions */
  void create_module(L0Context &context, std::vector<uint8_t> binary_file,
                     ze_module_format_t format, const char *build_flag);
  void set_argument_value(L0Context &context, uint32_t argIndex, size_t argSize,
                          const void *pArgValue);
  void setup_commandlist(L0Context &context, enum TestType test);
  void run_test(L0Context &context);
  void run_command_queue(L0Context &context);
  double measure_benchmark(L0Context &context, enum TestType test);
  void reset_commandlist(L0Context &context);
  void synchronize_command_queue(L0Context &context);
  void verify_result(int result);
};

#endif /* ZE_PINGPONG_H */
