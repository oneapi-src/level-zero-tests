/*
 *
 * Copyright (C) 2019-2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "test_harness/test_harness.hpp"

#include <level_zero/zes_api.h>
#include "utils/utils.hpp"

namespace lzt = level_zero_tests;

namespace level_zero_tests {
uint32_t get_standby_handle_count(zes_device_handle_t device) {
  uint32_t pCount = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesDeviceEnumStandbyDomains(device, &pCount, nullptr));
  EXPECT_GE(pCount, 0);
  return pCount;
}
std::vector<zes_standby_handle_t>
get_standby_handles(zes_device_handle_t device, uint32_t &pCount) {
  if (pCount == 0)
    pCount = get_standby_handle_count(device);
  std::vector<zes_standby_handle_t> pStandbyHandles(pCount);
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesDeviceEnumStandbyDomains(
                                   device, &pCount, pStandbyHandles.data()));
  return pStandbyHandles;
}
zes_standby_promo_mode_t get_standby_mode(zes_standby_handle_t pStandByHandle) {
  zes_standby_promo_mode_t pMode = {};
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesStandbyGetMode(pStandByHandle, &pMode));
  return pMode;
}
void set_standby_mode(zes_standby_handle_t pStandByHandle,
                      zes_standby_promo_mode_t pMode) {
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesStandbySetMode(pStandByHandle, pMode));
}
zes_standby_properties_t
get_standby_properties(zes_standby_handle_t pStandbyHandle) {
  zes_standby_properties_t pProperties = {ZES_STRUCTURE_TYPE_STANDBY_PROPERTIES,
                                          nullptr};
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesStandbyGetProperties(pStandbyHandle, &pProperties));
  return pProperties;
}
} // namespace level_zero_tests
