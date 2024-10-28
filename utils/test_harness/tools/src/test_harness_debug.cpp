/*
 *
 * Copyright (C) 2021-2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <thread>

#include "test_harness/test_harness.hpp"
#include <level_zero/ze_api.h>
#include "utils/utils.hpp"
#include "test_harness/zet_intel_gpu_debug.h"

namespace lzt = level_zero_tests;

namespace level_zero_tests {

std::map<zet_debug_event_type_t, std::string> debuggerEventTypeString = {
    {ZET_DEBUG_EVENT_TYPE_INVALID, "ZET_DEBUG_EVENT_TYPE_INVALID"},
    {ZET_DEBUG_EVENT_TYPE_DETACHED, "ZET_DEBUG_EVENT_TYPE_DETACHED"},
    {ZET_DEBUG_EVENT_TYPE_PROCESS_ENTRY, "ZET_DEBUG_EVENT_TYPE_PROCESS_ENTRY"},
    {ZET_DEBUG_EVENT_TYPE_PROCESS_EXIT, "ZET_DEBUG_EVENT_TYPE_PROCESS_EXIT"},
    {ZET_DEBUG_EVENT_TYPE_MODULE_LOAD, "ZET_DEBUG_EVENT_TYPE_MODULE_LOAD"},
    {ZET_DEBUG_EVENT_TYPE_MODULE_UNLOAD, "ZET_DEBUG_EVENT_TYPE_MODULE_UNLOAD"},
    {ZET_DEBUG_EVENT_TYPE_THREAD_STOPPED,
     "ZET_DEBUG_EVENT_TYPE_THREAD_STOPPED"},
    {ZET_DEBUG_EVENT_TYPE_THREAD_UNAVAILABLE,
     "ZET_DEBUG_EVENT_TYPE_THREAD_UNAVAILABLE"},
    {ZET_DEBUG_EVENT_TYPE_PAGE_FAULT, "ZET_DEBUG_EVENT_TYPE_PAGE_FAULT"},
    {ZET_DEBUG_EVENT_TYPE_FORCE_UINT32, "ZET_DEBUG_EVENT_TYPE_FORCE_UINT32 "}};

zet_device_debug_properties_t get_debug_properties(ze_device_handle_t device) {

  zet_device_debug_properties_t properties = {};
  properties.stype = ZET_STRUCTURE_TYPE_DEVICE_DEBUG_PROPERTIES;

  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetDeviceGetDebugProperties(device, &properties));

  return properties;
}

std::unordered_map<zet_debug_session_handle_t, bool> sessionsAttachStatus;

zet_debug_session_handle_t debug_attach(const ze_device_handle_t &device,
                                        const zet_debug_config_t &debug_config,
                                        uint32_t timeout) {

  zet_debug_session_handle_t debug_session = {};

  ze_result_t result;
  std::chrono::high_resolution_clock::time_point startTime =
      std::chrono::high_resolution_clock::now();
  std::chrono::high_resolution_clock::time_point loopTime =
      std::chrono::high_resolution_clock::now();
  do {
    result = zetDebugAttach(device, &debug_config, &debug_session);

    if (result == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE ||
        result == ZE_RESULT_ERROR_DEVICE_LOST) {
      break;
    }

    if (result != ZE_RESULT_SUCCESS) {
      LOG_DEBUG << "Attach failed, sleeping and retrying...";
      std::this_thread::sleep_for(std::chrono::milliseconds(350));
      loopTime = std::chrono::high_resolution_clock::now();
    }

  } while (result != ZE_RESULT_SUCCESS &&
           (std::chrono::duration_cast<std::chrono::duration<double>>(loopTime -
                                                                      startTime)
                .count() < timeout));

  EXPECT_EQ(ZE_RESULT_SUCCESS, result);
  if (ZE_RESULT_SUCCESS == result) {
    sessionsAttachStatus[debug_session] = true;
  }

  return debug_session;
}

void debug_detach(const zet_debug_session_handle_t &debug_session) {

  ze_result_t result = zetDebugDetach(debug_session);
  EXPECT_EQ(ZE_RESULT_SUCCESS, result);

  if (ZE_RESULT_SUCCESS == result) {
    sessionsAttachStatus[debug_session] = false;
  }
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
    LOG_INFO << "[Debugger] zetDebugReadEvent timed out";
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

void printRegSetProperties(zet_debug_regset_properties_t regSet) {

  LOG_DEBUG << "[Debugger] Reg Set Type: " << regSet.type;
  LOG_DEBUG << "[Debugger] Reg Set Version: " << regSet.version;
  LOG_DEBUG << "[Debugger] Reg Set GeneralFlags: " << regSet.generalFlags;
  LOG_DEBUG << "[Debugger] Reg Set deviceFlags: " << regSet.deviceFlags;
  LOG_DEBUG << "[Debugger] Reg Set count: " << regSet.count;
  LOG_DEBUG << "[Debugger] Reg Set bit size: " << regSet.bitSize;
  LOG_DEBUG << "[Debugger] Reg Set byte size: " << regSet.byteSize;
  return;
}

bool get_register_set_props(ze_device_handle_t device,
                            zet_debug_regset_type_intel_gpu_t type,
                            zet_debug_regset_properties_t &reg) {
  uint32_t nRegSets = 0;
  zetDebugGetRegisterSetProperties(device, &nRegSets, nullptr);
  zet_debug_regset_properties_t *pRegSets =
      new zet_debug_regset_properties_t[nRegSets];
  for (int i = 0; i < nRegSets; i++) {
    pRegSets[i] = {ZET_STRUCTURE_TYPE_DEBUG_REGSET_PROPERTIES, nullptr};
  }
  zetDebugGetRegisterSetProperties(device, &nRegSets, pRegSets);

  bool found = false;
  for (int i = 0; i < nRegSets; i++) {
    if (pRegSets[i].type == type) {
      printRegSetProperties(pRegSets[i]);
      reg = pRegSets[i];
      found = true;
      break;
    }
  }

  delete[] pRegSets;
  return found;
}

void clear_exceptions(const ze_device_handle_t &device,
                    const zet_debug_session_handle_t &debug_session,
                  const ze_device_thread_t &device_thread) {
  size_t reg_size_in_bytes = 0;

  zet_debug_regset_properties_t cr_reg_prop;
  ASSERT_TRUE(get_register_set_props(device, ZET_DEBUG_REGSET_TYPE_CR_INTEL_GPU,
                                     cr_reg_prop));

  reg_size_in_bytes = cr_reg_prop.count * cr_reg_prop.byteSize;
  uint8_t *cr_values = new uint8_t[reg_size_in_bytes];
  uint32_t *uint32_values = (uint32_t *)cr_values;

  ASSERT_EQ(zetDebugReadRegisters(debug_session, device_thread,
                                  ZET_DEBUG_REGSET_TYPE_CR_INTEL_GPU, 0,
                                  cr_reg_prop.count, cr_values),
            ZE_RESULT_SUCCESS);

  uint32_values[1] &=
      ~((1 << 26) | (1 << 30));
  ASSERT_EQ(zetDebugWriteRegisters(debug_session, device_thread,
                                   ZET_DEBUG_REGSET_TYPE_CR_INTEL_GPU, 0,
                                   cr_reg_prop.count, cr_values),
            ZE_RESULT_SUCCESS);
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
  LOG_DEBUG << "[Debugger] Found " << count << " regsets";
  EXPECT_EQ(device, device_initial);
  EXPECT_NE(properties.data(), nullptr);

  return properties;
}

std::vector<zet_debug_regset_properties_t> get_thread_register_set_properties(
    const zet_debug_session_handle_t &debug_session,
    const ze_device_handle_t &device, const ze_device_thread_t &thread) {

  uint32_t count = 0;

  auto device_initial = device;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zetDebugGetThreadRegisterSetProperties(
                                   debug_session, thread, &count, nullptr));

  std::vector<zet_debug_regset_properties_t> properties(
      count, {ZET_STRUCTURE_TYPE_DEBUG_REGSET_PROPERTIES});

  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetDebugGetThreadRegisterSetProperties(debug_session, thread,
                                                   &count, properties.data()));

  EXPECT_EQ(device, device_initial);

  return properties;
}

void debug_read_registers(const zet_debug_session_handle_t &debug_session,
                          const ze_device_thread_t &device_thread,
                          const uint32_t type, const uint32_t start,
                          const uint32_t count, void *buffer) {

  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetDebugReadRegisters(debug_session, device_thread, type, start,
                                  count, buffer));
}

void debug_write_registers(const zet_debug_session_handle_t &debug_session,
                           const ze_device_thread_t &device_thread,
                           const uint32_t type, const uint32_t start,
                           const uint32_t count, void *buffer) {

  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetDebugWriteRegisters(debug_session, device_thread, type, start,
                                   count, buffer));
}

std::vector<uint8_t> get_debug_info(const zet_module_handle_t &module_handle) {
  size_t size = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetModuleGetDebugInfo(module_handle,
                                  ZET_MODULE_DEBUG_INFO_FORMAT_ELF_DWARF, &size,
                                  nullptr));

  std::vector<uint8_t> debug_info(size);
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetModuleGetDebugInfo(module_handle,
                                  ZET_MODULE_DEBUG_INFO_FORMAT_ELF_DWARF, &size,
                                  debug_info.data()));
  return debug_info;
}

} // namespace level_zero_tests
