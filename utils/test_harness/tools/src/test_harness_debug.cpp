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

zet_debug_event_t
debug_read_event(const zet_debug_session_handle_t &debug_session) {

  zet_debug_event_t debug_event = {};

  auto timeout = std::numeric_limits<uint64_t>::max();
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetDebugReadEvent(debug_session, timeout, &debug_event));

  return debug_event;
}

ze_result_t debug_read_event(const zet_debug_session_handle_t &debug_session,
                             zet_debug_event_t &debugEvent, uint64_t timeout,
                             bool allowTimeout) {

  zet_debug_event_t event = {};

  ze_result_t result = zetDebugReadEvent(debug_session, timeout, &event);

  // Expect that timeout expired if not successful
  if (allowTimeout && ZE_RESULT_SUCCESS != result) {
    LOG_INFO << "[Debugger]  zetDebugReadEvent timed out";
    EXPECT_EQ(ZE_RESULT_NOT_READY, result);
  } else {
    EXPECT_EQ(ZE_RESULT_SUCCESS, result);
  }

  debugEvent = event;
  return result;
}

void debug_ack_event(const zet_debug_session_handle_t &debug_session,
                     const zet_debug_event_t *debug_event) {
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetDebugAcknowledgeEvent(debug_session, debug_event));
}

void debug_interrupt(const zet_debug_session_handle_t &debug_session,
                     const ze_device_thread_t &device_thread) {

  EXPECT_EQ(ZE_RESULT_SUCCESS, zetDebugInterrupt(debug_session, device_thread));
}

void debug_resume(const zet_debug_session_handle_t &debug_session,
                  const ze_device_thread_t &device_thread) {

  EXPECT_EQ(ZE_RESULT_SUCCESS, zetDebugResume(debug_session, device_thread));
}

void debug_read_memory(const zet_debug_session_handle_t &debug_session,
                       const ze_device_thread_t &device_thread,
                       const zet_debug_memory_space_desc_t &desc, size_t size,
                       void *buffer) {
  EXPECT_EQ(ZE_RESULT_SUCCESS, zetDebugReadMemory(debug_session, device_thread,
                                                  &desc, size, buffer));
}

void debug_write_memory(const zet_debug_session_handle_t &debug_session,
                        const ze_device_thread_t &device_thread,
                        const zet_debug_memory_space_desc_t &desc, size_t size,
                        const void *buffer) {

  EXPECT_EQ(ZE_RESULT_SUCCESS, zetDebugWriteMemory(debug_session, device_thread,
                                                   &desc, size, buffer));
}

uint32_t get_register_set_properties_count(const ze_device_handle_t &device) {
  uint32_t count = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetDebugGetRegisterSetProperties(device, &count, nullptr));

  return count;
}

std::vector<zet_debug_regset_properties_t>
get_register_set_properties(const ze_device_handle_t &device) {

  auto count = get_register_set_properties_count(device);
  std::vector<zet_debug_regset_properties_t> properties(
      count, {ZET_STRUCTURE_TYPE_DEBUG_REGSET_PROPERTIES});

  auto device_initial = device;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zetDebugGetRegisterSetProperties(
                                   device, &count, properties.data()));
  EXPECT_EQ(device, device_initial);
  return properties;
}

} // namespace level_zero_tests
