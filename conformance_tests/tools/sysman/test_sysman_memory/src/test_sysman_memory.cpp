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

#include <level_zero/ze_api.h>
#include <level_zero/zet_api.h>

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
      EXPECT_GE(properties.type, ZET_MEM_TYPE_HBM);
      EXPECT_LE(properties.type, ZET_MEM_TYPE_SLM);
      if (properties.onSubdevice == true) {
        EXPECT_LT(properties.subdeviceId, UINT32_MAX);
      } else {
        EXPECT_EQ(0, properties.subdeviceId);
      }
      EXPECT_LT(properties.physicalSize, UINT64_MAX);
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
      EXPECT_GE(state.health, ZET_MEM_HEALTH_OK);
      EXPECT_LE(state.health, ZET_MEM_HEALTH_REPLACE);
      auto properties = lzt::get_mem_properties(memHandle);
      EXPECT_LE(state.maxSize, properties.physicalSize);
      EXPECT_LT(state.allocatedSize, state.maxSize);
    }
  }
}

} // namespace
