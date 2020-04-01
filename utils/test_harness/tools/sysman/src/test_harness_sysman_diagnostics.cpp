/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "test_harness/test_harness.hpp"

#include <level_zero/ze_api.h>
#include "utils/utils.hpp"

namespace lzt = level_zero_tests;

namespace level_zero_tests {

uint32_t get_diag_handle_count(ze_device_handle_t device) {
  uint32_t count = 0;
  zet_sysman_handle_t hSysman = lzt::get_sysman_handle(device);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanDiagnosticsGet(hSysman, &count, nullptr));
  EXPECT_GT(count, 0);
  return count;
}

std::vector<zet_sysman_diag_handle_t>
get_diag_handles(ze_device_handle_t device, uint32_t &count) {
  zet_sysman_handle_t hSysman = lzt::get_sysman_handle(device);
  if (count == 0)
    count = get_diag_handle_count(device);
  std::vector<zet_sysman_diag_handle_t> diagHandles(count);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanDiagnosticsGet(hSysman, &count, diagHandles.data()));
  EXPECT_EQ(diagHandles.size(), count);
  return diagHandles;
}

zet_diag_properties_t get_diag_properties(zet_sysman_diag_handle_t diagHandle) {
  zet_diag_properties_t properties;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanDiagnosticsGetProperties(diagHandle, &properties));
  return properties;
}

std::vector<zet_diag_test_t> get_diag_tests(zet_sysman_diag_handle_t diagHandle,
                                            uint32_t &count) {
  if (count == 0) {
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zetSysmanDiagnosticsGetTests(diagHandle, &count, nullptr));
    EXPECT_GT(count, 0);
  }
  std::vector<zet_diag_test_t> diagTests(count);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanDiagnosticsGetTests(diagHandle, &count, diagTests.data()));
  EXPECT_EQ(diagTests.size(), count);
  return diagTests;
}

zet_diag_result_t run_diag_tests(zet_sysman_diag_handle_t diagHandle,
                                 uint32_t start, uint32_t end) {
  zet_diag_result_t result;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanDiagnosticsRunTests(diagHandle, start, end, &result));
  return result;
}

} // namespace level_zero_tests
