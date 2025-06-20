/*
 *
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "test_harness/test_harness.hpp"

#include <level_zero/zes_api.h>
#include "utils/utils.hpp"

namespace lzt = level_zero_tests;

namespace level_zero_tests {

ze_bool_t get_ecc_available(zes_device_handle_t device) {
  ze_bool_t pAvailable;
  EXPECT_ZE_RESULT_SUCCESS(zesDeviceEccAvailable(device, &pAvailable));
  return pAvailable;
}

ze_bool_t get_ecc_configurable(zes_device_handle_t device) {
  ze_bool_t pConfigurable;
  EXPECT_ZE_RESULT_SUCCESS(zesDeviceEccConfigurable(device, &pConfigurable));
  return pConfigurable;
}

zes_device_ecc_properties_t get_ecc_state(zes_device_handle_t device) {
  zes_device_ecc_properties_t pState = {
      ZES_STRUCTURE_TYPE_DEVICE_ECC_PROPERTIES, nullptr};
  EXPECT_ZE_RESULT_SUCCESS(zesDeviceGetEccState(device, &pState));
  return pState;
}

zes_device_ecc_properties_t set_ecc_state(zes_device_handle_t device,
                                          zes_device_ecc_desc_t &newState) {
  zes_device_ecc_properties_t pState = {
      ZES_STRUCTURE_TYPE_DEVICE_ECC_PROPERTIES, nullptr};
  EXPECT_ZE_RESULT_SUCCESS(zesDeviceSetEccState(device, &newState, &pState));
  return pState;
}

} // namespace level_zero_tests
