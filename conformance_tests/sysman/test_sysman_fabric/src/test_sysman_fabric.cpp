/*
 *
 * Copyright (C) 2020-2023 Intel Corporation
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

#ifdef USE_ZESINIT
class FabricPortsOperationsZesTest : public lzt::ZesSysmanCtsClass {};
#define FABRICPORT_TEST FabricPortsOperationsZesTest
#else // USE_ZESINIT
class FabricPortsOperationsTest : public lzt::SysmanCtsClass {};
#define FABRICPORT_TEST FabricPortsOperationsTest
#endif // USE_ZESINIT

void validate_fabric_port_speed(zes_fabric_port_speed_t speed) {

  if ((speed.bitRate == -1) || (speed.width == -1)) {

    return;
  }

  EXPECT_LT(speed.bitRate, INT64_MAX);

  EXPECT_LT(speed.width, INT32_MAX);
}

LZT_TEST_F(
    FABRICPORT_TEST,
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

LZT_TEST_F(
    FABRICPORT_TEST,
    GivenComponentCountZeroWhenRetrievingSysmanHandlesThenNotNullFabricPortHandlesAreReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto fabric_port_handles = lzt::get_fabric_port_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    ASSERT_EQ(fabric_port_handles.size(), count);
    for (auto fabric_port_handle : fabric_port_handles) {
      EXPECT_NE(nullptr, fabric_port_handle);
    }
  }
}

LZT_TEST_F(
    FABRICPORT_TEST,
    GivenInvalidComponentCountWhenRetrievingSysmanHandlesThenActualComponentCountIsUpdated) {
  for (auto device : devices) {
    uint32_t actual_count = 0;
    lzt::get_fabric_port_handles(device, actual_count);
    if (actual_count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    uint32_t test_count = actual_count + 1;
    lzt::get_fabric_port_handles(device, test_count);
    EXPECT_EQ(test_count, actual_count);
  }
}

LZT_TEST_F(
    FABRICPORT_TEST,
    GivenValidComponentCountWhenCallingApiTwiceThenSimilarFabricPortHandlesReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto fabric_port_handles_initial =
        lzt::get_fabric_port_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto fabric_port_handle : fabric_port_handles_initial) {
      EXPECT_NE(nullptr, fabric_port_handle);
    }

    count = 0;
    auto fabric_port_handles_later =
        lzt::get_fabric_port_handles(device, count);
    for (auto fabric_port_handle : fabric_port_handles_later) {
      EXPECT_NE(nullptr, fabric_port_handle);
    }
    EXPECT_EQ(fabric_port_handles_initial, fabric_port_handles_later);
  }
}

LZT_TEST_F(
    FABRICPORT_TEST,
    GivenValidFabricPortHandleWhenRetrievingFabricPortPropertiesThenValidPropertiesAreReturned) {
  for (auto device : devices) {
    auto deviceProperties = lzt::get_sysman_device_properties(device);
    uint32_t count = 0;
    auto fabric_port_handles = lzt::get_fabric_port_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto fabric_port_handle : fabric_port_handles) {
      ASSERT_NE(nullptr, fabric_port_handle);
      auto properties = lzt::get_fabric_port_properties(fabric_port_handle);
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

LZT_TEST_F(
    FABRICPORT_TEST,
    GivenValidFabricPortHandleWhenRetrievingFabricPortPropertiesThenExpectSamePropertiesReturnedTwice) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto fabric_port_handles = lzt::get_fabric_port_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto fabric_port_handle : fabric_port_handles) {
      ASSERT_NE(nullptr, fabric_port_handle);
      auto properties_initial =
          lzt::get_fabric_port_properties(fabric_port_handle);
      auto properties_later =
          lzt::get_fabric_port_properties(fabric_port_handle);
      EXPECT_EQ(properties_initial.onSubdevice, properties_later.onSubdevice);
      if (properties_initial.onSubdevice && properties_later.onSubdevice) {
        EXPECT_EQ(properties_initial.subdeviceId, properties_later.subdeviceId);
      }
      if (strcmp(properties_initial.model, properties_later.model) == 0) {
        SUCCEED();
      } else {
        FAIL();
      }
      EXPECT_EQ(strlen(properties_initial.model),
                strlen(properties_later.model));
      EXPECT_EQ(properties_initial.maxRxSpeed.bitRate,
                properties_later.maxRxSpeed.bitRate);
      EXPECT_EQ(properties_initial.maxRxSpeed.width,
                properties_later.maxRxSpeed.width);
      EXPECT_EQ(properties_initial.maxTxSpeed.bitRate,
                properties_later.maxTxSpeed.bitRate);
      EXPECT_EQ(properties_initial.maxTxSpeed.width,
                properties_later.maxTxSpeed.width);
      EXPECT_EQ(properties_initial.portId.fabricId,
                properties_later.portId.fabricId);
      EXPECT_EQ(properties_initial.portId.attachId,
                properties_later.portId.attachId);
      EXPECT_EQ(properties_initial.portId.portNumber,
                properties_later.portId.portNumber);
    }
  }
}

LZT_TEST_F(
    FABRICPORT_TEST,
    GivenValidFabricPortHandleWhenSettingPortConfigThenGetSamePortConfig) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto fabric_port_handles = lzt::get_fabric_port_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto fabric_port_handle : fabric_port_handles) {
      ASSERT_NE(nullptr, fabric_port_handle);
      auto defaultConfig = lzt::get_fabric_port_config(fabric_port_handle);
      zes_fabric_port_config_t setConfig = {};
      // To validate if set_fabric_port_config API is really working, try to
      // toggle config as compared to defaultConfig
      setConfig.beaconing = (defaultConfig.beaconing == true) ? false : true;
      setConfig.enabled = (defaultConfig.enabled == true) ? false : true;
      lzt::set_fabric_port_config(fabric_port_handle, setConfig);
      auto get_config = lzt::get_fabric_port_config(fabric_port_handle);
      EXPECT_EQ(setConfig.beaconing, get_config.beaconing);
      EXPECT_EQ(setConfig.enabled, get_config.enabled);
    }
  }
}

LZT_TEST_F(
    FABRICPORT_TEST,
    GivenValidFabricPortHandleWhenGettingPortStateThenValidStatesAreReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto fabric_port_handles = lzt::get_fabric_port_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto fabric_port_handle : fabric_port_handles) {
      ASSERT_NE(nullptr, fabric_port_handle);
      auto state = lzt::get_fabric_port_state(fabric_port_handle);
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
        auto properties = lzt::get_fabric_port_properties(fabric_port_handle);
        bool loopback =
            (state.remotePortId.fabricId == properties.portId.fabricId) &&
            (state.remotePortId.attachId == properties.portId.attachId) &&
            (state.remotePortId.portNumber == properties.portId.portNumber);
        EXPECT_FALSE(loopback);
      }
      validate_fabric_port_speed(state.rxSpeed);
      validate_fabric_port_speed(state.txSpeed);
    }
  }
}

LZT_TEST_F(
    FABRICPORT_TEST,
    GivenValidFabricPortHandleWhenGettingPortThroughputThenValidThroughputAreReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto fabric_port_handles = lzt::get_fabric_port_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto fabric_port_handle : fabric_port_handles) {
      ASSERT_NE(nullptr, fabric_port_handle);
      auto throughput = lzt::get_fabric_port_throughput(fabric_port_handle);
      EXPECT_LT(throughput.rxCounter, UINT64_MAX);
      EXPECT_LT(throughput.txCounter, UINT64_MAX);
      EXPECT_LT(throughput.timestamp, UINT64_MAX);
    }
  }
}

LZT_TEST_F(FABRICPORT_TEST,
           GivenValidFabricPortHandleWhenGettingPortLinkThenSuccessIsReturned) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto fabric_port_handles = lzt::get_fabric_port_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto fabric_port_handle : fabric_port_handles) {
      ASSERT_NE(nullptr, fabric_port_handle);
      auto fabric_port_link_type =
          lzt::get_fabric_port_link(fabric_port_handle);
      if (fabric_port_link_type.desc[0] == '\0') {
        FAIL();
      } else {
        SUCCEED();
      }
      EXPECT_LE(strlen(fabric_port_link_type.desc),
                ZES_MAX_FABRIC_LINK_TYPE_SIZE);
    }
  }
}

LZT_TEST_F(
    FABRICPORT_TEST,
    GivenValidFabricPortHandleWhenGettingPortLinkTwiceThenSameValueIsReturnedTwice) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto fabric_port_handles = lzt::get_fabric_port_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }

    for (auto fabric_port_handle : fabric_port_handles) {
      ASSERT_NE(nullptr, fabric_port_handle);
      auto fabric_port_link_type_initial =
          lzt::get_fabric_port_link(fabric_port_handle);
      auto fabric_port_link_type_later =
          lzt::get_fabric_port_link(fabric_port_handle);
      if (strcmp(fabric_port_link_type_initial.desc,
                 fabric_port_link_type_later.desc) == 0) {
        SUCCEED();
      } else {
        FAIL();
      }
      EXPECT_EQ(strlen(fabric_port_link_type_initial.desc),
                strlen(fabric_port_link_type_later.desc));
    }
  }
}

} // namespace
