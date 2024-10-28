/*
 *
 * Copyright (C) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <thread>

#include "gtest/gtest.h"

#include "logging/logging.hpp"
#include "utils/utils.hpp"
#include "test_debug.hpp"
#include "test_debug_utils.hpp"

namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>
#include <level_zero/zet_api.h>

void print_device(const ze_device_handle_t &device) {
  std::cout << "\n==========================================================="
               "=====================\n\n"
            << std::endl;
  auto properties = lzt::get_device_properties(device);
  // print uuid
  LOG_INFO << "Device : " << properties.uuid;
}

bool check_event(const zet_debug_session_handle_t &debug_session,
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

  if (debugEvent.flags & ZET_DEBUG_EVENT_FLAG_NEED_ACK) {
    LOG_INFO << "[Debugger] Acking event";
    lzt::debug_ack_event(debug_session, &debugEvent);
  }
  return found;
}

bool check_events(const zet_debug_session_handle_t &debug_session,
                  std::vector<zet_debug_event_type_t> eventTypes) {

  for (auto eventType : eventTypes) {
    if (!check_event(debug_session, eventType)) {
      return false;
    }
  }

  return true;
}

bool check_events_unordered(const zet_debug_session_handle_t &debug_session,
                            std::vector<zet_debug_event_type_t> &eventTypes) {

  zet_debug_event_t debugEvent;

  for (int i = 0; i < eventTypes.size(); i++) {
    lzt::debug_read_event(debug_session, debugEvent, eventsTimeoutMS, false);

    // note: this should be modified if eventTypes contains duplicates
    if (eventTypes.end() !=
        std::find(eventTypes.begin(), eventTypes.end(), debugEvent.type)) {
      LOG_INFO << "[Debugger] expected event received: "
               << lzt::debuggerEventTypeString[debugEvent.type];
    } else {
      LOG_WARNING << "[Debugger] UNEXPECTED event received: "
                  << lzt::debuggerEventTypeString[debugEvent.type];
      return false;
    }

    if (debugEvent.flags & ZET_DEBUG_EVENT_FLAG_NEED_ACK) {
      LOG_INFO << "[Debugger] Acking event";
      lzt::debug_ack_event(debug_session, &debugEvent);
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

ze_result_t readWriteSLMMemory(const zet_debug_session_handle_t &debug_session,
                               const ze_device_thread_t &thread,
                               uint64_t slmBaseAddress) {

  static constexpr uint16_t bufferSize =
      512; // Also defined in test_debug_helper.cpp for SLM buffer size

  zet_debug_memory_space_desc_t desc = {};
  desc.type = ZET_DEBUG_MEMORY_SPACE_TYPE_SLM;

  zet_debug_memory_space_desc_t verifyDesc = {};
  verifyDesc.type = ZET_DEBUG_MEMORY_SPACE_TYPE_SLM;
  verifyDesc.address = slmBaseAddress;

  uint8_t buffer1[bufferSize];
  uint8_t buffer2[bufferSize];
  uint8_t original[bufferSize];

  unsigned char slm_pattern[4] = {0xDE, 0xAD, 0xBE, 0xEF};

  memset(buffer1, 0xaa, bufferSize);
  int i = 0;
  for (i = 0; i < bufferSize; i++) {
    buffer2[i] = slm_pattern[i & 0x3];
  }

  // Save original content to restore at the end
  int accessSize;
  int accessOffset;
  desc.address = slmBaseAddress;

  lzt::debug_read_memory(debug_session, thread, desc, bufferSize, original);
  for (i = 0; i < bufferSize; i++) {
    // see test_debug_helper.cpp run_long_kernel() src_buffer[] init
    EXPECT_EQ(original[i], (i + 1 & 0xFF));
  }
  if (::testing::Test::HasFailure()) {
    return ZE_RESULT_ERROR_UNKNOWN;
  }

  // SIP access SLM in defined unit sizes at aligned addresses, so test multiple
  // combinations
  accessSize = 7;
  accessOffset = 0;
  desc.address = slmBaseAddress + accessOffset;
  lzt::debug_write_memory(debug_session, thread, desc, accessSize, buffer2);
  lzt::debug_read_memory(debug_session, thread, verifyDesc, bufferSize,
                         buffer1);
  // verify the content written
  for (i = 0; i < accessSize; i++) {
    EXPECT_EQ(buffer1[i], buffer2[i]);
  }
  // Veriy the rest of the buffer was not altered
  for (i = accessSize; i < bufferSize; i++) {
    EXPECT_EQ(buffer1[i], (i + 1 & 0xFF) + accessOffset);
  }
  memset(buffer1, 0, bufferSize);
  if (::testing::Test::HasFailure()) {
    return ZE_RESULT_ERROR_UNKNOWN;
  }

  accessSize = 7;
  accessOffset = 0x05;
  desc.address = slmBaseAddress + accessOffset;
  lzt::debug_write_memory(debug_session, thread, desc, accessSize, buffer2);
  lzt::debug_read_memory(debug_session, thread, verifyDesc, bufferSize,
                         buffer1);
  // verify the content written
  for (i = accessOffset; i < accessSize + accessOffset; i++) {
    EXPECT_EQ(buffer1[i], buffer2[i - accessOffset]);
  }
  // Veriy the rest of the buffer was not altered
  for (i = accessOffset + accessSize;
       i < bufferSize - accessOffset - accessSize; i++) {
    EXPECT_EQ(buffer1[i], (i + 1 & 0xFF));
  }
  memset(buffer1, 0, bufferSize);
  if (::testing::Test::HasFailure()) {
    return ZE_RESULT_ERROR_UNKNOWN;
  }

  // restore
  lzt::debug_write_memory(debug_session, thread, verifyDesc, bufferSize,
                          original);

  accessSize = 7;
  accessOffset = 0x0f;
  desc.address = slmBaseAddress + accessOffset;
  lzt::debug_write_memory(debug_session, thread, desc, accessSize, buffer2);
  lzt::debug_read_memory(debug_session, thread, verifyDesc, bufferSize,
                         buffer1);
  // Veriy the rest of the buffer was not altered
  for (i = 0; i < accessOffset; i++) {
    EXPECT_EQ(buffer1[i], (i + 1 & 0xFF));
  }
  // verify the content written
  for (i = accessOffset; i < accessSize + accessOffset; i++) {
    EXPECT_EQ(buffer1[i], buffer2[i - accessOffset]);
  }
  // Veriy the rest of the buffer was not altered
  for (i = accessOffset + accessSize;
       i < bufferSize - accessOffset - accessSize; i++) {
    EXPECT_EQ(buffer1[i], (i + 1 & 0xFF));
  }
  memset(buffer1, 0, bufferSize);
  if (::testing::Test::HasFailure()) {
    return ZE_RESULT_ERROR_UNKNOWN;
  }

  accessSize = 132;
  accessOffset = 0x0f;
  desc.address = slmBaseAddress + accessOffset;
  lzt::debug_write_memory(debug_session, thread, desc, accessSize, buffer2);
  lzt::debug_read_memory(debug_session, thread, verifyDesc, bufferSize,
                         buffer1);
  // Veriy the rest of the buffer was not altered
  for (i = 0; i < accessOffset; i++) {
    EXPECT_EQ(buffer1[i], (i + 1 & 0xFF));
  }
  // verify the content written
  for (i = accessOffset; i < accessSize + accessOffset; i++) {
    EXPECT_EQ(buffer1[i], buffer2[i - accessOffset]);
  }
  // Veriy the rest of the buffer was not altered
  for (i = accessOffset + accessSize;
       i < bufferSize - accessOffset - accessSize; i++) {
    EXPECT_EQ(buffer1[i], (i + 1 & 0xFF));
  }
  memset(buffer1, 0, bufferSize);
  if (::testing::Test::HasFailure()) {
    return ZE_RESULT_ERROR_UNKNOWN;
  }

  accessSize = 230;
  accessOffset = 0x0a;
  desc.address = slmBaseAddress + accessOffset;
  lzt::debug_write_memory(debug_session, thread, desc, accessSize, buffer2);
  lzt::debug_read_memory(debug_session, thread, verifyDesc, bufferSize,
                         buffer1);
  // Veriy the rest of the buffer was not altered
  for (i = 0; i < accessOffset; i++) {
    EXPECT_EQ(buffer1[i], (i + 1 & 0xFF));
  }
  // verify the content written
  for (i = accessOffset; i < accessSize + accessOffset; i++) {
    EXPECT_EQ(buffer1[i], buffer2[i - accessOffset]);
  }
  // Veriy the rest of the buffer was not altered
  for (i = accessOffset + accessSize;
       i < bufferSize - accessOffset - accessSize; i++) {
    EXPECT_EQ(buffer1[i], (i + 1 & 0xFF));
  }
  memset(buffer1, 0, bufferSize);
  if (::testing::Test::HasFailure()) {
    return ZE_RESULT_ERROR_UNKNOWN;
  }

  // restore
  lzt::debug_write_memory(debug_session, thread, verifyDesc, bufferSize,
                          original);

  return ZE_RESULT_SUCCESS;
}

void readWriteModuleMemory(const zet_debug_session_handle_t &debug_session,
                           const ze_device_thread_t &thread,
                           zet_debug_event_t &module_event, bool access_elf) {

  static constexpr uint8_t bufferSize = 16;
  bool read_success = false;
  zet_debug_memory_space_desc_t desc = {};
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

int get_numCQs_per_ordinal(ze_device_handle_t &device,
                           std::map<int, int> &ordinalCQs) {

  int totalNumCQs = 0;
  uint32_t numQueueGroups =
      lzt::get_command_queue_group_properties_count(device);

  EXPECT_GE(numQueueGroups, 0);

  std::vector<ze_command_queue_group_properties_t> queueProperties(
      numQueueGroups);
  queueProperties = lzt::get_command_queue_group_properties(device);

  for (int i = 0; i < numQueueGroups; i++) {
    ordinalCQs[i] = queueProperties[i].numQueues;

    LOG_DEBUG << "ordinal: " << i << " CQs: " << queueProperties[i].numQueues;
    EXPECT_GE(queueProperties[i].numQueues, 0);
    totalNumCQs += queueProperties[i].numQueues;
  }

  LOG_DEBUG << "Total num of CQs: " << totalNumCQs;

  return totalNumCQs;
}

bool read_register(const zet_debug_session_handle_t &debug_session,
                   const ze_device_thread_t &device_thread,
                   const zet_debug_regset_properties_t &regset, bool printerr) {

  uint32_t data[32] = {};

  auto result = zetDebugReadRegisters(debug_session, device_thread, regset.type,
                                      0, regset.count, data);

  if (result == ZE_RESULT_SUCCESS) {
    LOG_DEBUG << "[Debugger] zetDebugReadRegisters: thread ("
              << device_thread.slice << " ," << device_thread.subslice << ", "
              << device_thread.eu << ", " << device_thread.thread
              << ") read successfully";

    return true;
  } else if (printerr) {
    LOG_WARNING
        << "[Debugger] Error reading register to determine thread state: "
        << result;
  }
  return false;
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

bool unique_thread(const ze_device_thread_t &device_thread) {
  print_thread("[Debugger] is thread unique: ", device_thread, DEBUG);
  return (device_thread.slice != UINT32_MAX &&
          device_thread.subslice != UINT32_MAX &&
          device_thread.eu != UINT32_MAX && device_thread.thread != UINT32_MAX);
}

bool are_threads_equal(const ze_device_thread_t &thread1,
                       const ze_device_thread_t &thread2) {
  return (thread1.slice == thread2.slice &&
          thread1.subslice == thread2.subslice && thread1.eu == thread2.eu &&
          thread1.thread == thread2.thread);
}

bool is_thread_in_vector(const ze_device_thread_t &thread,
                         const std::vector<ze_device_thread_t> &threads) {
  bool flag = false;
  for (auto threadIterator : threads) {
    if (are_threads_equal(thread, threadIterator)) {
      flag = true;
      break;
    }
  }

  return flag;
}

// return a list of stopped threads
std::vector<ze_device_thread_t>
get_stopped_threads(const zet_debug_session_handle_t &debug_session,
                    const ze_device_handle_t &device) {
  std::vector<ze_device_thread_t> threads;

  auto device_properties = lzt::get_device_properties(device);
  auto regset_properties = lzt::get_register_set_properties(device);

  for (uint32_t slice = 0; slice < device_properties.numSlices; slice++) {
    for (uint32_t subslice = 0;
         subslice < device_properties.numSubslicesPerSlice; subslice++) {
      for (uint32_t eu = 0; eu < device_properties.numEUsPerSubslice; eu++) {
        for (uint32_t thread = 0; thread < device_properties.numThreadsPerEU;
             thread++) {

          ze_device_thread_t device_thread = {};
          device_thread.slice = slice;
          device_thread.subslice = subslice;
          device_thread.eu = eu;
          device_thread.thread = thread;

          if (read_register(debug_session, device_thread, regset_properties[2],
                            false)) {
            lzt::clear_exceptions(device, debug_session, device_thread);
            threads.push_back(device_thread);
          }
        }
      }
    }
  }
  LOG_INFO << "[Debugger] Number of stopped threads: " << threads.size();

  return threads;
}

// wait for stopped thread event and return stopped threads
bool find_stopped_threads(const zet_debug_session_handle_t &debugSession,
                          const ze_device_handle_t &device,
                          ze_device_thread_t thread, bool checkEvent,
                          std::vector<ze_device_thread_t> &stoppedThreads) {
  uint8_t attempts = 0;
  zet_debug_event_t debugEvent = {};
  stoppedThreads.clear();
  do {
    lzt::debug_read_event(debugSession, debugEvent, eventsTimeoutMS / 10, true);
    LOG_INFO << "[Debugger] received event: "
             << lzt::debuggerEventTypeString[debugEvent.type];

    if (debugEvent.type == ZET_DEBUG_EVENT_TYPE_THREAD_STOPPED) {
      print_thread("[Debugger] Stopped thread event for ",
                   debugEvent.info.thread.thread, DEBUG);

      if (checkEvent) {
        EXPECT_TRUE(are_threads_equal(thread, debugEvent.info.thread.thread));
      }

      stoppedThreads = get_stopped_threads(debugSession, device);
      break;
    }
    attempts++;
  } while (attempts < 5);

  if (stoppedThreads.size() > 0) {
    return true;
  } else {
    return false;
  }
}

bool find_multi_event_stopped_threads(
    const zet_debug_session_handle_t &debugSession,
    const ze_device_handle_t &device,
    std::vector<ze_device_thread_t> &threadsToCheck, bool checkEvent,
    std::vector<ze_device_thread_t> &stoppedThreadsFound) {

  uint8_t attempts = 0;
  uint16_t numEventsReceived = 0;
  uint16_t numEventsExpected = threadsToCheck.size();

  zet_debug_event_t debugEvent = {};
  stoppedThreadsFound.clear();
  bool foundAll = true;

  LOG_DEBUG << "[Debugger] Expecting " << threadsToCheck.size() << " events.";
  for (auto threadToCheck : threadsToCheck) {
    do {
      lzt::debug_read_event(debugSession, debugEvent, eventsTimeoutMS / 10,
                            true);
      LOG_INFO << "[Debugger] received event: "
               << lzt::debuggerEventTypeString[debugEvent.type];

      if (debugEvent.type == ZET_DEBUG_EVENT_TYPE_THREAD_STOPPED) {
        print_thread("[Debugger] Stopped thread event for ",
                     debugEvent.info.thread.thread, DEBUG);

        if (checkEvent) {
          EXPECT_TRUE(is_thread_in_vector(debugEvent.info.thread.thread,
                                          threadsToCheck));
        }
        numEventsReceived++;
        break;
      }
      attempts++;
    } while (attempts < 5);
  }

  EXPECT_EQ(numEventsReceived, numEventsExpected);

  stoppedThreadsFound = get_stopped_threads(debugSession, device);

  for (auto threadToCheck : threadsToCheck) {
    if (!is_thread_in_vector(threadToCheck, stoppedThreadsFound)) {
      foundAll = false;
      EXPECT_TRUE(0);
      break;
    }
  }

  return foundAll;
}

std::vector<ze_device_thread_t>
get_threads_in_eu(uint32_t eu, std::vector<ze_device_thread_t> threads) {
  std::vector<ze_device_thread_t> threadsInEu;
  for (auto &thread : threads) {
    if (thread.eu == eu) {
      threadsInEu.push_back(thread);
    }
  }
  return threadsInEu;
}

std::vector<ze_device_thread_t>
get_threads_in_subSlice(uint32_t subSlice,
                        std::vector<ze_device_thread_t> threads) {
  std::vector<ze_device_thread_t> threadsInSubSlice;
  for (auto &thread : threads) {
    if (thread.subslice == subSlice) {
      threadsInSubSlice.push_back(thread);
    }
  }
  return threadsInSubSlice;
}

std::vector<ze_device_thread_t>
get_threads_in_slice(uint32_t slice, std::vector<ze_device_thread_t> threads) {
  std::vector<ze_device_thread_t> threadsInSlice;
  for (auto &thread : threads) {
    if (thread.slice == slice) {
      threadsInSlice.push_back(thread);
    }
  }
  return threadsInSlice;
}

void wait_for_events_interrupt_and_resume(
    const zet_debug_session_handle_t &debugSession, process_synchro *synchro,
    uint64_t *gpu_buffer_va, ze_device_handle_t device, bool *address_valid) {

  auto device_properties = lzt::get_device_properties(device);

  zet_debug_event_t debug_event;
  std::vector<zet_debug_event_type_t> expectedEvents = {
      ZET_DEBUG_EVENT_TYPE_PROCESS_ENTRY, ZET_DEBUG_EVENT_TYPE_MODULE_LOAD};

  if (!check_events(debugSession, expectedEvents)) {
    FAIL() << "[Debugger] Did not receive expected events";
  }

  ze_device_thread_t device_thread;
  device_thread.slice = UINT32_MAX;
  device_thread.subslice = UINT32_MAX;
  device_thread.eu = UINT32_MAX;
  device_thread.thread = UINT32_MAX;

  LOG_INFO << "[Debugger] Sleeping to wait for device threads";
  std::this_thread::sleep_for(std::chrono::seconds(12));

  LOG_INFO << "[Debugger] Sending interrupt";
  lzt::debug_interrupt(debugSession, device_thread);

  std::vector<ze_device_thread_t> stopped_threads;
  if (!find_stopped_threads(debugSession, device, device_thread, true,
                            stopped_threads)) {
    FAIL() << "[Debugger] Did not find stopped threads";
  }

  // write to kernel buffer to signal to application to end
  zet_debug_memory_space_desc_t memory_space_desc = {};

  // we need to wait until address is valid
  LOG_INFO << "[Debugger] Waiting until address " << std::hex << address_valid
           << " is valid";
  while (!address_valid)
    ;

  memory_space_desc.address = *gpu_buffer_va;
  memory_space_desc.type = ZET_DEBUG_MEMORY_SPACE_TYPE_DEFAULT;
  memory_space_desc.stype = ZET_STRUCTURE_TYPE_DEBUG_MEMORY_SPACE_DESC;

  uint8_t *buffer = new uint8_t[1], *val_buffer = new uint8_t[1];
  buffer[0] = 0;
  val_buffer[0] = 1;
  auto thread = debug_event.info.thread.thread;
  LOG_INFO << "[Debugger] Writing to address: " << std::hex << *gpu_buffer_va;
  LOG_INFO << "[Debugger] on device:" << device_properties.uuid;

  lzt::debug_write_memory(debugSession, thread, memory_space_desc, 1, buffer);
  // validate write succeeded
  lzt::debug_read_memory(debugSession, thread, memory_space_desc, 1,
                         val_buffer);
  ASSERT_EQ(val_buffer[0], 0);

  delete[] buffer;
  delete[] val_buffer;
  print_thread("Resuming device thread ", thread, DEBUG);
  lzt::debug_resume(debugSession, thread);
}

void zetDebugMemAccessTest::run_read_write_module_and_memory_test(
    std::vector<ze_device_handle_t> &devices, bool test_slm,
    uint64_t slmBaseAddress, bool use_sub_devices) {
  for (auto &device : devices) {
    print_device(device);
    if (!is_debug_supported(device)) {
      continue;
    }

    synchro->clear_debugger_signal();

    debug_test_type_t helper_test_type;
    helper_test_type = test_slm ? LONG_RUNNING_KERNEL_INTERRUPTED_SLM
                                : LONG_RUNNING_KERNEL_INTERRUPTED;

    debugHelper = launch_process(helper_test_type, device, use_sub_devices);

    zet_debug_event_t module_event;
    attach_and_get_module_event(debugHelper.id(), synchro, device, debugSession,
                                module_event);

    if (module_event.flags & ZET_DEBUG_EVENT_FLAG_NEED_ACK) {
      LOG_DEBUG << "[Debugger] Acking event: "
                << lzt::debuggerEventTypeString[module_event.type];
      lzt::debug_ack_event(debugSession, &module_event);
    }

    uint64_t gpu_buffer_va = 0;
    synchro->wait_for_application_signal();
    if (!synchro->get_app_gpu_buffer_address(gpu_buffer_va)) {
      FAIL() << "[Debugger] Could not get a valid GPU buffer VA";
    }
    synchro->clear_application_signal();

    ze_device_thread_t thread;
    thread.slice = UINT32_MAX;
    thread.subslice = UINT32_MAX;
    thread.eu = UINT32_MAX;
    thread.thread = UINT32_MAX;

    LOG_INFO << "[Debugger] Interrupting all threads";
    // give time to app to launch the kernel
    std::this_thread::sleep_for(std::chrono::seconds(60));

    lzt::debug_interrupt(debugSession, thread);
    std::vector<ze_device_thread_t> stopped_threads;
    if (!find_stopped_threads(debugSession, device, thread, true,
                              stopped_threads)) {
      FAIL() << "[Debugger] Did not find stopped threads";
    }

    zet_debug_memory_space_desc_t memorySpaceDesc = {};
    memorySpaceDesc.type = ZET_DEBUG_MEMORY_SPACE_TYPE_DEFAULT;
    int sizeToRead = 512;
    uint8_t *buffer = new uint8_t[sizeToRead];

    memorySpaceDesc.address = gpu_buffer_va;

    LOG_INFO << "[Debugger] Reading/Writing on interrupted threads";
    ze_result_t status;
    for (auto &stopped_thread : stopped_threads) {

      lzt::debug_read_memory(debugSession, stopped_thread, memorySpaceDesc,
                             sizeToRead, buffer);

      if (test_slm) {
        print_thread(
            "[Debugger] Reading and writing SLM memory from Stopped thread",
            stopped_thread, DEBUG);

        status =
            readWriteSLMMemory(debugSession, stopped_thread, slmBaseAddress);
        if (status != ZE_RESULT_SUCCESS) {
          FAIL() << "[Debugger] Failed accessing SLM memory";
        }

        // Modify pattern only on the last thread since SLM area is shared by
        // all threads
        if (stopped_thread == stopped_threads.back()) {
          // Write the custom pattern in the first bytes which
          // test_debug_helper is expecting to find.
          unsigned char slm_pattern[24] = {0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD,
                                           0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF,
                                           0xDE, 0xAD, 0xBE, 0xEF, 0xDE, 0xAD,
                                           0xBE, 0xEF, 0xDE, 0xAD, 0xBE, 0xEF};

          zet_debug_memory_space_desc_t slmSpaceDesc = {};
          slmSpaceDesc.type = ZET_DEBUG_MEMORY_SPACE_TYPE_SLM;
          slmSpaceDesc.address = slmBaseAddress;

          lzt::debug_write_memory(debugSession, stopped_thread, slmSpaceDesc,
                                  sizeof(slm_pattern), slm_pattern);
        }

      } else {
        print_thread("[Debugger] Reading and writing DEFAULT memory from "
                     "Stopped thread ",
                     stopped_thread, DEBUG);

        readWriteModuleMemory(debugSession, stopped_thread, module_event,
                              false);

        // Skip the first byte since the first thread read will
        // see it 1 and others will see 0 after setting buffer[0]=0 below
        int i = 1;
        for (i = 1; i < sizeToRead; i++) {
          // see test_debug_helper.cpp run_long_kernel() src_buffer[] init
          EXPECT_EQ(buffer[i], (i + 1 & 0xFF));
        }
      }

      // set buffer[0] to 0 to break the loop. See debug_loop.cl
      buffer[0] = 0;
      lzt::debug_write_memory(debugSession, thread, memorySpaceDesc, sizeToRead,
                              buffer);
    }

    LOG_INFO << "[Debugger] resuming interrupted threads";
    lzt::debug_resume(debugSession, thread);
    delete[] buffer;

    debugHelper.wait();
    lzt::debug_detach(debugSession);
    ASSERT_EQ(debugHelper.exit_code(), 0);
  }
}
