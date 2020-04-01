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

zet_sysman_event_handle_t get_event_handle(zet_device_handle_t device);

zet_event_config_t get_event_config(zet_sysman_event_handle_t hEvent);

void set_event_config(zet_sysman_event_handle_t hEvent,
                      zet_event_config_t config);

ze_result_t get_event_state(zet_sysman_event_handle_t hEvent, ze_bool_t clear,
                            uint32_t &events);

ze_result_t listen_event(ze_driver_handle_t hDriver, uint32_t timeout,
                         uint32_t count, zet_sysman_event_handle_t *phEvents,
                         uint32_t *pEvents);

} // namespace level_zero_tests

#endif
