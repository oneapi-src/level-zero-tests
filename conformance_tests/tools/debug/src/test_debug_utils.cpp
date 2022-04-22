/*
 *
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <boost/filesystem.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/named_condition.hpp>

#include "gtest/gtest.h"

#include "logging/logging.hpp"
#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "test_debug.hpp"

namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>
#include <level_zero/zet_api.h>

void readWriteModuleMemory(const zet_debug_session_handle_t &debug_session,
                           const ze_device_thread_t &thread,
                           zet_debug_event_t &module_event, bool access_elf) {

  static constexpr uint8_t bufferSize = 16;
  bool read_success = false;
  zet_debug_memory_space_desc_t desc;
  desc.type = ZET_DEBUG_MEMORY_SPACE_TYPE_DEFAULT;
  uint8_t buffer[bufferSize];
  memset(buffer, 0xaa, bufferSize);

  // Access ISA
  desc.address = module_event.info.module.load;
  lzt::debug_read_memory(debug_session, thread, desc, bufferSize, buffer);
  for (int i = 0; i < bufferSize; i++) {
    if (buffer[i] != 0xaa) {
      read_success = true;
    }
  }
  EXPECT_TRUE(read_success);
  read_success = false;
  memset(buffer, 0xaa, bufferSize);

  desc.address += 0xF; // add intentional missalignment
  lzt::debug_read_memory(debug_session, thread, desc, bufferSize, buffer);
  for (int i = 0; i < bufferSize; i++) {
    if (buffer[i] != 0xaa) {
      read_success = true;
    }
  }
  EXPECT_TRUE(read_success);
  read_success = false;

  uint8_t bufferCopy[bufferSize];
  memcpy(bufferCopy, buffer, bufferSize);

  lzt::debug_write_memory(debug_session, thread, desc, bufferSize, buffer);

  // Confirm reading again returns the original content
  memset(buffer, 0xaa, bufferSize);
  lzt::debug_read_memory(debug_session, thread, desc, bufferSize, buffer);
  EXPECT_FALSE(
      memcmp(bufferCopy, buffer, bufferSize)); // memcmp retruns 0 on equal

  // Access ELF

  if (access_elf) {
    int offset = 0xF;
    size_t elf_size = module_event.info.module.moduleEnd -
                      module_event.info.module.moduleBegin;
    uint8_t *elf_buffer = new uint8_t[elf_size];
    memset(elf_buffer, 0xaa, elf_size);

    desc.address = module_event.info.module.moduleBegin;
    LOG_DEBUG << "[Debugger] Reading ELF of size " << elf_size;
    lzt::debug_read_memory(debug_session, thread, desc, elf_size, elf_buffer);

    EXPECT_EQ(elf_buffer[1], 'E');
    EXPECT_EQ(elf_buffer[2], 'L');
    EXPECT_EQ(elf_buffer[3], 'F');

    for (int i = 0; i < elf_size; i++) {
      if (elf_buffer[i] != 0xaa) {
        read_success = true;
      }
    }
    EXPECT_TRUE(read_success);
    read_success = false;

    memset(elf_buffer, 0xaa, elf_size);
    desc.address += offset; // add intentional missalignment
    lzt::debug_read_memory(debug_session, thread, desc, elf_size - offset,
                           elf_buffer);
    for (int i = 0; i < elf_size; i++) {
      if (elf_buffer[i] != 0xaa) {
        read_success = true;
      }
    }
    EXPECT_TRUE(read_success);
    read_success = false;

    delete[] elf_buffer;
  }
}

void attach_and_get_module_event(uint32_t pid, process_synchro *synchro,
                                 ze_device_handle_t device,
                                 zet_debug_session_handle_t &debug_session,
                                 zet_debug_event_t &module_event) {
  module_event = {};
  zet_debug_config_t debug_config = {};
  debug_config.pid = pid;
  debug_session = lzt::debug_attach(device, debug_config);
  if (!debug_session) {
    LOG_ERROR << "[Debugger] Failed to attach to start a debug session";
    return;
  }

  synchro->notify_attach();

  bool module_loaded = false;
  std::chrono::time_point<std::chrono::system_clock> start, checkpoint;
  start = std::chrono::system_clock::now();

  LOG_INFO << "[Debugger] Listening for events";

  while (!module_loaded) {
    zet_debug_event_t debug_event;
    ze_result_t result = lzt::debug_read_event(debug_session, debug_event,
                                               eventsTimeoutMS, false);
    if (ZE_RESULT_SUCCESS != result) {
      break;
    }

    LOG_INFO << "[Debugger] received event: "
             << lzt::debuggerEventTypeString[debug_event.type];

    if (ZET_DEBUG_EVENT_TYPE_MODULE_LOAD == debug_event.type) {
      LOG_INFO << "[Debugger] ZET_DEBUG_EVENT_TYPE_MODULE_LOAD."
               << " ISA load address: " << std::hex
               << debug_event.info.module.load << " ELF begin: " << std::hex
               << debug_event.info.module.moduleBegin
               << " ELF end: " << std::hex << debug_event.info.module.moduleEnd;
      EXPECT_TRUE(debug_event.flags & ZET_DEBUG_EVENT_FLAG_NEED_ACK);
      if (debug_event.info.module.load) {
        module_loaded = true;
        module_event = debug_event;
      }
    }
    checkpoint = std::chrono::system_clock::now();
    std::chrono::duration<double> secondsLooping = checkpoint - start;
    if (secondsLooping.count() > eventsTimeoutS) {
      LOG_ERROR << "[Debugger] Timed out waiting for module event";
      break;
    }
  }
}
