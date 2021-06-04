/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "test_harness/test_harness.hpp"

#include <level_zero/ze_api.h>
#include <level_zero/zes_api.h>
#include "utils/utils.hpp"

namespace lzt = level_zero_tests;

namespace level_zero_tests {

void register_event(zes_device_handle_t device, zes_event_type_flags_t events) {
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesDeviceEventRegister(device, events));
}

ze_result_t listen_event(ze_driver_handle_t hDriver, uint32_t timeout,
                         uint32_t count, zes_device_handle_t *devices,
                         uint32_t *numDeviceEvents,
                         zes_event_type_flags_t *events) {
  ze_result_t result = zesDriverEventListen(hDriver, timeout, count, devices,
                                            numDeviceEvents, events);
  EXPECT_EQ(ZE_RESULT_SUCCESS, result);
  return result;
}

ze_result_t listen_eventEx(ze_driver_handle_t hDriver, uint64_t timeoutEx,
                           uint32_t count, zes_device_handle_t *devices,
                           uint32_t *numDeviceEvents,
                           zes_event_type_flags_t *events) {
  ze_result_t result = zesDriverEventListenEx(hDriver, timeoutEx, count,
                                              devices, numDeviceEvents, events);
  EXPECT_EQ(ZE_RESULT_SUCCESS, result);
  return result;
}

} // namespace level_zero_tests
