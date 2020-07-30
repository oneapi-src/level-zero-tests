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
    lzt::get_mem_handle_count(device);
  }
}

TEST_F(
    MemoryModuleTest,
    GivenComponentCountZeroWhenRetrievingSysmanHandlesThenNotNullMemoryHandlesAreReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto memHandles = lzt::get_mem_handles(device, count);
    ASSERT_EQ(memHandles.size(), count);
    for (auto memHandle : memHandles) {
      EXPECT_NE(nullptr, memHandle);
    }
  }
}

TEST_F(
    MemoryModuleTest,
    GivenInvalidComponentCountWhenRetrievingSysmanHandlesThenActualComponentCountIsUpdated) {
  for (auto device : devices) {
    uint32_t actualCount = 0;
    lzt::get_mem_handles(device, actualCount);
    uint32_t testCount = actualCount + 1;
    lzt::get_mem_handles(device, testCount);
    EXPECT_EQ(testCount, actualCount);
  }
}

TEST_F(
    MemoryModuleTest,
    GivenValidComponentCountWhenCallingApiTwiceThenSimilarMemHandlesReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto memHandlesInitial = lzt::get_mem_handles(device, count);
    for (auto memHandle : memHandlesInitial) {
      EXPECT_NE(nullptr, memHandle);
    }

    count = 0;
    auto memHandlesLater = lzt::get_mem_handles(device, count);
    for (auto memHandle : memHandlesLater) {
      EXPECT_NE(nullptr, memHandle);
    }
    EXPECT_EQ(memHandlesInitial, memHandlesLater);
  }
}

TEST_F(
    MemoryModuleTest,
    GivenValidMemHandleWhenRetrievingMemPropertiesThenValidPropertiesAreReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto memHandles = lzt::get_mem_handles(device, count);
    for (auto memHandle : memHandles) {
      ASSERT_NE(nullptr, memHandle);
      auto properties = lzt::get_mem_properties(memHandle);
      if (properties.onSubdevice == true) {
        EXPECT_LT(properties.subdeviceId, UINT32_MAX);
      } else {
        EXPECT_EQ(0, properties.subdeviceId);
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
    auto memHandles = lzt::get_mem_handles(device, count);
    for (auto memHandle : memHandles) {
      EXPECT_NE(nullptr, memHandle);
      auto propertiesInitial = lzt::get_mem_properties(memHandle);
      auto propertiesLater = lzt::get_mem_properties(memHandle);
      EXPECT_EQ(propertiesInitial.type, propertiesLater.type);
      EXPECT_EQ(propertiesInitial.onSubdevice, propertiesLater.onSubdevice);
      if (propertiesInitial.onSubdevice == true &&
          propertiesLater.onSubdevice == true) {
        EXPECT_EQ(propertiesInitial.subdeviceId, propertiesLater.subdeviceId);
      }
      EXPECT_EQ(propertiesInitial.physicalSize, propertiesLater.physicalSize);
      EXPECT_EQ(propertiesInitial.location, propertiesLater.location);
      EXPECT_EQ(propertiesInitial.busWidth, propertiesLater.busWidth);
      EXPECT_EQ(propertiesInitial.numChannels, propertiesLater.numChannels);
    }
  }
}

TEST_F(
    MemoryModuleTest,
    GivenValidMemHandleWhenRetrievingMemBandWidthThenValidBandWidthCountersAreReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto memHandles = lzt::get_mem_handles(device, count);
    for (auto memHandle : memHandles) {
      ASSERT_NE(nullptr, memHandle);
      auto bandwidth = lzt::get_mem_bandwidth(memHandle);
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
    auto memHandles = lzt::get_mem_handles(device, count);
    for (auto memHandle : memHandles) {
      ASSERT_NE(nullptr, memHandle);
      auto state = lzt::get_mem_state(memHandle);
      EXPECT_GE(state.health, ZES_MEM_HEALTH_UNKNOWN);
      EXPECT_LE(state.health, ZES_MEM_HEALTH_REPLACE);
      auto properties = lzt::get_mem_properties(memHandle);
      EXPECT_LE(state.size, properties.physicalSize);
      EXPECT_LE(state.free, state.size);
    }
  }
}

} // namespace
