/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "test_harness/test_harness.hpp"

#include <level_zero/ze_api.h>
#include "utils/utils.hpp"

namespace lzt = level_zero_tests;

namespace level_zero_tests {

zet_sysman_event_handle_t get_event_handle(zet_device_handle_t device) {
  zet_sysman_event_handle_t hEvent;
  zet_sysman_handle_t hSysman = lzt::get_sysman_handle(device);
  EXPECT_EQ(ZE_RESULT_SUCCESS, zetSysmanEventGet(hSysman, &hEvent));
  return hEvent;
}

zet_event_config_t get_event_config(zet_sysman_event_handle_t hEvent) {
  zet_event_config_t config;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zetSysmanEventGetConfig(hEvent, &config));
  return config;
}

void set_event_config(zet_sysman_event_handle_t hEvent,
                      zet_event_config_t config) {
  EXPECT_EQ(ZE_RESULT_SUCCESS, zetSysmanEventSetConfig(hEvent, &config));
}

ze_result_t get_event_state(zet_sysman_event_handle_t hEvent, ze_bool_t clear,
                            uint32_t &events) {
  ze_result_t result = zetSysmanEventGetState(hEvent, clear, &events);
  EXPECT_EQ(ZE_RESULT_SUCCESS, result);
  return result;
}

ze_result_t listen_event(ze_driver_handle_t hDriver, uint32_t timeout,
                         uint32_t count, zet_sysman_event_handle_t *phEvents,
                         uint32_t *pEvents) {
  ze_result_t result =
      zetSysmanEventListen(hDriver, timeout, count, phEvents, pEvents);
  EXPECT_EQ(ZE_RESULT_SUCCESS, result);
  return result;
}

} // namespace level_zero_tests