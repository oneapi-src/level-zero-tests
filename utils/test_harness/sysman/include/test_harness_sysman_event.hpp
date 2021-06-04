/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef level_zero_tests_ZE_TEST_HARNESS_SYSMAN_EVENT_HPP
#define level_zero_tests_ZE_TEST_HARNESS_SYSMAN_EVENT_HPP

#include <level_zero/ze_api.h>
#include "gtest/gtest.h"
#include "test_harness_sysman_init.hpp"

namespace level_zero_tests {

void register_event(zes_device_handle_t device, zes_event_type_flags_t events);

ze_result_t listen_event(ze_driver_handle_t hDriver, uint32_t timeout,
                         uint32_t count, zes_device_handle_t *devices,
                         uint32_t *numDeviceEvents,
                         zes_event_type_flags_t *events);

ze_result_t listen_eventEx(ze_driver_handle_t hDriver, uint64_t timeoutEx,
                           uint32_t count, zes_device_handle_t *devices,
                           uint32_t *numDeviceEvents,
                           zes_event_type_flags_t *events);

} // namespace level_zero_tests

#endif
