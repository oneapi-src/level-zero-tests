/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "test_harness/test_harness.hpp"

#include <level_zero/zes_api.h>
#include "utils/utils.hpp"

namespace lzt = level_zero_tests;

namespace level_zero_tests {

uint32_t get_engine_handle_count(zes_device_handle_t device) {
  uint32_t count = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesDeviceEnumEngineGroups(device, &count, nullptr));
  EXPECT_GE(count, 0);
  return count;
}

std::vector<zes_engine_handle_t> get_engine_handles(zes_device_handle_t device,
                                                    uint32_t &count) {
  if (count == 0)
    count = get_engine_handle_count(device);
  std::vector<zes_engine_handle_t> engineHandles(count);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zesDeviceEnumEngineGroups(device, &count, engineHandles.data()));
  return engineHandles;
}

zes_engine_properties_t get_engine_properties(zes_engine_handle_t hEngine) {
  zes_engine_properties_t properties = {ZES_STRUCTURE_TYPE_ENGINE_PROPERTIES,
                                        nullptr};
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesEngineGetProperties(hEngine, &properties));
  return properties;
}

zes_engine_stats_t get_engine_activity(zes_engine_handle_t hEngine) {
  zes_engine_stats_t stats = {};
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesEngineGetActivity(hEngine, &stats));
  return stats;
}

} // namespace level_zero_tests
