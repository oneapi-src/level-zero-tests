/*
 *
 * Copyright (C) 2023-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef TEST_METRIC_UTILS_HPP
#define TEST_METRIC_UTILS_HPP

#include <level_zero/ze_api.h>
#include <level_zero/zet_api.h>

ze_kernel_handle_t get_matrix_multiplication_kernel(
    ze_device_handle_t device, ze_group_count_t *tg, void **a_buffer,
    void **b_buffer, void **c_buffer, uint32_t dimensions = 1024);

void metric_validate_stall_sampling_data(
    std::vector<zet_metric_properties_t> &metricProperties,
    std::vector<zet_typed_value_t> &totalMetricValues,
    std::vector<uint32_t> &metricValueSets);

void metric_run_ip_sampling_with_validation(
    bool enableOverflow, const std::vector<ze_device_handle_t> &devices,
    uint32_t notifyEveryNReports, uint32_t samplingPeriod,
    uint32_t timeForNReportsComplete, uint32_t dimensions = 8192);

bool success_metric_tracer_read_data_retry(
    zet_metric_tracer_exp_handle_t metric_tracer_handle,
    int32_t number_of_retries, int32_t retry_wait_milliseconds,
    bool check_read_non_zero_size, size_t *ptr_raw_data_size,
    uint8_t *ptr_raw_data);

#endif // TEST_METRIC_UTILS_HPP