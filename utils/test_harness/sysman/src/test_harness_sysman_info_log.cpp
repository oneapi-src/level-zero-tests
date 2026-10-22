/*
 *
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "test_harness/test_harness.hpp"

#include <level_zero/zes_api.h>
#include "utils/utils.hpp"

namespace lzt = level_zero_tests;

namespace level_zero_tests {

uint32_t get_info_log_handle_count(zes_driver_handle_t driver) {
  uint32_t count = 0;
  EXPECT_ZE_RESULT_SUCCESS(zesDriverEnumInfoLogsExt(driver, &count, nullptr));
  return count;
}

std::vector<zes_info_log_handle_t>
get_info_log_handles(zes_driver_handle_t driver, uint32_t &count) {
  if (count == 0) {
    count = get_info_log_handle_count(driver);
  }
  std::vector<zes_info_log_handle_t> info_log_handles(count, nullptr);
  EXPECT_ZE_RESULT_SUCCESS(
      zesDriverEnumInfoLogsExt(driver, &count, info_log_handles.data()));
  return info_log_handles;
}

zes_info_log_ext_properties_t
get_info_log_properties(zes_info_log_handle_t info_log_handle) {
  zes_info_log_ext_properties_t properties = {
      ZES_STRUCTURE_TYPE_INFO_LOG_EXT_PROPERTIES, nullptr};
  EXPECT_ZE_RESULT_SUCCESS(
      zesInfoLogGetPropertiesExt(info_log_handle, &properties));
  return properties;
}

zes_info_log_instance_handle_t
create_info_log_instance(zes_info_log_handle_t info_log_handle,
                         const char *instance_name,
                         uint32_t *buffer_size_in_kb) {
  zes_info_log_instance_ext_desc_t desc = {
      ZES_STRUCTURE_TYPE_INFO_LOG_INSTANCE_EXT_DESC, nullptr,
      buffer_size_in_kb};
  zes_info_log_instance_handle_t instance = nullptr;
  ze_result_t result = zesInfoLogCreateInstanceExt(
      info_log_handle, instance_name, &desc, &instance);
  if (result == ZE_RESULT_ERROR_INSUFFICIENT_PERMISSIONS) {
    throw LztGtestSkipExecutionException(
        "User does not have permissions to collect info log records");
  }
  EXPECT_ZE_RESULT_SUCCESS(result);
  EXPECT_NE(nullptr, instance);
  return instance;
}

void delete_info_log_instance(zes_info_log_instance_handle_t instance) {
  EXPECT_ZE_RESULT_SUCCESS(zesInfoLogInstanceDeleteExt(instance));
}

ze_result_t read_info_log_records(zes_info_log_instance_handle_t instance,
                                  bool peek, uint64_t timeout, uint32_t &size,
                                  uint8_t *buffer, uint32_t &record_count,
                                  zes_info_log_metadata_ext_t *descriptors,
                                  zes_info_log_read_status_ext_t *read_status) {
  if (peek) {
    return zesInfoLogInstancePeekWithMetadataExt(instance, timeout, &size,
                                                 buffer, &record_count,
                                                 descriptors, read_status);
  }
  return zesInfoLogInstanceReadWithMetadataExt(instance, timeout, &size, buffer,
                                               &record_count, descriptors,
                                               read_status);
}

bool is_info_log_read_successful(ze_result_t result) {
  return result == ZE_RESULT_SUCCESS ||
         result == ZE_RESULT_WARNING_DROPPED_DATA;
}

} // namespace level_zero_tests
