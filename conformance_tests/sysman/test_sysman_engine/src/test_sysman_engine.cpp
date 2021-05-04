/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "logging/logging.hpp"
#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"

namespace lzt = level_zero_tests;

#include <level_zero/zes_api.h>

namespace {

class EngineModuleTest : public lzt::SysmanCtsClass {};

TEST_F(
    EngineModuleTest,
    GivenComponentCountZeroWhenRetrievingSysmanEngineHandlesThenNonZeroCountIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    count = lzt::get_engine_handle_count(device);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }
  }
}

TEST_F(
    EngineModuleTest,
    GivenComponentCountZeroWhenRetrievingSysmanEngineHandlesThenNotNullEngineHandlesAreReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto engine_handles = lzt::get_engine_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    ASSERT_EQ(engine_handles.size(), count);
    for (auto engine_handle : engine_handles) {
      EXPECT_NE(nullptr, engine_handle);
    }
  }
}

TEST_F(
    EngineModuleTest,
    GivenInvalidComponentCountWhenRetrievingSysmanEngineHandlesThenActualComponentCountIsUpdated) {
  for (auto device : devices) {
    uint32_t actual_count = 0;
    lzt::get_engine_handles(device, actual_count);
    if (actual_count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    uint32_t test_count = actual_count + 1;
    lzt::get_engine_handles(device, test_count);
    EXPECT_EQ(test_count, actual_count);
  }
}

TEST_F(
    EngineModuleTest,
    GivenValidComponentCountWhenCallingApiTwiceThenSimilarEngineHandlesReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto engine_handles_initial = lzt::get_engine_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto engine_handle : engine_handles_initial) {
      EXPECT_NE(nullptr, engine_handle);
    }

    count = 0;
    auto engine_handles_later = lzt::get_engine_handles(device, count);
    for (auto engine_handle : engine_handles_later) {
      EXPECT_NE(nullptr, engine_handle);
    }
    EXPECT_EQ(engine_handles_initial, engine_handles_later);
  }
}

TEST_F(
    EngineModuleTest,
    GivenValidEngineHandleWhenRetrievingEnginePropertiesThenValidPropertiesAreReturned) {
  for (auto device : devices) {
    auto deviceProperties = lzt::get_sysman_device_properties(device);
    uint32_t count = 0;
    auto engine_handles = lzt::get_engine_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto engine_handle : engine_handles) {
      ASSERT_NE(nullptr, engine_handle);
      auto properties = lzt::get_engine_properties(engine_handle);
      EXPECT_GE(properties.type, ZES_ENGINE_GROUP_ALL);
      EXPECT_LE(properties.type, ZES_ENGINE_GROUP_3D_ALL);
      if (properties.onSubdevice) {
        EXPECT_LT(properties.subdeviceId, deviceProperties.numSubdevices);
      }
    }
  }
}

TEST_F(
    EngineModuleTest,
    GivenValidEngineHandleWhenRetrievingEnginePropertiesThenExpectSamePropertiesReturnedTwice) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto engine_handles = lzt::get_engine_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto engine_handle : engine_handles) {
      EXPECT_NE(nullptr, engine_handle);
      auto properties_initial = lzt::get_engine_properties(engine_handle);
      auto properties_later = lzt::get_engine_properties(engine_handle);
      EXPECT_EQ(properties_initial.type, properties_later.type);
      EXPECT_EQ(properties_initial.onSubdevice, properties_later.onSubdevice);
      if (properties_initial.onSubdevice && properties_later.onSubdevice) {
        EXPECT_EQ(properties_initial.subdeviceId, properties_later.subdeviceId);
      }
    }
  }
}

TEST_F(
    EngineModuleTest,
    GivenValidEngineHandleWhenRetrievingEngineActivityStatsThenValidStatsIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto engine_handles = lzt::get_engine_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto engine_handle : engine_handles) {
      ASSERT_NE(nullptr, engine_handle);
      auto state = lzt::get_engine_activity(engine_handle);
      EXPECT_LT(state.activeTime, UINT32_MAX);
      EXPECT_LT(state.timestamp, UINT32_MAX);
    }
  }
}
TEST_F(
    EngineModuleTest,
    GivenValidEngineHandleWhenRetrievingEngineActivityStatsThenTimestampWillbeIncrementedInNextCalltoEngineActivity) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto engine_handles = lzt::get_engine_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto engine_handle : engine_handles) {
      ASSERT_NE(nullptr, engine_handle);
      auto oldstate = lzt::get_engine_activity(engine_handle);
      EXPECT_LT(oldstate.activeTime, UINT32_MAX);
      EXPECT_LT(oldstate.timestamp, UINT32_MAX);
      auto newstate = lzt::get_engine_activity(engine_handle);
      EXPECT_LT(newstate.activeTime, UINT32_MAX);
      EXPECT_LT(newstate.timestamp, UINT32_MAX);
      EXPECT_GT(newstate.timestamp, oldstate.timestamp);
    }
  }
}
} // namespace
