/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "test_harness/test_harness.hpp"

#include <level_zero/ze_api.h>
#include "utils/utils.hpp"

namespace lzt = level_zero_tests;

namespace level_zero_tests {
uint32_t get_standby_handle_count(ze_device_handle_t device) {
  zet_sysman_handle_t hSysmanDevice = lzt::get_sysman_handle(device);
  uint32_t pCount = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanStandbyGet(hSysmanDevice, &pCount, nullptr));
  EXPECT_GT(pCount, 0);
  return pCount;
}
std::vector<zet_sysman_standby_handle_t>
get_standby_handles(ze_device_handle_t device, uint32_t &pCount) {
  if (pCount == 0)
    pCount = get_standby_handle_count(device);
  zet_sysman_handle_t hSysmanDevice = lzt::get_sysman_handle(device);
  std::vector<zet_sysman_standby_handle_t> pStandbyHandles(pCount);
  EXPECT_EQ(ZE_RESULT_SUCCESS, zetSysmanStandbyGet(hSysmanDevice, &pCount,
                                                   pStandbyHandles.data()));
  return pStandbyHandles;
}
zet_standby_promo_mode_t
get_standby_mode(zet_sysman_standby_handle_t pStandByHandle) {
  zet_standby_promo_mode_t pMode;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zetSysmanStandbyGetMode(pStandByHandle, &pMode));
  return pMode;
}
void set_standby_mode(zet_sysman_standby_handle_t pStandByHandle,
                      zet_standby_promo_mode_t pMode) {
  EXPECT_EQ(ZE_RESULT_SUCCESS, zetSysmanStandbySetMode(pStandByHandle, pMode));
}
zet_standby_properties_t
get_standby_properties(zet_sysman_standby_handle_t pStandbyHandle) {
  zet_standby_properties_t pProperties;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanStandbyGetProperties(pStandbyHandle, &pProperties));
  return pProperties;
}
} // namespace level_zero_tests
