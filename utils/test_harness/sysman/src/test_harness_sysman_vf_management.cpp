/*
 *
 * Copyright (C) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "test_harness/test_harness.hpp"

#include <level_zero/zes_api.h>
#include "utils/utils.hpp"

namespace level_zero_tests {

uint32_t get_vf_handles_count(zes_device_handle_t device) {
  uint32_t count = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesDeviceEnumEnabledVFExp(device, &count, nullptr));
  return count;
}

std::vector<zes_vf_handle_t> get_vf_handles(zes_device_handle_t device,
                                            uint32_t &count) {
  if (count == 0) {
    count = get_vf_handles_count(device);
  }
  std::vector<zes_vf_handle_t> vf_handles(count);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesDeviceEnumEnabledVFExp(device, &count, vf_handles.data()));
  return vf_handles;
}

} // namespace level_zero_tests
