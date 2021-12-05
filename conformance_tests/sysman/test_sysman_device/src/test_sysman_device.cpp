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

uint32_t get_prop_length(char prop[ZES_STRING_PROPERTY_SIZE]) {
  uint32_t length = 0;
  for (int i = 0; i < ZES_STRING_PROPERTY_SIZE; i++) {
    if (prop[i] == '\0') {
      break;
    } else {
      length += 1;
    }
  }

  return length;
}

class SysmanDeviceTest : public lzt::SysmanCtsClass {};

TEST_F(
    SysmanDeviceTest,
    GivenValidDeviceWhenRetrievingSysmanDevicePropertiesThenValidPropertiesAreReturned) {
  for (auto device : devices) {
    auto properties = lzt::get_sysman_device_properties(device);

    EXPECT_GE(ZE_DEVICE_TYPE_GPU, properties.core.type);
    EXPECT_LE(properties.core.type, ZE_DEVICE_TYPE_MCA);
    if (properties.core.flags <= ZE_DEVICE_PROPERTY_FLAG_INTEGRATED |
        ZE_DEVICE_PROPERTY_FLAG_SUBDEVICE | ZE_DEVICE_PROPERTY_FLAG_ECC |
        ZE_DEVICE_PROPERTY_FLAG_ONDEMANDPAGING) {
      if (properties.core.flags & ZE_DEVICE_PROPERTY_FLAG_SUBDEVICE) {
        EXPECT_LT(properties.core.subdeviceId, UINT32_MAX);
      } else {
        EXPECT_EQ(0, properties.core.subdeviceId);
      }
    } else {
      FAIL();
    }
    EXPECT_LE(get_prop_length(properties.serialNumber),
              ZES_STRING_PROPERTY_SIZE);
    EXPECT_LE(get_prop_length(properties.boardNumber),
              ZES_STRING_PROPERTY_SIZE);
    EXPECT_LE(get_prop_length(properties.brandName), ZES_STRING_PROPERTY_SIZE);
    EXPECT_LE(get_prop_length(properties.modelName), ZES_STRING_PROPERTY_SIZE);
    EXPECT_LE(get_prop_length(properties.vendorName), ZES_STRING_PROPERTY_SIZE);
    EXPECT_LE(get_prop_length(properties.driverVersion),
              ZES_STRING_PROPERTY_SIZE);
  }
}

TEST_F(
    SysmanDeviceTest,
    GivenValidDeviceWhenRetrievingSysmanDevicePropertiesThenExpectSamePropertiesReturnedTwice) {
  for (auto device : devices) {
    auto properties_initial = lzt::get_sysman_device_properties(device);
    auto properties_later = lzt::get_sysman_device_properties(device);
    EXPECT_EQ(properties_initial.core.type, properties_later.core.type);
    if (properties_initial.core.flags <= ZE_DEVICE_PROPERTY_FLAG_INTEGRATED |
            ZE_DEVICE_PROPERTY_FLAG_SUBDEVICE | ZE_DEVICE_PROPERTY_FLAG_ECC |
            ZE_DEVICE_PROPERTY_FLAG_ONDEMANDPAGING &&
        properties_initial.core.flags <= ZE_DEVICE_PROPERTY_FLAG_INTEGRATED |
            ZE_DEVICE_PROPERTY_FLAG_SUBDEVICE | ZE_DEVICE_PROPERTY_FLAG_ECC |
            ZE_DEVICE_PROPERTY_FLAG_ONDEMANDPAGING) {
      EXPECT_EQ(properties_initial.core.flags, properties_later.core.flags);
      if (properties_initial.core.flags & ZE_DEVICE_PROPERTY_FLAG_SUBDEVICE &&
          properties_later.core.flags & ZE_DEVICE_PROPERTY_FLAG_SUBDEVICE) {
        EXPECT_EQ(properties_initial.core.subdeviceId,
                  properties_later.core.subdeviceId);
      }
    } else {
      FAIL();
    }
    EXPECT_TRUE(0 ==
                std::memcmp(properties_initial.serialNumber,
                            properties_later.serialNumber,
                            get_prop_length(properties_initial.serialNumber)));
    EXPECT_TRUE(0 ==
                std::memcmp(properties_initial.boardNumber,
                            properties_later.boardNumber,
                            get_prop_length(properties_initial.boardNumber)));

    EXPECT_TRUE(0 ==
                std::memcmp(properties_initial.brandName,
                            properties_later.brandName,
                            get_prop_length(properties_initial.brandName)));

    EXPECT_TRUE(0 ==
                std::memcmp(properties_initial.modelName,
                            properties_later.modelName,
                            get_prop_length(properties_initial.modelName)));

    EXPECT_TRUE(0 ==
                std::memcmp(properties_initial.vendorName,
                            properties_later.vendorName,
                            get_prop_length(properties_initial.vendorName)));

    EXPECT_TRUE(0 ==
                std::memcmp(properties_initial.driverVersion,
                            properties_later.driverVersion,
                            get_prop_length(properties_initial.driverVersion)));
  }
}

TEST_F(
    SysmanDeviceTest,
    GivenProcessesCountZeroWhenRetrievingProcessesStateThenSuccessIsReturned) {
  for (auto device : devices) {
    lzt::get_processes_count(device);
  }
}

TEST_F(
    SysmanDeviceTest,
    GivenProcessesCountZeroWhenRetrievingProcessesStateThenValidProcessesStateAreReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto processes = lzt::get_processes_state(device, count);
    if (processes.size() > 0) {
      for (auto process : processes) {
        EXPECT_GT(process.processId, 0u);
        EXPECT_GE(process.memSize, 0u);
        EXPECT_LT(process.sharedSize, UINT64_MAX);
        EXPECT_GE(process.engines, 0);
        EXPECT_LE(process.engines, (1 << ZES_ENGINE_TYPE_FLAG_DMA));
      }
    }
  }
}

TEST_F(
    SysmanDeviceTest,
    GivenInvalidComponentCountWhenRetrievingSysmanHandlesThenActualComponentCountIsUpdated) {
  for (auto device : devices) {
    uint32_t actual_count = 0;
    lzt::get_processes_state(device, actual_count);
    uint32_t test_count = actual_count + 1;
    lzt::get_processes_state(device, test_count);
    EXPECT_EQ(test_count, actual_count);
  }
}

TEST_F(
    SysmanDeviceTest,
    GivenValidDeviceWhenRetrievingSysmanDeviceStateThenValidStateIsReturned) {
  for (auto device : devices) {
    auto state = lzt::get_device_state(device);
    EXPECT_GE(state.reset, 0);
    EXPECT_LE(state.reset,
              ZES_RESET_REASON_FLAG_WEDGED | ZES_RESET_REASON_FLAG_REPAIR);
    EXPECT_GE(state.repaired, ZES_REPAIR_STATUS_UNSUPPORTED);
    EXPECT_LE(state.repaired, ZES_REPAIR_STATUS_PERFORMED);
  }
}

TEST_F(
    SysmanDeviceTest,
    GivenValidDeviceWhenResettingSysmanDeviceThenSysmanDeviceResetIsSucceded) {
  for (auto device : devices) {
    lzt::sysman_device_reset(device);
  }
}

TEST_F(
    SysmanDeviceTest,
    GivenValidDeviceWhenResettingSysmanDeviceNnumberOfTimesThenSysmanDeviceResetAlwaysSucceded) {
  int number_iterations = 2;
  for (int i = 0; i < number_iterations; i++) {
    for (auto device : devices) {
      lzt::sysman_device_reset(device);
    }
  }
}
} // namespace
