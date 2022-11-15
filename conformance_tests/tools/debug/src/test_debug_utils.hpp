/*
 *
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef TEST_DEBUG_UTILS_HPP
#define TEST_DEBUG_UTILS_HPP

#include "test_debug_common.hpp"
void print_device(const ze_device_handle_t &device);

bool check_event(const zet_debug_session_handle_t &debug_session,
                 zet_debug_event_type_t eventType);
bool check_events(const zet_debug_session_handle_t &debug_session,
                  std::vector<zet_debug_event_type_t> eventTypes);
bool check_events_unordered(const zet_debug_session_handle_t &debug_session,
                            std::vector<zet_debug_event_type_t> &eventTypes);

void attach_and_get_module_event(uint32_t pid, process_synchro *synchro,
                                 ze_device_handle_t device,
                                 zet_debug_session_handle_t &debug_session,
                                 zet_debug_event_t &module_event);

ze_result_t readWriteSLMMemory(const zet_debug_session_handle_t &debug_session,
                               const ze_device_thread_t &thread,
                               uint64_t slmBaseAddress);

void readWriteModuleMemory(const zet_debug_session_handle_t &debug_session,
                           const ze_device_thread_t &thread,
                           zet_debug_event_t &module_event, bool access_elf);

int get_numCQs_per_ordinal(ze_device_handle_t &device,
                           std::map<int, int> &ordinalCQs);
bool read_register(const zet_debug_session_handle_t &debug_session,
                   const ze_device_thread_t &device_thread,
                   const zet_debug_regset_properties_t &regset, bool printerr);

void print_thread(const char *entry_message,
                  const ze_device_thread_t &device_thread,
                  log_level_t logLevel);

bool unique_thread(const ze_device_thread_t &device_thread);
bool are_threads_equal(const ze_device_thread_t &thread1,
                       const ze_device_thread_t &thread2);
bool is_thread_in_vector(const ze_device_thread_t &thread,
                         const std::vector<ze_device_thread_t> &threads);
std::vector<ze_device_thread_t>
get_stopped_threads(const zet_debug_session_handle_t &debug_session,
                    const ze_device_handle_t &device);
bool find_stopped_threads(const zet_debug_session_handle_t &debugSession,
                          const ze_device_handle_t &device,
                          ze_device_thread_t thread, bool checkEvent,
                          std::vector<ze_device_thread_t> &stoppedThreads);

bool find_multi_event_stopped_threads(
    const zet_debug_session_handle_t &debugSession,
    const ze_device_handle_t &device,
    std::vector<ze_device_thread_t> &threadsToCheck, bool checkEvent,
    std::vector<ze_device_thread_t> &stoppedThreadsFound);

std::vector<ze_device_thread_t>
get_threads_in_eu(uint32_t eu, std::vector<ze_device_thread_t> threads);
std::vector<ze_device_thread_t>
get_threads_in_subSlice(uint32_t subSlice,
                        std::vector<ze_device_thread_t> threads);
std::vector<ze_device_thread_t>
get_threads_in_slice(uint32_t slice, std::vector<ze_device_thread_t> threads);

struct smaller_thread_functor {

  inline bool operator()(ze_device_thread_t thread1,
                         ze_device_thread_t thread2) const {
    if (thread1.slice < thread2.slice) {
      return true;
    } else if ((thread1.slice == thread2.slice) &&
               (thread1.subslice < thread2.subslice)) {
      return true;
    } else if ((thread1.slice == thread2.slice) &&
               (thread1.subslice == thread2.subslice) &&
               (thread1.eu < thread2.eu)) {
      return true;
    } else if ((thread1.slice == thread2.slice) &&
               (thread1.subslice == thread2.subslice) &&
               (thread1.eu == thread2.eu) &&
               (thread1.thread < thread2.thread)) {
      return true;
    }

    return false;
  }
};

bool static address_valid = false;
void wait_for_events_interrupt_and_resume(
    const zet_debug_session_handle_t &debugSession, process_synchro *synchro,
    uint64_t *gpu_buffer_va, ze_device_handle_t device);

#endif // TEST_DEBUG_UTILS_HPP
