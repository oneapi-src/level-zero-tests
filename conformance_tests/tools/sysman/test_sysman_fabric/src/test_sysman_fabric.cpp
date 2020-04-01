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

class FabricPortsOperationsTest : public lzt::SysmanCtsClass {};

void validate_fabric_port_qual_stab_issues(int val) {
  switch (val) {
  case 0:
    SUCCEED();
    break;
  case ZE_BIT(0):
    SUCCEED();
    break;
  case ZE_BIT(1):
    SUCCEED();
    break;
  case ZE_BIT(2):
    SUCCEED();
    break;
  default:
    FAIL();
  }
}

void validate_fabric_port_speed(zet_fabric_port_speed_t speed) {
  EXPECT_LT(speed.bitRate, UINT64_MAX);
  EXPECT_LT(speed.width, UINT32_MAX);
  EXPECT_LT(speed.maxBandwidth, UINT64_MAX);
}

uint32_t get_model_length(int8_t model[ZET_MAX_FABRIC_PORT_MODEL_SIZE]) {
  uint32_t length = 0;
  for (int i = 0; i < ZET_MAX_FABRIC_PORT_MODEL_SIZE; i++) {
    if (model[i] == '\0') {
      break;
    } else {
      length += 1;
    }
  }

  return length;
}

TEST_F(
    FabricPortsOperationsTest,
    GivenComponentCountZeroWhenRetrievingSysmanHandlesThenNonZeroCountIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    lzt::get_fabric_port_handles(device, count);
  }
}

TEST_F(
    FabricPortsOperationsTest,
    GivenComponentCountZeroWhenRetrievingSysmanHandlesThenNotNullFabricPortHandlesAreReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto fabricPortHandles = lzt::get_fabric_port_handles(device, count);
    ASSERT_EQ(fabricPortHandles.size(), count);
    for (auto fabricPortHandle : fabricPortHandles) {
      EXPECT_NE(nullptr, fabricPortHandle);
    }
  }
}

TEST_F(
    FabricPortsOperationsTest,
    GivenInvalidComponentCountWhenRetrievingSysmanHandlesThenActualComponentCountIsUpdated) {
  for (auto device : devices) {
    uint32_t actualCount = 0;
    lzt::get_fabric_port_handles(device, actualCount);
    uint32_t testCount = actualCount + 1;
    lzt::get_fabric_port_handles(device, testCount);
    EXPECT_EQ(testCount, actualCount);
  }
}

TEST_F(
    FabricPortsOperationsTest,
    GivenValidComponentCountWhenCallingApiTwiceThenSimilarFabricPortHandlesReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto fabricPortHandlesInitial = lzt::get_fabric_port_handles(device, count);
    for (auto fabricPortHandle : fabricPortHandlesInitial) {
      EXPECT_NE(nullptr, fabricPortHandle);
    }

    count = 0;
    auto fabricPortHandlesLater = lzt::get_fabric_port_handles(device, count);
    for (auto fabricPortHandle : fabricPortHandlesLater) {
      EXPECT_NE(nullptr, fabricPortHandle);
    }
    EXPECT_EQ(fabricPortHandlesInitial, fabricPortHandlesLater);
  }
}

TEST_F(
    FabricPortsOperationsTest,
    GivenValidFabricPortHandleWhenRetrievingFabricPortPropertiesThenValidPropertiesAreReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto fabricPortHandles = lzt::get_fabric_port_handles(device, count);
    for (auto fabricPortHandle : fabricPortHandles) {
      ASSERT_NE(nullptr, fabricPortHandle);
      auto properties = lzt::get_fabric_port_properties(fabricPortHandle);
      if (properties.onSubdevice == true) {
        EXPECT_LT(properties.subdeviceId, UINT32_MAX);
      }
      validate_fabric_port_speed(properties.maxRxSpeed);
      validate_fabric_port_speed(properties.maxTxSpeed);
      // If we have at least one non-null byte, then we Pass
      if (properties.model[0] == '\0') {
        FAIL();
      } else {
        SUCCEED();
      }
      int8_t val;
      int32_t i;
      for (i = 0; i < ZET_MAX_FABRIC_PORT_UUID_SIZE; i++) {
        val = properties.portUuid.id[i];
        if (val != 0) {
          // Check for at least one non-zero byte in uuid
          SUCCEED();
          break;
        }
      }
      if (i == ZET_MAX_FABRIC_PORT_UUID_SIZE) {
        // All bytes in uuid are zero. Fail
        FAIL();
      }
    }
  }
}

TEST_F(
    FabricPortsOperationsTest,
    GivenValidFabricPortHandleWhenRetrievingFabricPortPropertiesThenExpectSamePropertiesReturnedTwice) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto fabricPortHandles = lzt::get_fabric_port_handles(device, count);
    for (auto fabricPortHandle : fabricPortHandles) {
      ASSERT_NE(nullptr, fabricPortHandle);
      auto propertiesInitial =
          lzt::get_fabric_port_properties(fabricPortHandle);
      auto propertiesLater = lzt::get_fabric_port_properties(fabricPortHandle);
      EXPECT_EQ(propertiesInitial.onSubdevice, propertiesLater.onSubdevice);
      if (propertiesInitial.onSubdevice == true &&
          propertiesLater.onSubdevice == true) {
        EXPECT_EQ(propertiesInitial.subdeviceId, propertiesLater.subdeviceId);
      }
      EXPECT_EQ(get_model_length(propertiesInitial.model),
                get_model_length(propertiesLater.model));
      EXPECT_EQ(propertiesInitial.maxRxSpeed.bitRate,
                propertiesLater.maxRxSpeed.bitRate);
      EXPECT_EQ(propertiesInitial.maxRxSpeed.width,
                propertiesLater.maxRxSpeed.width);
      EXPECT_EQ(propertiesInitial.maxRxSpeed.maxBandwidth,
                propertiesLater.maxRxSpeed.maxBandwidth);
      EXPECT_EQ(propertiesInitial.maxTxSpeed.bitRate,
                propertiesLater.maxTxSpeed.bitRate);
      EXPECT_EQ(propertiesInitial.maxTxSpeed.width,
                propertiesLater.maxTxSpeed.width);
      EXPECT_EQ(propertiesInitial.maxTxSpeed.maxBandwidth,
                propertiesLater.maxTxSpeed.maxBandwidth);
      int i;
      for (i = 0; i < ZE_MAX_DEVICE_UUID_SIZE; i++) {
        if (propertiesInitial.portUuid.id[i] !=
            propertiesLater.portUuid.id[i]) {
          FAIL();
          break;
        }
      }
      if (i == ZE_MAX_DEVICE_UUID_SIZE) {
        SUCCEED();
      }
    }
  }
}

TEST_F(FabricPortsOperationsTest,
       GivenValidFabricPortHandleWhenSettingPortConfigThenGetSamePortConfig) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto fabricPortHandles = lzt::get_fabric_port_handles(device, count);
    for (auto fabricPortHandle : fabricPortHandles) {
      ASSERT_NE(nullptr, fabricPortHandle);
      auto defaultConfig = lzt::get_fabric_port_config(fabricPortHandle);
      zet_fabric_port_config_t setConfig;
      // To validate if set_fabric_port_config API is really working, try to
      // toggle config as compared to defaultConfig
      setConfig.beaconing = (defaultConfig.beaconing == true) ? false : true;
      setConfig.enabled = (defaultConfig.enabled == true) ? false : true;
      lzt::set_fabric_port_config(fabricPortHandle, setConfig);
      auto getConfig = lzt::get_fabric_port_config(fabricPortHandle);
      EXPECT_EQ(setConfig.beaconing, getConfig.beaconing);
      EXPECT_EQ(setConfig.enabled, getConfig.enabled);
    }
  }
}

TEST_F(
    FabricPortsOperationsTest,
    GivenValidFabricPortHandleWhenGettingPortStateThenValidStatesAreReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto fabricPortHandles = lzt::get_fabric_port_handles(device, count);
    for (auto fabricPortHandle : fabricPortHandles) {
      ASSERT_NE(nullptr, fabricPortHandle);
      auto state = lzt::get_fabric_port_state(fabricPortHandle);
      EXPECT_GE(state.status, ZET_FABRIC_PORT_STATUS_GREEN);
      EXPECT_LE(state.status, ZET_FABRIC_PORT_STATUS_BLACK);
      validate_fabric_port_qual_stab_issues(state.qualityIssues);
      validate_fabric_port_qual_stab_issues(state.stabilityIssues);
      validate_fabric_port_speed(state.rxSpeed);
      validate_fabric_port_speed(state.txSpeed);
    }
  }
}

TEST_F(
    FabricPortsOperationsTest,
    GivenValidFabricPortHandleWhenGettingPortThroughputThenValidThroughputAreReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto fabricPortHandles = lzt::get_fabric_port_handles(device, count);
    for (auto fabricPortHandle : fabricPortHandles) {
      ASSERT_NE(nullptr, fabricPortHandle);
      auto throughput = lzt::get_fabric_port_throughput(fabricPortHandle);
      EXPECT_LT(throughput.rxCounter, UINT64_MAX);
      EXPECT_LT(throughput.txCounter, UINT64_MAX);
      EXPECT_LT(throughput.rxMaxBandwidth, UINT64_MAX);
      EXPECT_LT(throughput.txMaxBandwidth, UINT64_MAX);
    }
  }
}

} // namespace
