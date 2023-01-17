/*
 *
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef TEST_METRIC_UTILS_HPP
#define TEST_METRIC_UTILS_HPP

#include <level_zero/ze_api.h>

ze_kernel_handle_t get_matrix_multiplication_kernel(
    ze_device_handle_t device, ze_group_count_t *tg, void **a_buffer,
    void **b_buffer, void **c_buffer, int dimensions = 1024);

#endif // TEST_METRIC_UTILS_HPP