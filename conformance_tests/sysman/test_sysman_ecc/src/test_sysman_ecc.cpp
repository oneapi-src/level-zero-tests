/*
 *
 * Copyright (C) 2022 Intel Corporation
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

class EccModuleTest : public lzt::SysmanCtsClass {};

TEST_F(
    EccModuleTest,
    GivenValidDeviceHandleIfEccIsConfigurableThenExpectEccShouldBeAvailable) {
  for (const auto &device : devices) {
    auto configurable = lzt::get_ecc_configurable(device);
    if (configurable == static_cast<ze_bool_t>(true)) {
      auto available = lzt::get_ecc_available(device);
      EXPECT_EQ(available, static_cast<ze_bool_t>(true));
    } else {
      LOG_INFO << "Ecc is not configurable";
    }
  }
}

TEST_F(EccModuleTest,
       GivenValidDeviceHandleIfEccIsAvailableThenExpectValidEccProperties) {
  for (const auto &device : devices) {
    auto available = lzt::get_ecc_available(device);
    if (available == static_cast<ze_bool_t>(true)) {
      auto state = lzt::get_ecc_state(device);
      EXPECT_GE(state.currentState, ZES_DEVICE_ECC_STATE_UNAVAILABLE);
      EXPECT_LE(state.currentState, ZES_DEVICE_ECC_STATE_DISABLED);
      EXPECT_GE(state.pendingState, ZES_DEVICE_ECC_STATE_UNAVAILABLE);
      EXPECT_LE(state.pendingState, ZES_DEVICE_ECC_STATE_DISABLED);
      EXPECT_GE(state.pendingAction, ZES_DEVICE_ACTION_NONE);
      EXPECT_LE(state.pendingAction, ZES_DEVICE_ACTION_COLD_SYSTEM_REBOOT);
    } else {
      LOG_INFO << "Ecc is not available";
    }
  }
}

TEST_F(EccModuleTest,
       GivenValidDeviceHandleIfEccIsAvailableThenExpectSameEccPropertiesTwice) {
  for (const auto &device : devices) {
    auto available = lzt::get_ecc_available(device);
    if (available == static_cast<ze_bool_t>(true)) {
      auto stateInitial = lzt::get_ecc_state(device);
      auto stateLater = lzt::get_ecc_state(device);
      EXPECT_EQ(stateInitial.currentState, stateLater.currentState);
      EXPECT_EQ(stateInitial.pendingState, stateLater.pendingState);
      EXPECT_EQ(stateInitial.pendingAction, stateLater.pendingAction);
    } else {
      LOG_INFO << "Ecc is not available";
    }
  }
}

TEST_F(
    EccModuleTest,
    GivenValidDeviceHandleIfEccIsConfigurableSetTheEccStateSameAsCurrentStateThenExpectNoPendingActionIsRequired) {
  for (const auto &device : devices) {
    auto configurable = lzt::get_ecc_configurable(device);
    if (configurable == static_cast<ze_bool_t>(true)) {
      auto stateInitial = lzt::get_ecc_state(device);
      zes_device_ecc_desc_t newState = {ZES_STRUCTURE_TYPE_DEVICE_ECC_DESC,
                                        nullptr};
      newState.state = stateInitial.currentState;
      auto stateLater = lzt::set_ecc_state(device, newState);
      EXPECT_EQ(stateInitial.currentState, stateLater.currentState);
      EXPECT_EQ(stateLater.pendingAction, ZES_DEVICE_ACTION_NONE);
    } else {
      LOG_INFO << "Ecc is not configurable";
    }
  }
}

TEST_F(
    EccModuleTest,
    GivenValidDeviceHandleIfEccIsConfigurableSetTheEccStateDifferentFromCurrentStateThenExpectPendingActionIsRequired) {
  for (const auto &device : devices) {
    auto configurable = lzt::get_ecc_configurable(device);
    if (configurable == static_cast<ze_bool_t>(true)) {
      auto stateInitial = lzt::get_ecc_state(device);
      zes_device_ecc_desc_t newState = {ZES_STRUCTURE_TYPE_DEVICE_ECC_DESC,
                                        nullptr};
      if (stateInitial.currentState == ZES_DEVICE_ECC_STATE_ENABLED) {
        newState.state = ZES_DEVICE_ECC_STATE_DISABLED;
      } else {
        newState.state = ZES_DEVICE_ECC_STATE_ENABLED;
      }
      auto stateLater = lzt::set_ecc_state(device, newState);
      EXPECT_EQ(stateInitial.currentState, stateLater.currentState);
      EXPECT_EQ(stateLater.pendingState, newState.state);
      EXPECT_NE(stateLater.pendingAction, ZES_DEVICE_ACTION_NONE);
    } else {
      LOG_INFO << "Ecc is not configurable";
    }
  }
}

TEST_F(
    EccModuleTest,
    GivenValidDeviceHandleIfEccIsNotAvailableThenGetStateIsInvokedThenExpectEccIsNotAvailable) {
  for (const auto &device : devices) {
    auto available = lzt::get_ecc_available(device);
    if (available == static_cast<ze_bool_t>(false)) {
      zes_device_ecc_properties_t getState = {
          ZES_STRUCTURE_TYPE_DEVICE_ECC_PROPERTIES, nullptr};
      EXPECT_EQ(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE,
                zesDeviceGetEccState(device, &getState));
    }
  }
}

TEST_F(
    EccModuleTest,
    GivenValidDeviceHandleIfEccIsAvailableButNotConfigurableThenGetStateIsInvokedThenExpectEccUnSupportedFeature) {
  for (const auto &device : devices) {
    auto available = lzt::get_ecc_available(device);
    if (available == static_cast<ze_bool_t>(true)) {
      auto configurable = lzt::get_ecc_configurable(device);
      if (configurable == static_cast<ze_bool_t>(false)) {
        zes_device_ecc_properties_t getState = {
            ZES_STRUCTURE_TYPE_DEVICE_ECC_PROPERTIES, nullptr};
        EXPECT_EQ(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE,
                  zesDeviceGetEccState(device, &getState));
      }
    }
  }
}

TEST_F(
    EccModuleTest,
    GivenValidDeviceHandleIfEccIsNotAvailableThenSetStateIsInvokedThenExpectEccIsNotAvailable) {
  for (const auto &device : devices) {
    auto available = lzt::get_ecc_available(device);
    if (available == static_cast<ze_bool_t>(false)) {
      zes_device_ecc_properties_t state = {
          ZES_STRUCTURE_TYPE_DEVICE_ECC_PROPERTIES, nullptr};
      zes_device_ecc_desc_t newState = {ZES_STRUCTURE_TYPE_DEVICE_ECC_DESC,
                                        nullptr};
      newState.state = ZES_DEVICE_ECC_STATE_ENABLED;
      EXPECT_EQ(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE,
                zesDeviceSetEccState(device, &newState, &state));
    }
  }
}

TEST_F(
    EccModuleTest,
    GivenValidDeviceHandleIfEccIsAvailableButNotConfigurableThenSetStateIsInvokedThenExpectEccUnSupportedFeature) {
  for (const auto &device : devices) {
    auto available = lzt::get_ecc_available(device);
    if (available == static_cast<ze_bool_t>(false)) {
      auto configurable = lzt::get_ecc_configurable(device);
      if (configurable == static_cast<ze_bool_t>(false)) {
        zes_device_ecc_properties_t state = {
            ZES_STRUCTURE_TYPE_DEVICE_ECC_PROPERTIES, nullptr};
        zes_device_ecc_desc_t newState = {ZES_STRUCTURE_TYPE_DEVICE_ECC_DESC,
                                          nullptr};
        newState.state = ZES_DEVICE_ECC_STATE_ENABLED;
        EXPECT_EQ(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE,
                  zesDeviceSetEccState(device, &newState, &state));
      }
    }
  }
}

} // namespace
