/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "test_harness/test_harness.hpp"

#include <level_zero/zes_api.h>
#include "utils/utils.hpp"

namespace lzt = level_zero_tests;

namespace level_zero_tests {

uint32_t get_diag_handle_count(zes_device_handle_t device) {
  uint32_t count = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesDeviceEnumDiagnosticTestSuites(device, &count, nullptr));
  EXPECT_GE(count, 0);
  return count;
}

std::vector<zes_diag_handle_t> get_diag_handles(zes_device_handle_t device,
                                                uint32_t &count) {
  if (count == 0)
    count = get_diag_handle_count(device);
  std::vector<zes_diag_handle_t> diag_handles(count, nullptr);
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesDeviceEnumDiagnosticTestSuites(
                                   device, &count, diag_handles.data()));
  return diag_handles;
}

zes_diag_properties_t get_diag_properties(zes_diag_handle_t diag_handle) {
  zes_diag_properties_t properties = {ZES_STRUCTURE_TYPE_DIAG_PROPERTIES,
                                      nullptr};
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesDiagnosticsGetProperties(diag_handle, &properties));
  return properties;
}

std::vector<zes_diag_test_t> get_diag_tests(zes_diag_handle_t diag_handle,
                                            uint32_t &count) {
  if (count == 0) {
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zesDiagnosticsGetTests(diag_handle, &count, nullptr));
    EXPECT_GT(count, 0);
  }
  std::vector<zes_diag_test_t> diagTests(count);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesDiagnosticsGetTests(diag_handle, &count, diagTests.data()));
  EXPECT_EQ(diagTests.size(), count);
  return diagTests;
}

zes_diag_result_t run_diag_tests(zes_diag_handle_t diag_handle, uint32_t start,
                                 uint32_t end) {
  zes_diag_result_t result = {};
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesDiagnosticsRunTests(diag_handle, start, end, &result));
  return result;
}

} // namespace level_zero_tests
