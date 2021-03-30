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

class MemoryModuleTest : public lzt::SysmanCtsClass {};

TEST_F(
    MemoryModuleTest,
    GivenComponentCountZeroWhenRetrievingSysmanHandlesThenNonZeroCountIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto mem_handles = lzt::get_mem_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }
  }
}

TEST_F(
    MemoryModuleTest,
    GivenComponentCountZeroWhenRetrievingSysmanHandlesThenNotNullMemoryHandlesAreReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto mem_handles = lzt::get_mem_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    ASSERT_EQ(mem_handles.size(), count);
    for (auto mem_handle : mem_handles) {
      EXPECT_NE(nullptr, mem_handle);
    }
  }
}

TEST_F(
    MemoryModuleTest,
    GivenInvalidComponentCountWhenRetrievingSysmanHandlesThenActualComponentCountIsUpdated) {
  for (auto device : devices) {
    uint32_t actual_count = 0;
    lzt::get_mem_handles(device, actual_count);
    if (actual_count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    uint32_t test_count = actual_count + 1;
    lzt::get_mem_handles(device, test_count);
    EXPECT_EQ(test_count, actual_count);
  }
}

TEST_F(
    MemoryModuleTest,
    GivenValidComponentCountWhenCallingApiTwiceThenSimilarMemHandlesReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto mem_handles_initial = lzt::get_mem_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto mem_handle : mem_handles_initial) {
      EXPECT_NE(nullptr, mem_handle);
    }

    count = 0;
    auto mem_handles_later = lzt::get_mem_handles(device, count);
    for (auto mem_handle : mem_handles_later) {
      EXPECT_NE(nullptr, mem_handle);
    }
    EXPECT_EQ(mem_handles_initial, mem_handles_later);
  }
}

TEST_F(
    MemoryModuleTest,
    GivenValidMemHandleWhenRetrievingMemPropertiesThenValidPropertiesAreReturned) {
  for (auto device : devices) {
    auto deviceProperties = lzt::get_sysman_device_properties(device);
    uint32_t count = 0;
    auto mem_handles = lzt::get_mem_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto mem_handle : mem_handles) {
      ASSERT_NE(nullptr, mem_handle);
      auto properties = lzt::get_mem_properties(mem_handle);
      if (properties.onSubdevice) {
        EXPECT_LT(properties.subdeviceId, deviceProperties.numSubdevices);
      }
      EXPECT_LT(properties.physicalSize, UINT64_MAX);
      EXPECT_GE(properties.location, ZES_MEM_LOC_SYSTEM);
      EXPECT_LE(properties.location, ZES_MEM_LOC_DEVICE);
      EXPECT_LE(properties.busWidth, INT32_MAX);
      EXPECT_GE(properties.busWidth, -1);
      EXPECT_NE(properties.busWidth, 0);
      EXPECT_LE(properties.numChannels, INT32_MAX);
      EXPECT_GE(properties.numChannels, -1);
      EXPECT_NE(properties.numChannels, 0);
    }
  }
}

TEST_F(
    MemoryModuleTest,
    GivenValidMemHandleWhenRetrievingMemPropertiesThenExpectSamePropertiesReturnedTwice) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto mem_handles = lzt::get_mem_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto mem_handle : mem_handles) {
      EXPECT_NE(nullptr, mem_handle);
      auto properties_initial = lzt::get_mem_properties(mem_handle);
      auto properties_later = lzt::get_mem_properties(mem_handle);
      EXPECT_EQ(properties_initial.type, properties_later.type);
      EXPECT_EQ(properties_initial.onSubdevice, properties_later.onSubdevice);
      if (properties_initial.onSubdevice && properties_later.onSubdevice) {
        EXPECT_EQ(properties_initial.subdeviceId, properties_later.subdeviceId);
      }
      EXPECT_EQ(properties_initial.physicalSize, properties_later.physicalSize);
      EXPECT_EQ(properties_initial.location, properties_later.location);
      EXPECT_EQ(properties_initial.busWidth, properties_later.busWidth);
      EXPECT_EQ(properties_initial.numChannels, properties_later.numChannels);
    }
  }
}

TEST_F(
    MemoryModuleTest,
    GivenValidMemHandleWhenRetrievingMemBandWidthThenValidBandWidthCountersAreReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto mem_handles = lzt::get_mem_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto mem_handle : mem_handles) {
      ASSERT_NE(nullptr, mem_handle);
      auto bandwidth = lzt::get_mem_bandwidth(mem_handle);
      EXPECT_LT(bandwidth.readCounter, UINT64_MAX);
      EXPECT_LT(bandwidth.writeCounter, UINT64_MAX);
      EXPECT_LT(bandwidth.maxBandwidth, UINT64_MAX);
      EXPECT_LT(bandwidth.timestamp, UINT64_MAX);
    }
  }
}

TEST_F(MemoryModuleTest,
       GivenValidMemHandleWhenRetrievingMemStateThenValidStateIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto mem_handles = lzt::get_mem_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto mem_handle : mem_handles) {
      ASSERT_NE(nullptr, mem_handle);
      auto state = lzt::get_mem_state(mem_handle);
      EXPECT_GE(state.health, ZES_MEM_HEALTH_UNKNOWN);
      EXPECT_LE(state.health, ZES_MEM_HEALTH_REPLACE);
      auto properties = lzt::get_mem_properties(mem_handle);
      if (properties.physicalSize != 0) {
        EXPECT_LE(state.size, properties.physicalSize);
      }
      EXPECT_LE(state.free, state.size);
    }
  }
}

} // namespace
