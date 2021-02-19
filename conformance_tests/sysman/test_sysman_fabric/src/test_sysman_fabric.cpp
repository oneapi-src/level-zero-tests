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

class FabricPortsOperationsTest : public lzt::SysmanCtsClass {};

void validate_fabric_port_speed(zes_fabric_port_speed_t speed) {
  EXPECT_LT(speed.bitRate, UINT64_MAX);
  EXPECT_LT(speed.width, UINT32_MAX);
}

TEST_F(
    FabricPortsOperationsTest,
    GivenComponentCountZeroWhenRetrievingSysmanHandlesThenNonZeroCountIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    lzt::get_fabric_port_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }
  }
}

TEST_F(
    FabricPortsOperationsTest,
    GivenComponentCountZeroWhenRetrievingSysmanHandlesThenNotNullFabricPortHandlesAreReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto fabricPortHandles = lzt::get_fabric_port_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

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
    if (actualCount == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

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
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

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
    auto deviceProperties = lzt::get_sysman_device_properties(device);
    uint32_t count = 0;
    auto fabricPortHandles = lzt::get_fabric_port_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto fabricPortHandle : fabricPortHandles) {
      ASSERT_NE(nullptr, fabricPortHandle);
      auto properties = lzt::get_fabric_port_properties(fabricPortHandle);
      if (properties.onSubdevice) {
        EXPECT_LT(properties.subdeviceId, deviceProperties.numSubdevices);
      }
      validate_fabric_port_speed(properties.maxRxSpeed);
      validate_fabric_port_speed(properties.maxTxSpeed);
      // If we have at least one non-null byte, then we Pass
      if (properties.model[0] == '\0') {
        FAIL();
      } else {
        SUCCEED();
      }
      EXPECT_LT(properties.portId.fabricId, UINT32_MAX);
      EXPECT_LT(properties.portId.attachId, UINT32_MAX);
      EXPECT_LT(properties.portId.portNumber, UINT8_MAX);
    }
  }
}

TEST_F(
    FabricPortsOperationsTest,
    GivenValidFabricPortHandleWhenRetrievingFabricPortPropertiesThenExpectSamePropertiesReturnedTwice) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto fabricPortHandles = lzt::get_fabric_port_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto fabricPortHandle : fabricPortHandles) {
      ASSERT_NE(nullptr, fabricPortHandle);
      auto propertiesInitial =
          lzt::get_fabric_port_properties(fabricPortHandle);
      auto propertiesLater = lzt::get_fabric_port_properties(fabricPortHandle);
      EXPECT_EQ(propertiesInitial.onSubdevice, propertiesLater.onSubdevice);
      if (propertiesInitial.onSubdevice && propertiesLater.onSubdevice) {
        EXPECT_EQ(propertiesInitial.subdeviceId, propertiesLater.subdeviceId);
      }
      if (strcmp(propertiesInitial.model, propertiesLater.model) == 0) {
        SUCCEED();
      } else {
        FAIL();
      }
      EXPECT_EQ(strlen(propertiesInitial.model), strlen(propertiesLater.model));
      EXPECT_EQ(propertiesInitial.maxRxSpeed.bitRate,
                propertiesLater.maxRxSpeed.bitRate);
      EXPECT_EQ(propertiesInitial.maxRxSpeed.width,
                propertiesLater.maxRxSpeed.width);
      EXPECT_EQ(propertiesInitial.maxTxSpeed.bitRate,
                propertiesLater.maxTxSpeed.bitRate);
      EXPECT_EQ(propertiesInitial.maxTxSpeed.width,
                propertiesLater.maxTxSpeed.width);
      EXPECT_EQ(propertiesInitial.portId.fabricId,
                propertiesLater.portId.fabricId);
      EXPECT_EQ(propertiesInitial.portId.attachId,
                propertiesLater.portId.attachId);
      EXPECT_EQ(propertiesInitial.portId.portNumber,
                propertiesLater.portId.portNumber);
    }
  }
}

TEST_F(FabricPortsOperationsTest,
       GivenValidFabricPortHandleWhenSettingPortConfigThenGetSamePortConfig) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto fabricPortHandles = lzt::get_fabric_port_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto fabricPortHandle : fabricPortHandles) {
      ASSERT_NE(nullptr, fabricPortHandle);
      auto defaultConfig = lzt::get_fabric_port_config(fabricPortHandle);
      zes_fabric_port_config_t setConfig = {};
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
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto fabricPortHandle : fabricPortHandles) {
      ASSERT_NE(nullptr, fabricPortHandle);
      auto state = lzt::get_fabric_port_state(fabricPortHandle);
      EXPECT_GE(state.status, ZES_FABRIC_PORT_STATUS_UNKNOWN);
      EXPECT_LE(state.status, ZES_FABRIC_PORT_STATUS_DISABLED);
      if (state.status == ZES_FABRIC_PORT_STATUS_DEGRADED) {
        EXPECT_GE(state.qualityIssues,
                  ZES_FABRIC_PORT_QUAL_ISSUE_FLAG_LINK_ERRORS);
        EXPECT_LE(state.qualityIssues,
                  ZES_FABRIC_PORT_QUAL_ISSUE_FLAG_LINK_ERRORS |
                      ZES_FABRIC_PORT_QUAL_ISSUE_FLAG_SPEED);
      } else {
        EXPECT_EQ(state.qualityIssues, 0);
      }
      if (state.status == ZES_FABRIC_PORT_STATUS_FAILED) {
        EXPECT_GE(state.failureReasons,
                  ZES_FABRIC_PORT_QUAL_ISSUE_FLAG_LINK_ERRORS);
        EXPECT_LE(state.failureReasons,
                  ZES_FABRIC_PORT_FAILURE_FLAG_FAILED |
                      ZES_FABRIC_PORT_FAILURE_FLAG_TRAINING_TIMEOUT |
                      ZES_FABRIC_PORT_FAILURE_FLAG_FLAPPING);
      } else {
        EXPECT_EQ(state.failureReasons, 0);
      }
      if (state.status == ZES_FABRIC_PORT_STATUS_HEALTHY ||
          state.status == ZES_FABRIC_PORT_STATUS_DEGRADED ||
          state.status == ZES_FABRIC_PORT_STATUS_FAILED) {
        auto properties = lzt::get_fabric_port_properties(fabricPortHandle);
        EXPECT_EQ(state.remotePortId.fabricId, properties.portId.fabricId);
        EXPECT_EQ(state.remotePortId.attachId, properties.portId.attachId);
        EXPECT_EQ(state.remotePortId.portNumber, properties.portId.portNumber);
      }
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
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto fabricPortHandle : fabricPortHandles) {
      ASSERT_NE(nullptr, fabricPortHandle);
      auto throughput = lzt::get_fabric_port_throughput(fabricPortHandle);
      EXPECT_LT(throughput.rxCounter, UINT64_MAX);
      EXPECT_LT(throughput.txCounter, UINT64_MAX);
      EXPECT_LT(throughput.timestamp, UINT64_MAX);
    }
  }
}

TEST_F(FabricPortsOperationsTest,
       GivenValidFabricPortHandleWhenGettingPortLinkThenSuccessIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto fabricPortHandles = lzt::get_fabric_port_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto fabricPortHandle : fabricPortHandles) {
      ASSERT_NE(nullptr, fabricPortHandle);
      auto fabricPortLinkType = lzt::get_fabric_port_link(fabricPortHandle);
      if (fabricPortLinkType.desc[0] == '\0') {
        FAIL();
      } else {
        SUCCEED();
      }
      EXPECT_LE(strlen(fabricPortLinkType.desc), ZES_MAX_FABRIC_LINK_TYPE_SIZE);
    }
  }
}

TEST_F(
    FabricPortsOperationsTest,
    GivenValidFabricPortHandleWhenGettingPortLinkTwiceThenSameValueIsReturnedTwice) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto fabricPortHandles = lzt::get_fabric_port_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto fabricPortHandle : fabricPortHandles) {
      ASSERT_NE(nullptr, fabricPortHandle);
      auto fabricPortLinkTypeInitial =
          lzt::get_fabric_port_link(fabricPortHandle);
      auto fabricPortLinkTypeLater =
          lzt::get_fabric_port_link(fabricPortHandle);
      if (strcmp(fabricPortLinkTypeInitial.desc,
                 fabricPortLinkTypeLater.desc) == 0) {
        SUCCEED();
      } else {
        FAIL();
      }
      EXPECT_EQ(strlen(fabricPortLinkTypeInitial.desc),
                strlen(fabricPortLinkTypeLater.desc));
    }
  }
}

} // namespace
