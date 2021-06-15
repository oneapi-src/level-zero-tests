/*
 *
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "test_harness/test_harness.hpp"

#include <level_zero/ze_api.h>
#include "utils/utils.hpp"

namespace lzt = level_zero_tests;

namespace level_zero_tests {

zet_device_debug_properties_t get_debug_properties(ze_device_handle_t device) {

  zet_device_debug_properties_t properties = {};
  properties.stype = ZET_STRUCTURE_TYPE_DEVICE_DEBUG_PROPERTIES;

  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetDeviceGetDebugProperties(device, &properties));

  return properties;
}

zet_debug_session_handle_t
debug_attach(const ze_device_handle_t &device,
             const zet_debug_config_t &debug_config) {
  zet_debug_session_handle_t debug_session = {};

  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetDebugAttach(device, &debug_config, &debug_session));
  return debug_session;
}

void debug_detach(const zet_debug_session_handle_t &debug_session) {
  EXPECT_EQ(ZE_RESULT_SUCCESS, zetDebugDetach(debug_session));
}

} // namespace level_zero_tests
