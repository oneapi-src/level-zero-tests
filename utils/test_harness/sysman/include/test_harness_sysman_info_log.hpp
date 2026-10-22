/*
 *
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef level_zero_tests_ZE_TEST_HARNESS_SYSMAN_INFO_LOG_HPP
#define level_zero_tests_ZE_TEST_HARNESS_SYSMAN_INFO_LOG_HPP

#include <level_zero/zes_api.h>
#include "gtest/gtest.h"

namespace level_zero_tests {

uint32_t get_info_log_handle_count(zes_driver_handle_t driver);

std::vector<zes_info_log_handle_t>
get_info_log_handles(zes_driver_handle_t driver, uint32_t &count);

zes_info_log_ext_properties_t
get_info_log_properties(zes_info_log_handle_t info_log_handle);

// Skips the calling test if the user lacks permission to collect records.
zes_info_log_instance_handle_t
create_info_log_instance(zes_info_log_handle_t info_log_handle,
                         const char *instance_name,
                         uint32_t *buffer_size_in_kb = nullptr);

void delete_info_log_instance(zes_info_log_instance_handle_t instance);

// Reads with zesInfoLogInstanceReadWithMetadataExt, or with
// zesInfoLogInstancePeekWithMetadataExt when peek is true. A query call is
// made when size or record_count is zero.
ze_result_t read_info_log_records(zes_info_log_instance_handle_t instance,
                                  bool peek, uint64_t timeout, uint32_t &size,
                                  uint8_t *buffer, uint32_t &record_count,
                                  zes_info_log_metadata_ext_t *descriptors,
                                  zes_info_log_read_status_ext_t *read_status);

bool is_info_log_read_successful(ze_result_t result);

} // namespace level_zero_tests

#endif
