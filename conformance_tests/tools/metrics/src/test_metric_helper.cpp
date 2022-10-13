/*
 *
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"

namespace lzt = level_zero_tests;

ze_kernel_handle_t load_gpu(ze_device_handle_t device, ze_group_count_t *tg,
                            void **a_buffer, void **b_buffer, void **c_buffer) {

  auto device_properties = lzt::get_device_properties(device);

  auto max_threads =
      device_properties.numSlices * device_properties.numSubslicesPerSlice *
      device_properties.numEUsPerSubslice * device_properties.numThreadsPerEU;

  LOG_INFO << "Available threads: " << max_threads;

  int m, k, n;
  m = k = n = (max_threads > 4096 ? 8192 : 256);
  std::vector<float> a(m * k, 1);
  std::vector<float> b(k * n, 1);
  std::vector<float> c(m * n, 0);
  *a_buffer = lzt::allocate_host_memory(m * k * sizeof(float));
  *b_buffer = lzt::allocate_host_memory(k * n * sizeof(float));
  *c_buffer = lzt::allocate_host_memory(m * n * sizeof(float));

  std::memcpy(*a_buffer, a.data(), a.size() * sizeof(float));
  std::memcpy(*b_buffer, b.data(), b.size() * sizeof(float));

  int group_count_x = m / 16;
  int group_count_y = n / 16;

  tg->groupCountX = group_count_x;
  tg->groupCountY = group_count_y;
  tg->groupCountZ = 1;

  ze_module_handle_t module =
      lzt::create_module(device, "ze_matrix_multiplication.spv",
                         ZE_MODULE_FORMAT_IL_SPIRV, nullptr, nullptr);
  ze_kernel_handle_t function =
      lzt::create_function(module, "matrix_multiplication");
  lzt::set_group_size(function, 16, 16, 1);
  lzt::set_argument_value(function, 0, sizeof(*a_buffer), a_buffer);
  lzt::set_argument_value(function, 1, sizeof(*b_buffer), b_buffer);
  lzt::set_argument_value(function, 2, sizeof(m), &m);
  lzt::set_argument_value(function, 3, sizeof(k), &k);
  lzt::set_argument_value(function, 4, sizeof(n), &n);
  lzt::set_argument_value(function, 5, sizeof(*c_buffer), c_buffer);
  return function;
}

int main(int argc, char **argv) {

  ze_result_t result = zeInit(0);
  if (result != ZE_RESULT_SUCCESS) {
    std::cout << "[Application] zeInit failed";

    exit(1);
  }

  auto driver = lzt::get_default_driver();
  ze_device_handle_t device = nullptr;

  device = lzt::get_default_device(driver);

  ze_command_queue_handle_t commandQueue = lzt::create_command_queue(device);
  zet_command_list_handle_t commandList = lzt::create_command_list(device);

  ze_group_count_t tg;
  void *a_buffer, *b_buffer, *c_buffer;
  ze_kernel_handle_t function =
      load_gpu(device, &tg, &a_buffer, &b_buffer, &c_buffer);

  zeCommandListAppendLaunchKernel(commandList, function, &tg, nullptr, 0,
                                  nullptr);

  lzt::close_command_list(commandList);
  lzt::execute_command_lists(commandQueue, 1, &commandList, nullptr);

  lzt::synchronize(commandQueue, UINT64_MAX);

  lzt::destroy_function(function);
  lzt::free_memory(a_buffer);
  lzt::free_memory(b_buffer);
  lzt::free_memory(c_buffer);

  lzt::destroy_command_queue(commandQueue);
  lzt::destroy_command_list(commandList);
}
