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
    GivenValidDeviceWhenResettingSysmanDeviceThenSysmanDeviceResetIsSucceded) {
  for (auto device : devices) {
    lzt::sysman_device_reset(device);
  }
}

TEST_F(
    SysmanDeviceTest,
    GivenValidDeviceWhenResettingSysmanDeviceNnumberOfTimesThenSysmanDeviceResetAlwaysSucceded) {
  int number_iterations = 10;
  for (int i = 0; i < number_iterations; i++) {
    for (auto device : devices) {
      lzt::sysman_device_reset(device);
    }
  }
}

TEST_F(
    SysmanDeviceTest,
    GivenValidDeviceWhenRetrievingSysmanDevicePropertiesThenValidPropertiesAreReturned) {
  for (auto device : devices) {
    auto properties = lzt::get_sysman_device_properties(device);

    EXPECT_GE(ZE_DEVICE_TYPE_GPU, properties.core.type);
    EXPECT_LE(ZE_DEVICE_TYPE_MCA, properties.core.type);
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
    auto propertiesInitial = lzt::get_sysman_device_properties(device);
    auto propertiesLater = lzt::get_sysman_device_properties(device);
    EXPECT_EQ(propertiesInitial.core.type, propertiesLater.core.type);
    if (propertiesInitial.core.flags <= ZE_DEVICE_PROPERTY_FLAG_INTEGRATED |
            ZE_DEVICE_PROPERTY_FLAG_SUBDEVICE | ZE_DEVICE_PROPERTY_FLAG_ECC |
            ZE_DEVICE_PROPERTY_FLAG_ONDEMANDPAGING &&
        propertiesInitial.core.flags <= ZE_DEVICE_PROPERTY_FLAG_INTEGRATED |
            ZE_DEVICE_PROPERTY_FLAG_SUBDEVICE | ZE_DEVICE_PROPERTY_FLAG_ECC |
            ZE_DEVICE_PROPERTY_FLAG_ONDEMANDPAGING) {
      EXPECT_EQ(propertiesInitial.core.flags, propertiesLater.core.flags);
      if (propertiesInitial.core.flags & ZE_DEVICE_PROPERTY_FLAG_SUBDEVICE &&
          propertiesLater.core.flags & ZE_DEVICE_PROPERTY_FLAG_SUBDEVICE) {
        EXPECT_EQ(propertiesInitial.core.subdeviceId,
                  propertiesLater.core.subdeviceId);
      }
    } else {
      FAIL();
    }
    EXPECT_TRUE(0 ==
                std::memcmp(propertiesInitial.serialNumber,
                            propertiesLater.serialNumber,
                            get_prop_length(propertiesInitial.serialNumber)));
    EXPECT_TRUE(0 ==
                std::memcmp(propertiesInitial.boardNumber,
                            propertiesLater.boardNumber,
                            get_prop_length(propertiesInitial.boardNumber)));

    EXPECT_TRUE(0 == std::memcmp(propertiesInitial.brandName,
                                 propertiesLater.brandName,
                                 get_prop_length(propertiesInitial.brandName)));

    EXPECT_TRUE(0 == std::memcmp(propertiesInitial.modelName,
                                 propertiesLater.modelName,
                                 get_prop_length(propertiesInitial.modelName)));

    EXPECT_TRUE(0 ==
                std::memcmp(propertiesInitial.vendorName,
                            propertiesLater.vendorName,
                            get_prop_length(propertiesInitial.vendorName)));

    EXPECT_TRUE(0 ==
                std::memcmp(propertiesInitial.driverVersion,
                            propertiesLater.driverVersion,
                            get_prop_length(propertiesInitial.driverVersion)));
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
        EXPECT_GT(process.memSize, 0u);
        EXPECT_LT(process.sharedSize, UINT64_MAX);
        EXPECT_GE(process.engines, 1);
        EXPECT_LE(process.engines, (1 << ZES_ENGINE_TYPE_FLAG_DMA));
      }
    }
  }
}

TEST_F(
    SysmanDeviceTest,
    GivenInvalidComponentCountWhenRetrievingSysmanHandlesThenActualComponentCountIsUpdated) {
  for (auto device : devices) {
    uint32_t actualCount = 0;
    lzt::get_processes_state(device, actualCount);
    uint32_t testCount = actualCount + 1;
    lzt::get_processes_state(device, testCount);
    EXPECT_EQ(testCount, actualCount);
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

} // namespace
