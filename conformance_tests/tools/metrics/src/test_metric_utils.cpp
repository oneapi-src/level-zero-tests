/*
 *
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"

namespace lzt = level_zero_tests;

ze_kernel_handle_t get_matrix_multiplication_kernel(
    ze_device_handle_t device, ze_group_count_t *tg, void **a_buffer,
    void **b_buffer, void **c_buffer, int dimensions = 1024) {

  const char *dimensions_test_environment_variable =
      std::getenv("LZT_METRICS_MATRIX_MULTIPLICATION_DIMENSIONS");
  if (dimensions_test_environment_variable != nullptr) {
    dimensions = atoi(dimensions_test_environment_variable);
    LOG_INFO
        << "overriding the matrix multiplication dimension as "
           "LZT_METRICS_MATRIX_MULTIPLICATION_DIMENSIONS is used with value of "
        << dimensions;
  }

  int m, k, n;
  m = k = n = dimensions;
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
      lzt::create_module(device, "ze_matrix_multiplication_metrics.spv",
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