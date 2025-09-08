/*
 *
 * Copyright (C) 2022-2023 Intel Corporation
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
class EccModuleZesTest : public lzt::ZesSysmanCtsClass {};
#define ECC_TEST EccModuleZesTest
#else // USE_ZESINIT
class EccModuleTest : public lzt::SysmanCtsClass {};
#define ECC_TEST EccModuleTest
#endif // USE_ZESINIT

LZT_TEST_F(
    ECC_TEST,
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

LZT_TEST_F(ECC_TEST,
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

LZT_TEST_F(
    ECC_TEST,
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

LZT_TEST_F(
    ECC_TEST,
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

LZT_TEST_F(
    ECC_TEST,
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

LZT_TEST_F(
    ECC_TEST,
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

LZT_TEST_F(
    ECC_TEST,
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

LZT_TEST_F(
    ECC_TEST,
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

LZT_TEST_F(
    ECC_TEST,
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
LZT_TEST_F(
    ECC_TEST,
    GivenValidDeviceHandleWhenEccDefaultStateIsQueriedAndSetThenDefaultValuesAreReturned) {
  for (const auto &device : devices) {
    // Step 1: Get the ECC state (with default state extension)
    zes_device_ecc_properties_t originalProps = {};
    zes_device_ecc_default_properties_ext_t extProps = {};
    originalProps.stype = ZES_STRUCTURE_TYPE_DEVICE_ECC_PROPERTIES;
    originalProps.pNext = &extProps;
    extProps.stype = ZES_STRUCTURE_TYPE_DEVICE_ECC_DEFAULT_PROPERTIES_EXT;
    extProps.pNext = nullptr;

    ze_result_t result = zesDeviceGetEccState(device, &originalProps);
    EXPECT_ZE_RESULT_SUCCESS(ZE_RESULT_SUCCESS, result);

    // Step 2: Set ECC state to default
    zes_device_ecc_desc_t setDesc = {};
    setDesc.stype = ZES_STRUCTURE_TYPE_DEVICE_ECC_DESC;
    setDesc.pNext = nullptr;
    setDesc.state = extProps.defaultState;

    zes_device_ecc_properties_t newProps = {};
    newProps.stype = ZES_STRUCTURE_TYPE_DEVICE_ECC_PROPERTIES;
    newProps.pNext = nullptr;

    result = zesDeviceSetEccState(device, &setDesc, &newProps);
    EXPECT_EQ(ZE_RESULT_SUCCESS, result);

    // Step 3: Re-check ECC state
    zes_device_ecc_properties_t checkProps = {};
    checkProps.stype = ZES_STRUCTURE_TYPE_DEVICE_ECC_PROPERTIES;
    checkProps.pNext = nullptr;

    result = zesDeviceGetEccState(device, &checkProps);
    EXPECT_EQ(ZE_RESULT_SUCCESS, result);
    EXPECT_EQ(checkProps.pendingState, extProps.defaultState);

    // Step 4: Restore original pending ECC state if it changed
    if (checkProps.pendingState != originalProps.pendingState) {
      zes_device_ecc_desc_t restoreDesc = {};
      restoreDesc.stype = ZES_STRUCTURE_TYPE_DEVICE_ECC_DESC;
      restoreDesc.pNext = nullptr;
      restoreDesc.state = originalProps.pendingState;

      zes_device_ecc_properties_t restoredProps = {};
      restoredProps.stype = ZES_STRUCTURE_TYPE_DEVICE_ECC_PROPERTIES;
      restoredProps.pNext = nullptr;

      result = zesDeviceSetEccState(device, &restoreDesc, &restoredProps);
      EXPECT_EQ(ZE_RESULT_SUCCESS, result);
      EXPECT_EQ(restoredProps.pendingState,
                               originalProps.pendingState);
    }
  }
}
} // namespace
