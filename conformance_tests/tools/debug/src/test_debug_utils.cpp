/*
 *
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "logging/logging.hpp"
#include "utils/utils.hpp"
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
  uint8_t buffer1[bufferSize];
  uint8_t buffer2[bufferSize];
  uint8_t origBuffer[bufferSize];
  memset(buffer1, 0xaa, bufferSize);
  memset(buffer2, 0xaa, bufferSize);

  // Access ISA
  desc.address = module_event.info.module.load;
  lzt::debug_read_memory(debug_session, thread, desc, bufferSize, buffer1);
  for (int i = 0; i < bufferSize; i++) {
    if (buffer1[i] != 0xaa) {
      read_success = true;
    }
  }
  EXPECT_TRUE(read_success);
  read_success = false;

  desc.address += 0xF; // add intentional missalignment
  lzt::debug_read_memory(debug_session, thread, desc, bufferSize, buffer2);
  for (int i = 0; i < bufferSize; i++) {
    if (buffer2[i] != 0xaa) {
      read_success = true;
    }
  }
  EXPECT_TRUE(read_success);
  read_success = false;

  EXPECT_FALSE(memcmp(buffer1 + 0xF, buffer2,
                      bufferSize - 0xF)); // memcmp returns 0 if equal

  memcpy(origBuffer, buffer2, bufferSize);

  *(reinterpret_cast<uint64_t *>(buffer2)) = 0xDEADBEEFDEADBEEF;

  lzt::debug_write_memory(debug_session, thread, desc, bufferSize, buffer2);
  memset(buffer2, 0xaa, bufferSize);
  lzt::debug_read_memory(debug_session, thread, desc, bufferSize, buffer2);
  if (*(reinterpret_cast<uint64_t *>(buffer2)) != 0xDEADBEEFDEADBEEF) {
    FAIL() << "[Debugger] Writing memory failed";
  }

  // Restore content
  lzt::debug_write_memory(debug_session, thread, desc, bufferSize, origBuffer);

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

bool check_event(zet_debug_session_handle_t &debug_session,
                 zet_debug_event_type_t eventType) {

  bool found = false;
  zet_debug_event_t debugEvent;

  lzt::debug_read_event(debug_session, debugEvent, eventsTimeoutMS, false);

  if (debugEvent.type == eventType) {
    LOG_INFO << "[Debugger] expected event received: "
             << lzt::debuggerEventTypeString[debugEvent.type];
    found = true;
  } else {
    LOG_WARNING << "[Debugger] UNEXPECTED event received: "
                << lzt::debuggerEventTypeString[debugEvent.type];
  }

  return found;
}

bool check_events(zet_debug_session_handle_t &debug_session,
                  std::vector<zet_debug_event_type_t> eventTypes) {

  for (auto eventType : eventTypes) {
    if (!check_event(debug_session, eventType)) {
      return false;
    }
  }

  return true;
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
    FAIL() << "[Debugger] Failed to attach to start a debug session";
  }

  synchro->notify_application();

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
      LOG_INFO << "[Debugger]"
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
      FAIL() << "[Debugger] Timed out waiting for module load event";
    }
  }

  if (!module_loaded) {
    FAIL() << "[Debugger] Did not receive module load event";
  }
}

void print_thread(const char *entry_message,
                  const ze_device_thread_t &device_thread,
                  log_level_t logLevel) {
  std::stringstream message;
  message << entry_message << "SLICE:" << device_thread.slice
          << " SUBSLICE: " << device_thread.subslice
          << " EU: " << device_thread.eu << " THREAD: " << device_thread.thread;

  if (logLevel == WARNING) {
    LOG_WARNING << message.str();
  } else if (logLevel == INFO) {
    LOG_INFO << message.str();
  } else if (logLevel == DEBUG) {
    LOG_DEBUG << message.str();
  }
}

void get_numCQs_per_ordinal(ze_device_handle_t &device,
                            std::map<int, int> &ordinalCQs) {

  uint32_t numQueueGroups =
      lzt::get_command_queue_group_properties_count(device);

  ASSERT_GE(numQueueGroups, 0);

  std::vector<ze_command_queue_group_properties_t> queueProperties(
      numQueueGroups);
  queueProperties = lzt::get_command_queue_group_properties(device);

  for (int i = 0; i < numQueueGroups; i++) {
    ordinalCQs[i] = queueProperties[i].numQueues;

    LOG_DEBUG << "ordinal: " << i << " CQs: " << queueProperties[i].numQueues;
    ASSERT_GE(queueProperties[i].numQueues, 0);
  }
}
