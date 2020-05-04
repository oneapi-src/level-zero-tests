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

uint32_t get_engine_handle_count(ze_device_handle_t device) {
  uint32_t count = 0;
  zet_sysman_handle_t hSysman = lzt::get_sysman_handle(device);
  EXPECT_EQ(ZE_RESULT_SUCCESS, zetSysmanEngineGet(hSysman, &count, nullptr));
  EXPECT_GT(count, 0);
  return count;
}

std::vector<zet_sysman_engine_handle_t>
get_engine_handles(ze_device_handle_t device, uint32_t &count) {
  zet_sysman_handle_t hSysman = lzt::get_sysman_handle(device);
  if (count == 0)
    count = get_engine_handle_count(device);
  std::vector<zet_sysman_engine_handle_t> engineHandles(count);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanEngineGet(hSysman, &count, engineHandles.data()));
  return engineHandles;
}

zet_engine_properties_t
get_engine_properties(zet_sysman_engine_handle_t hEngine) {
  zet_engine_properties_t properties;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanEngineGetProperties(hEngine, &properties));
  return properties;
}

zet_engine_stats_t get_engine_activity(zet_sysman_engine_handle_t hEngine) {
  zet_engine_stats_t stats;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zetSysmanEngineGetActivity(hEngine, &stats));
  return stats;
}

} // namespace level_zero_tests
