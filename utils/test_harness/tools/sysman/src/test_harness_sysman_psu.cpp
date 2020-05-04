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

uint32_t get_psu_handles_count(ze_device_handle_t device) {
  uint32_t count = 0;
  zet_sysman_handle_t hSysman = lzt::get_sysman_handle(device);
  EXPECT_EQ(ZE_RESULT_SUCCESS, zetSysmanPsuGet(hSysman, &count, nullptr));
  EXPECT_GT(count, 0);
  return count;
}

std::vector<zet_sysman_psu_handle_t> get_psu_handles(ze_device_handle_t device,
                                                     uint32_t &count) {
  zet_sysman_handle_t hSysman = lzt::get_sysman_handle(device);
  if (count == 0) {
    count = get_psu_handles_count(device);
  }
  std::vector<zet_sysman_psu_handle_t> psuHandles(count);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanPsuGet(hSysman, &count, psuHandles.data()));
  return psuHandles;
}

zet_psu_properties_t get_psu_properties(zet_sysman_psu_handle_t psuHandle) {
  zet_psu_properties_t properties;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanPsuGetProperties(psuHandle, &properties));
  return properties;
}
zet_psu_state_t get_psu_state(zet_sysman_psu_handle_t psuHandle) {
  zet_psu_state_t state;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zetSysmanPsuGetState(psuHandle, &state));
  return state;
}
} // namespace level_zero_tests
