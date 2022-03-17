/*
 *
 * Copyright (C) 2021 Intel Corporation
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

namespace fs = boost::filesystem;
namespace bp = boost::process;
namespace bi = boost::interprocess;

namespace {

TEST(
    zetDeviceGetDebugPropertiesTest,
    GivenValidDeviceWhenGettingDebugPropertiesThenPropertiesReturnedSuccessfully) {
  auto driver = lzt::get_default_driver();

  for (auto device : lzt::get_devices(driver)) {
    zetDebugBaseSetup::is_debug_supported(device);
  }
}

TEST(
    zetDeviceGetDebugPropertiesTest,
    GivenSubDeviceWhenGettingDebugPropertiesThenPropertiesReturnedSuccessfully) {
  auto driver = lzt::get_default_driver();

  for (auto &device : lzt::get_devices(driver)) {
    for (auto &sub_device : lzt::get_ze_sub_devices(device)) {
      zetDebugBaseSetup::is_debug_supported(device);
    }
  }
}

class zetDebugAttachDetachTest : public zetDebugBaseSetup {
protected:
  void run_test(std::vector<ze_device_handle_t> devices, bool use_sub_devices,
                bool reattach);
};

void zetDebugAttachDetachTest::run_test(std::vector<ze_device_handle_t> devices,
                                        bool use_sub_devices, bool reattach) {

  for (auto &device : devices) {
    if (!is_debug_supported(device))
      continue;

    auto debug_helper = launch_process(BASIC, device, use_sub_devices);
    zet_debug_config_t debug_config = {};
    debug_config.pid = debug_helper.id();

    if (!reattach) {
      auto debug_session = lzt::debug_attach(device, debug_config);
      if (!debug_session) {
        FAIL() << "[Debugger] Failed to attach to start a debug session";
      }
      synchro->notify_attach();
      LOG_INFO << "[Debugger] Detaching";
      lzt::debug_detach(debug_session);
      debug_helper.wait(); // we don't care about the child processes exit code
                           // at the moment
    } else {
      int loop = 1;
      for (loop = 1; loop < 11; loop++) {
        LOG_INFO << "[Debugger] Attaching. Loop " << loop;
        auto debug_session = lzt::debug_attach(device, debug_config);
        if (!debug_session) {
          FAIL()
              << "[Debugger] Failed to attach to start a debug session. Loop "
              << loop;
        }
        // delay last detach to happen after application finishes
        if (loop < 10) {
          LOG_INFO << "[Debugger] Detaching. Loop " << loop;
          lzt::debug_detach(debug_session);
        } else {
          synchro->notify_attach();
          debug_helper
              .terminate(); // we don't care about the child processes exit code
          LOG_INFO << "[Debugger] LAST Detach after aplication finished. Loop "
                   << loop;
          lzt::debug_detach(debug_session);
        }
      }
    }
  }
}

TEST_F(
    zetDebugAttachDetachTest,
    GivenDeviceSupportsDebugAttachWhenAttachingThenAttachAndDetachIsSuccessful) {

  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);
  run_test(devices, false, false);
}

TEST_F(
    zetDebugAttachDetachTest,
    GivenSubDeviceSupportsDebugAttachWhenAttachingThenAttachAndDetachIsSuccessful) {

  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);

  std::vector<ze_device_handle_t> all_sub_devices;
  for (auto &device : devices) {
    auto sub_devices = lzt::get_ze_sub_devices(device);
    all_sub_devices.insert(all_sub_devices.end(), sub_devices.begin(),
                           sub_devices.end());
  }

  run_test(all_sub_devices, true, false);
}

TEST_F(zetDebugAttachDetachTest,
       GivenPreviousDebugSessionDetachedWenAttachingThenReAttachIsSuccessful) {

  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);

  run_test(devices, false, true);
}

class zetDebugEventReadTest : public zetDebugAttachDetachTest {
protected:
  void SetUp() override { zetDebugAttachDetachTest::SetUp(); }
  void TearDown() override { zetDebugAttachDetachTest::TearDown(); }
  void run_test(std::vector<ze_device_handle_t> devices, bool use_sub_devices,
                debug_test_type_t test_type);
  void run_advanced_test(std::vector<ze_device_handle_t> devices,
                         bool use_sub_devices, debug_test_type_t test_type,
                         num_threads_t threads = SINGLE_THREAD);
};

void zetDebugEventReadTest::run_test(std::vector<ze_device_handle_t> devices,
                                     bool use_sub_devices,
                                     debug_test_type_t test_type) {
  for (auto &device : devices) {

    if (!is_debug_supported(device))
      continue;
    auto debug_helper = launch_process(test_type, device, use_sub_devices);

    zet_debug_config_t debug_config = {};
    debug_config.pid = debug_helper.id();
    auto debug_session = lzt::debug_attach(device, debug_config);
    if (!debug_session) {
      FAIL() << "[Debugger] Failed to attach to start a debug session";
    }

    synchro->notify_attach();
    LOG_INFO << "[Debugger] Listening for events";

    uint16_t eventNum = 0;
    uint16_t moduleLoadCount = 0;
    uint16_t moduleUnloadCount = 0;
    bool gotProcEntry = false;
    bool gotProcExit = false;

    std::chrono::time_point<std::chrono::system_clock> start, checkpoint;
    start = std::chrono::system_clock::now();

    zet_debug_event_t debug_event;
    do {
      ze_result_t result = lzt::debug_read_event(debug_session, debug_event,
                                                 eventsTimeoutMS, false);
      if (ZE_RESULT_SUCCESS != result) {
        break;
      }
      LOG_INFO << "[Debugger] received event: "
               << lzt::debuggerEventTypeString[debug_event.type];
      eventNum++;

      if (ZET_DEBUG_EVENT_TYPE_PROCESS_ENTRY == debug_event.type) {
        EXPECT_EQ(eventNum, 1);
        gotProcEntry = true;
      } else if (ZET_DEBUG_EVENT_TYPE_MODULE_LOAD == debug_event.type) {
        EXPECT_GT(eventNum, 1);
        moduleLoadCount++;
        EXPECT_TRUE(debug_event.flags & ZET_DEBUG_EVENT_FLAG_NEED_ACK);
        if (debug_event.flags & ZET_DEBUG_EVENT_FLAG_NEED_ACK) {
          lzt::debug_ack_event(debug_session, &debug_event);
        }
      } else if (ZET_DEBUG_EVENT_TYPE_MODULE_UNLOAD == debug_event.type) {
        EXPECT_GT(eventNum, 1);
        moduleUnloadCount++;
      } else if (ZET_DEBUG_EVENT_TYPE_PROCESS_EXIT == debug_event.type) {
        EXPECT_GT(eventNum, 1);
        gotProcExit = true;
        break;
      }

      checkpoint = std::chrono::system_clock::now();
      std::chrono::duration<double> secondsLooping = checkpoint - start;
      if (secondsLooping.count() > eventsTimeoutS) {
        LOG_ERROR << "[Debugger] Timed out waiting for events";
        break;
      }
    } while (ZET_DEBUG_EVENT_TYPE_INVALID != debug_event.type);

    EXPECT_GE(moduleLoadCount, 1);
    EXPECT_EQ(moduleLoadCount, moduleUnloadCount);
    EXPECT_TRUE(gotProcEntry);
    EXPECT_TRUE(gotProcExit);

    lzt::debug_detach(debug_session);
    debug_helper.wait();
  }
}

TEST_F(
    zetDebugEventReadTest,
    GivenDebugCapableDeviceWhenProcessIsBeingDebuggedThenCorrectEventsAreRead) {

  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);
  run_test(devices, false, BASIC);
}

TEST_F(
    zetDebugEventReadTest,
    GivenDebugCapableSubDeviceWhenProcessIsBeingDebuggedThenCorrectEventsAreRead) {

  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);

  std::vector<ze_device_handle_t> all_sub_devices;
  for (auto &device : devices) {
    auto sub_devices = lzt::get_ze_sub_devices(device);
    all_sub_devices.insert(all_sub_devices.end(), sub_devices.begin(),
                           sub_devices.end());
  }

  run_test(all_sub_devices, true, BASIC);
}

TEST_F(zetDebugEventReadTest,
       GivenDeviceWhenCreatingMultipleModulesThenMultipleEventsReceived) {

  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);
  run_test(devices, false, MULTIPLE_MODULES_CREATED);
}

typedef enum {
  SAVING_SINGLE_THREAD,
  WAITING_SINGLE_THREAD,
  SAVING_MULTIPLE_THREADS,
  WAITING_MULTIPLE_THREADS,
  WAITING_FOR_ALL_STOP_EVENT,
  TIMEOUT_SAVE_THREADS,
  TIMEOUT_VERIFY_THREADS
} interrupt_test_state_t;

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
            threads.push_back(device_thread);
          }
        }
      }
    }
  }

  return threads;
}

void print_thread(const char *entry_message,
                  const ze_device_thread_t &device_thread) {
  LOG_DEBUG << entry_message << "SLICE:" << device_thread.slice
            << " SUBSLICE: " << device_thread.subslice
            << " EU: " << device_thread.eu
            << " THREAD: " << device_thread.thread;
}

// wait for stopped thread event and retunrn stopped threads
bool find_stopped_threads(const zet_debug_session_handle_t &debug_session,
                          const ze_device_handle_t &device,
                          std::vector<ze_device_thread_t> &threads) {
  uint8_t attempts = 0;
  zet_debug_event_t debug_event = {};
  threads.clear();
  do {
    lzt::debug_read_event(debug_session, debug_event, eventsTimeoutMS / 10,
                          true);
    LOG_INFO << "[Debugger] received event: "
             << lzt::debuggerEventTypeString[debug_event.type];

    if (debug_event.type == ZET_DEBUG_EVENT_TYPE_THREAD_STOPPED) {
      print_thread("[Debugger] Stopped thread event for ",
                   debug_event.info.thread.thread);
      threads = get_stopped_threads(debug_session, device);
      break;
    }
    attempts++;
  } while (attempts < 5);

  if (threads.size() > 0) {
    return true;
  } else {
    return false;
  }
}

bool unique_thread(const ze_device_thread_t &device_thread) {
  print_thread("[Debugger] is thread unique: ", device_thread);
  return (device_thread.slice != UINT32_MAX &&
          device_thread.subslice != UINT32_MAX &&
          device_thread.eu != UINT32_MAX && device_thread.thread != UINT32_MAX);
}

int interrupt_test(zet_debug_session_handle_t &debug_session,
                   zet_debug_event_t &debug_event, num_threads_t threads,
                   bp::child &debug_helper, uint64_t &timeout,
                   uint32_t &timeout_count, const ze_device_handle_t &device) {

  int result = 0;
  static interrupt_test_state_t interrupt_test_state = SAVING_SINGLE_THREAD;
  static ze_device_thread_t saved_thread = {};
  static std::vector<ze_device_thread_t> saved_threads;
  static std::vector<ze_device_thread_t> stopped_threads;
  std::vector<ze_device_thread_t> stopped_threads_initial,
      stopped_threads_final;
  if (timeout_count > timeoutThreshold) {
    LOG_DEBUG << "[Debugger] Timeout count exceeds threshold";
    if (TIMEOUT_SAVE_THREADS == interrupt_test_state) {
      LOG_DEBUG << "New State: TIMEOUT_VERIFY_THREADS";
      interrupt_test_state = TIMEOUT_VERIFY_THREADS;
      stopped_threads_final = get_stopped_threads(debug_session, device);
    } else if (TIMEOUT_VERIFY_THREADS == interrupt_test_state) {
      ADD_FAILURE() << "Test failed due to time outs";
      result = 1;
      return result;
    } else {
      LOG_DEBUG << "[Debugger] New State: TIMEOUT_SAVE_THREADS";
      interrupt_test_state = TIMEOUT_SAVE_THREADS;
      timeout_count = 0;
      stopped_threads_initial = get_stopped_threads(debug_session, device);
      LOG_DEBUG << "[Debugger] Stopped Threads: "
                << stopped_threads_initial.size();
    }
  }

  if (TIMEOUT_SAVE_THREADS == interrupt_test_state) {
    if (stopped_threads_initial.size() > 0) {
      LOG_DEBUG << "[Debugger] List of stopped threads is populated";

      std::this_thread::sleep_for(std::chrono::seconds(2));
      switch (threads) {
      case SINGLE_THREAD: {
        // resume thread from list of stopped threads
        print_thread("[Debugger] Updating and resuming thread ",
                     stopped_threads_initial[0]);
        saved_thread = stopped_threads_initial[0];
        lzt::debug_resume(debug_session, stopped_threads_initial[0]);

        std::this_thread::sleep_for(std::chrono::seconds(5));
        auto regset_properties = lzt::get_register_set_properties(device);
        if (read_register(debug_session, saved_thread, regset_properties[2],
                          true)) {
          LOG_DEBUG << "[Debugger] Thread Still Stopped";
        } else {
          LOG_DEBUG << "[Debugger] Thread was resumed";
        }
        LOG_DEBUG << "[Debugger] Interrupting Thread Again";
        lzt::debug_interrupt(debug_session, saved_thread);

        break;
      }
      case GROUP_OF_THREADS: {
        std::map<int, int> eu_map, slice_map, subslice_map, thread_map;
        for (auto &temp_thread : stopped_threads_initial) {
          eu_map[temp_thread.eu]++;
          slice_map[temp_thread.slice]++;
          subslice_map[temp_thread.subslice]++;
          thread_map[temp_thread.thread]++;
        }

        uint32_t slice_field = UINT32_MAX;
        uint32_t subslice_field = UINT32_MAX;
        uint32_t eu_field = UINT32_MAX;
        uint32_t thread_field = UINT32_MAX;
        bool field_selected = false;

        for (auto &subslice : subslice_map) {
          if (subslice.second > 1) {
            LOG_DEBUG << "Selected subslice";

            field_selected = true;
            subslice_field = subslice.first;
            break;
          }
        }

        if (!field_selected) {
          for (auto &eu : eu_map) {
            if (eu.second > 1) {
              LOG_DEBUG << "Selected eu";

              field_selected = true;
              eu_field = eu.first;
              break;
            }
          }
        }

        if (!field_selected) {
          for (auto &thread : thread_map) {
            if (thread.second > 1) {
              LOG_DEBUG << "Selected thread";

              field_selected = true;
              thread_field = thread.first;
              break;
            }
          }
        }

        if (!field_selected) {
          for (auto slice : slice_map) {
            if (slice.second > 1) {
              LOG_DEBUG << "Selected slice";
              field_selected = true;
              slice_field = slice.first;
              break;
            }
          }
        }

        if (!field_selected) {
          ADD_FAILURE() << "Could not determine group of threads to interrupt";
          result = 1;
          break;
        }

        // go through list of stopped threads to determine which ones to save
        for (auto &thread : stopped_threads_initial) {
          if ((slice_field != UINT32_MAX && thread.slice == slice_field) ||
              (subslice_field != UINT32_MAX &&
               thread.subslice == subslice_field) ||
              (eu_field != UINT32_MAX && thread.eu == eu_field) ||
              (thread_field != UINT32_MAX && thread.thread == thread_field)) {
            saved_threads.push_back(thread);
          }
        }
        LOG_DEBUG << "[Debugger] Saved " << saved_threads.size() << " threads";
        LOG_DEBUG << "[Debugger] Reusming several threads";

        ze_device_thread_t device_threads = {};
        device_threads.slice = slice_field;
        device_threads.subslice = subslice_field;
        device_threads.eu = eu_field;
        device_threads.thread = thread_field;

        lzt::debug_resume(debug_session, device_threads);
        std::this_thread::sleep_for(std::chrono::seconds(10));

        LOG_DEBUG << "[Debugger] Interrupting several threads";
        lzt::debug_interrupt(debug_session, device_threads);

        break;
      }
      case ALL_THREADS: {
        LOG_DEBUG << "[Debugger] Saving all stopped threads";

        for (auto &thread : stopped_threads_initial) {
          saved_threads.push_back(thread);
        }
        ze_device_thread_t device_threads = {};
        device_threads.slice = UINT32_MAX;
        device_threads.subslice = UINT32_MAX;
        device_threads.eu = UINT32_MAX;
        device_threads.thread = UINT32_MAX;
        lzt::debug_resume(debug_session, device_threads);

        std::this_thread::sleep_for(std::chrono::seconds(10));
        LOG_DEBUG << "[Debugger] Interrupting all threads";

        lzt::debug_interrupt(debug_session, device_threads);

        break;
      }
      default:
        break;
      }
    } else {
      LOG_DEBUG << "[Debugger] Waiting for threads to stop";
    }
  } else if (TIMEOUT_VERIFY_THREADS == interrupt_test_state) {
    switch (threads) {
    case SINGLE_THREAD: {
      LOG_DEBUG << "Searching for saved thread";
      auto found_thread = false;
      for (auto &thread : stopped_threads_final) {
        if (thread == saved_thread) {
          found_thread = true;
          LOG_DEBUG << "Thread Found";
          break;
        }
      }

      EXPECT_TRUE(found_thread) << "Failed to get stop event for saved thread";
      result = 1;
      break;
    }
    case GROUP_OF_THREADS: {
      LOG_DEBUG << "[Debugger] Searching for " << saved_threads.size()
                << " saved threads";
      LOG_DEBUG << "[Debugger] Stopped threads: "
                << stopped_threads_final.size();

      auto thread_missing = false;
      for (auto &thread : saved_threads) {
        if (std::find(stopped_threads_final.begin(),
                      stopped_threads_final.end(),
                      thread) == stopped_threads_final.end()) {
          thread_missing = true;
        }

        if (thread_missing) {
          ADD_FAILURE() << "A thread was not stopped";
          break;
        }
      }
      saved_threads.clear();
      result = 1;
      break;
    }
    case ALL_THREADS: {
      LOG_DEBUG << "[Debugger] Searching for saved threads";
      auto thread_missing = false;
      for (auto &temp_thread : stopped_threads_final) {

        if (std::find(saved_threads.begin(), saved_threads.end(),
                      temp_thread) == saved_threads.end()) {
          thread_missing = true;
        }

        if (thread_missing) {
          ADD_FAILURE() << "A thread was not stopped";
          break;
        }
      }
      saved_threads.clear();
      result = 1;
      break;
    }
    default:
      break;
    }
    interrupt_test_state = SAVING_SINGLE_THREAD;
  }

  switch (debug_event.type) {
  case ZET_DEBUG_EVENT_TYPE_MODULE_LOAD: {
    ze_device_thread_t device_threads = {};
    device_threads.slice = UINT32_MAX;
    device_threads.subslice = UINT32_MAX;
    device_threads.eu = UINT32_MAX;
    device_threads.thread = UINT32_MAX;
    switch (threads) {
    case SINGLE_THREAD: // we don't know which threads are running, so ...
                        // stop all threads
                        // wait for stop event
                        // save that thread from stop event,
      LOG_DEBUG << "New State: SAVING SINGLE THREAD";
      interrupt_test_state = SAVING_SINGLE_THREAD;
      break;
    case GROUP_OF_THREADS:
      LOG_DEBUG << "New State: SAVING MULTIPLE THREADS";
      interrupt_test_state =
          SAVING_MULTIPLE_THREADS; // we need to save several threads
      break;
    case ALL_THREADS:
      LOG_DEBUG << "New State: WAITING FOR ALL STOP EVENT";
      interrupt_test_state = WAITING_FOR_ALL_STOP_EVENT;
      break;
    default:
      break;
    }

    std::this_thread::sleep_for(std::chrono::seconds(5));
    LOG_DEBUG << "[Debugger] Interrupting All threads";
    lzt::debug_interrupt(debug_session, device_threads);
    timeout = 5000;
    break;
  }

  case ZET_DEBUG_EVENT_TYPE_THREAD_STOPPED: {
    LOG_DEBUG << "[Debugger] Thread Stop Event Received";
    // depending on the state we are in, either save this stopped thread, or
    // end test as success
    switch (interrupt_test_state) {
    case SAVING_SINGLE_THREAD: {
      LOG_DEBUG << "[Debugger] Saving Single Thread";
      if (unique_thread(debug_event.info.thread.thread)) {
        LOG_DEBUG << "[Debugger] Updating Saved Thread";
        saved_thread = debug_event.info.thread.thread;

        LOG_DEBUG << "[Debugger] Resuming saved thread in order to interrupt";
        lzt::debug_resume(debug_session, saved_thread);
        std::this_thread::sleep_for(std::chrono::seconds(2));

        LOG_DEBUG << "[Debugger] Interrupting saved thread";
        lzt::debug_interrupt(debug_session, saved_thread);

        LOG_DEBUG << "[Debugger] New State: WAITING SINGLE THREAD";
        interrupt_test_state = WAITING_SINGLE_THREAD;
      }
      break;
    }
    case SAVING_MULTIPLE_THREADS: {
      if (unique_thread(debug_event.info.thread.thread)) {
        auto temp = debug_event.info.thread.thread;
        LOG_INFO << "[Debugger] Thread stopped [slice, subslice, eu, thread]: ["
                 << temp.slice << ", " << temp.subslice << ", " << temp.eu
                 << ", " << temp.thread << "]";

        if (saved_threads.size()) {
          if (temp.subslice == saved_threads[0].subslice) {
            saved_threads.push_back(temp);
          }
        }

        if (saved_threads.size() > 1) {
          ze_device_thread_t device_threads = {};
          device_threads.slice = UINT32_MAX;
          device_threads.subslice = temp.subslice;
          device_threads.eu = UINT32_MAX;
          device_threads.thread = UINT32_MAX;
          lzt::debug_resume(debug_session, device_threads);

          std::this_thread::sleep_for(std::chrono::seconds(5));

          lzt::debug_interrupt(debug_session, device_threads);

          LOG_DEBUG << "[Debugger] New State: WAITING MULTIPLE THREADS";
          interrupt_test_state = WAITING_MULTIPLE_THREADS;
        }
      }
      break;
    }
    case WAITING_SINGLE_THREAD: {
      auto temp_thread = debug_event.info.thread.thread;
      if (temp_thread.slice == saved_thread.slice &&
          temp_thread.subslice == saved_thread.subslice &&
          temp_thread.eu == saved_thread.eu &&
          temp_thread.thread == saved_thread.thread) {

        debug_helper.terminate();
        // test successful, exit
        lzt::debug_detach(debug_session);
        result = 1;
      }
      break;
    }
    case WAITING_MULTIPLE_THREADS: {
      auto temp_thread = debug_event.info.thread.thread;
      auto thread_it =
          std::find(saved_threads.begin(), saved_threads.end(), temp_thread);
      if (thread_it != saved_threads.end()) {

        saved_threads.erase(thread_it);

        if (saved_threads.empty()) {
          LOG_DEBUG
              << "[Debugger] All previously saved threads were stopped again";
          debug_helper.terminate();
          lzt::debug_detach(debug_session);
          result = 1;
        }
      }

      break;
    }
    case WAITING_FOR_ALL_STOP_EVENT: {

      auto stopped_thread = debug_event.info.thread.thread;
      if (stopped_thread.eu == UINT32_MAX &&
          stopped_thread.slice == UINT32_MAX &&
          stopped_thread.subslice == UINT32_MAX &&
          stopped_thread.thread == UINT32_MAX) {
        LOG_DEBUG << "[Debugger] Received All Thread Stop Event";
      }
      break;
    }
    default:
      break;
    }
    break;
  }

  case ZET_DEBUG_EVENT_TYPE_THREAD_UNAVAILABLE: {
    LOG_ERROR << "[Debugger] Received Thread Unavailable Event";
    break;
  }

  default: { break; }
  }
  return result;
}

void zetDebugEventReadTest::run_advanced_test(
    std::vector<ze_device_handle_t> devices, bool use_sub_devices,
    debug_test_type_t test_type, num_threads_t threads) {

  std::string test_options = "";

  for (auto &device : devices) {
    if (!is_debug_supported(device))
      continue;
    auto debug_helper = launch_process(test_type, device, use_sub_devices);

    zet_debug_config_t debug_config = {};
    debug_config.pid = debug_helper.id();
    auto debug_session = lzt::debug_attach(device, debug_config);
    if (!debug_session) {
      FAIL() << "[Debugger] Failed to attach to start a debug session";
    }

    synchro->notify_attach();

    std::vector<zet_debug_event_type_t> events;
    uint64_t timeout = std::numeric_limits<uint64_t>::max();
    auto event_num = 0;
    std::chrono::time_point<std::chrono::system_clock> start, checkpoint;
    start = std::chrono::system_clock::now();

    // debug event loop
    uint32_t timeout_count = 0;
    auto end_test = false;
    uint32_t thread_unavailable_event_count = 0;
    while (true) {
      zet_debug_event_t debug_event;
      ze_result_t result = lzt::debug_read_event(debug_session, debug_event,
                                                 eventsTimeoutMS / 10, true);
      if (ZE_RESULT_SUCCESS != result) {
        if (ZE_RESULT_NOT_READY == result) {
          timeout_count++;
        } else {
          break;
        }
      } else {
        LOG_INFO << "[Debugger] received event: "
                 << lzt::debuggerEventTypeString[debug_event.type];

        if (debug_event.type == ZET_DEBUG_EVENT_TYPE_THREAD_STOPPED) {
          print_thread("[Debugger] Stopped thread event for ",
                       debug_event.info.thread.thread);
        }
        events.push_back(debug_event.type);

        if (debug_event.flags & ZET_DEBUG_EVENT_FLAG_NEED_ACK) {
          LOG_DEBUG << "[Debugger] Acking event: "
                    << lzt::debuggerEventTypeString[debug_event.type];
          lzt::debug_ack_event(debug_session, &debug_event);
        }
      }

      switch (test_type) {
      case LONG_RUNNING_KERNEL_INTERRUPTED: {
        if (::testing::Test::HasFailure()) {
          debug_helper.terminate();
          lzt::debug_detach(debug_session);
          FAIL() << "[Debugger] Failed to receive either stop or unavailable "
                    "result after "
                    "interrupting threads";
        } else {
          auto result =
              interrupt_test(debug_session, debug_event, threads, debug_helper,
                             timeout, timeout_count, device);
          if (result) {
            end_test = true;
          }
        }
        break;
      }
      case THREAD_UNAVAILABLE: {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        // attempt to stop a thread that is not running
        ze_device_thread_t device_thread = {};
        device_thread.slice = 0;
        device_thread.subslice = 0;
        device_thread.eu = 0;
        device_thread.thread = 0;
        lzt::debug_interrupt(debug_session, device_thread);
        switch (debug_event.type) {
        case ZET_DEBUG_EVENT_TYPE_THREAD_UNAVAILABLE:
          thread_unavailable_event_count++;
          end_test = true;
          break;
        }
      }
      default: {
        checkpoint = std::chrono::system_clock::now();
        std::chrono::duration<double> secondsLooping = checkpoint - start;
        if (secondsLooping.count() > 30) {
          LOG_ERROR << "[Debugger] Timed out waiting for events";
          end_test = true;
          break;
        }
      }
      }

      if ((ZET_DEBUG_EVENT_TYPE_PROCESS_EXIT == debug_event.type) || end_test) {
        break;
      }
    }

    // cleanup
    LOG_INFO << "[Debugger] terminating application to finish";
    debug_helper.terminate();
    std::this_thread::sleep_for(std::chrono::seconds(1));
    LOG_INFO << "[Debugger] detaching";
    lzt::debug_detach(debug_session);
    if (test_type == THREAD_UNAVAILABLE) {
      ASSERT_EQ(thread_unavailable_event_count, 1)
          << "Number of ZET_DEBUG_EVENT_TYPE_THREAD_UNAVAILABLE events is not "
             "1";
    }
  }
}

TEST_F(zetDebugEventReadTest,
       GivenDeviceWhenThenAttachingAfterModuleCreatedThenEventReceived) {

  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);

  for (auto &device : devices) {

    if (!is_debug_supported(device))
      continue;

    auto debug_helper =
        launch_process(ATTACH_AFTER_MODULE_CREATED, device, false);
    zet_debug_config_t debug_config = {};
    debug_config.pid = debug_helper.id();

    synchro->wait_for_application();

    auto debug_session = lzt::debug_attach(device, debug_config);
    if (!debug_session) {
      FAIL() << "[Debugger] Failed to attach to start a debug session";
    }

    synchro->notify_attach();

    auto event_found = false;
    while (true) {
      zet_debug_event_t debug_event;
      ze_result_t result = lzt::debug_read_event(debug_session, debug_event,
                                                 eventsTimeoutMS, false);
      if (ZE_RESULT_SUCCESS != result) {
        break;
      }

      if (ZET_DEBUG_EVENT_TYPE_MODULE_LOAD == debug_event.type) {
        event_found = true;
      }

      if (debug_event.flags & ZET_DEBUG_EVENT_FLAG_NEED_ACK) {
        lzt::debug_ack_event(debug_session, &debug_event);
      }

      if (ZET_DEBUG_EVENT_TYPE_PROCESS_EXIT == debug_event.type) {
        break;
      }
    }
    lzt::debug_detach(debug_session);
    debug_helper.wait();

    ASSERT_TRUE(event_found);
  }
}

TEST_F(
    zetDebugEventReadTest,
    GivenDebugEnabledDeviceWhenAttachingAfterCreatingAndDestroyingModuleThenNoModuleEventReceived) {

  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);

  run_advanced_test(devices, false, ATTACH_AFTER_MODULE_DESTROYED);
}

TEST_F(
    zetDebugEventReadTest,
    GivenDebugAttachedWhenResumingAndInterruptingKernelSingleThreadThenStopEventReceivedAndThreadStopped) {

  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);

  run_advanced_test(devices, false, LONG_RUNNING_KERNEL_INTERRUPTED,
                    SINGLE_THREAD);
}

TEST_F(
    zetDebugEventReadTest,
    GivenDebugAttachedWhenResumingAndInterruptingKernelGroupOfThreadsThenStopEventReceivedAndThreadsStopped) {

  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);

  run_advanced_test(devices, false, LONG_RUNNING_KERNEL_INTERRUPTED,
                    GROUP_OF_THREADS);
}

TEST_F(
    zetDebugEventReadTest,
    GivenDebugAttachedWhenResumingAndInterruptingKernelAllThreadsThenStopEventReceivedAndThreadsStopped) {

  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);

  run_advanced_test(devices, false, LONG_RUNNING_KERNEL_INTERRUPTED,
                    ALL_THREADS);
}

TEST_F(zetDebugEventReadTest,
       GivenThreadUnavailableWhenDebugEnabledThenThreadUnavailableEventRead) {

  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);

  run_advanced_test(devices, false, THREAD_UNAVAILABLE);
}

class zetDebugMemAccessTest : public zetDebugAttachDetachTest {
protected:
  void SetUp() override { zetDebugAttachDetachTest::SetUp(); }
  void TearDown() override { zetDebugAttachDetachTest::TearDown(); }
  void attachAndGetModuleEvent(uint32_t pid, ze_device_handle_t device,
                               zet_debug_event_t &module_event);
  void readWriteModuleMemory(const zet_debug_session_handle_t &debug_session,
                             const ze_device_thread_t &thread,
                             zet_debug_event_t &module_event);

  zet_debug_session_handle_t debug_session;
  static constexpr uint8_t bufferSize = 16;
};

void zetDebugMemAccessTest::attachAndGetModuleEvent(
    uint32_t pid, ze_device_handle_t device, zet_debug_event_t &module_event) {

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

void zetDebugMemAccessTest::readWriteModuleMemory(
    const zet_debug_session_handle_t &debug_session,
    const ze_device_thread_t &thread, zet_debug_event_t &module_event) {

  zet_debug_memory_space_desc_t desc;
  desc.type = ZET_DEBUG_MEMORY_SPACE_TYPE_DEFAULT;
  uint8_t buffer[bufferSize];
  memset(buffer, 0xaa, bufferSize);

  // Access ISA
  desc.address = module_event.info.module.load;
  lzt::debug_read_memory(debug_session, thread, desc, bufferSize, buffer);
  for (int i = 0; i < bufferSize; i++) {
    EXPECT_NE(static_cast<char>(0xaa), buffer[i]);
  }
  memset(buffer, 0xaa, bufferSize);

  desc.address += 0xF; // add intentional missalignment
  lzt::debug_read_memory(debug_session, thread, desc, bufferSize, buffer);
  for (int i = 0; i < bufferSize; i++) {
    EXPECT_NE(static_cast<char>(0xaa), buffer[i]);
  }

  uint8_t bufferCopy[bufferSize];
  memcpy(bufferCopy, buffer, bufferSize);

  lzt::debug_write_memory(debug_session, thread, desc, bufferSize, buffer);

  // Confirm reading again returns the original content
  memset(buffer, 0xaa, bufferSize);
  lzt::debug_read_memory(debug_session, thread, desc, bufferSize, buffer);
  EXPECT_FALSE(
      memcmp(bufferCopy, buffer, bufferSize)); // memcmp retruns 0 on equal

  // Access ELF
  if (module_event.info.module.moduleBegin) {
    desc.address = module_event.info.module.moduleBegin;
    lzt::debug_read_memory(debug_session, thread, desc, bufferSize, buffer);
    for (int i = 0; i < bufferSize; i++) {
      EXPECT_NE(static_cast<char>(0xaa), buffer[i]);
    }
    memset(buffer, 0xaa, bufferSize);

    desc.address += 0xF; // add intentional missalignment
    lzt::debug_read_memory(debug_session, thread, desc, bufferSize, buffer);
    for (int i = 0; i < bufferSize; i++) {
      EXPECT_NE(static_cast<char>(0xaa), buffer[i]);
    }
    // NO writing allowed to ELF
  }
}

TEST_F(zetDebugMemAccessTest,
       GivenDebuggerAttachedAndModuleLoadedAccessISAAndELFMemory) {
  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);

  for (auto &device : devices) {

    if (!is_debug_supported(device))
      continue;

    auto debug_helper = launch_process(BASIC, device, false);
    zet_debug_event_t module_event;
    attachAndGetModuleEvent(debug_helper.id(), device, module_event);
    CLEAN_AND_ASSERT(module_event.info.module.load, debug_session,
                     debug_helper);

    // ALL threads
    ze_device_thread_t thread;
    thread.slice = UINT32_MAX;
    thread.subslice = UINT32_MAX;
    thread.eu = UINT32_MAX;
    thread.thread = UINT32_MAX;
    readWriteModuleMemory(debug_session, thread, module_event);

    lzt::debug_detach(debug_session);
    debug_helper.terminate();
  }
}

TEST_F(
    zetDebugMemAccessTest,
    GivenDebuggerAttachedAndModuleLoadedWhenThreadStoppedThenDebuggerCanReadWriteToAllocatedBuffersInTheContextOfThread) {
  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);

  for (auto &device : devices) {
    if (!is_debug_supported(device))
      continue;

    auto debug_helper =
        launch_process(LONG_RUNNING_KERNEL_INTERRUPTED, device, false);
    zet_debug_event_t module_event;
    attachAndGetModuleEvent(debug_helper.id(), device, module_event);
    CLEAN_AND_ASSERT(module_event.info.module.load, debug_session,
                     debug_helper);

    if (module_event.flags & ZET_DEBUG_EVENT_FLAG_NEED_ACK) {
      LOG_DEBUG << "[Debugger] Acking event: "
                << lzt::debuggerEventTypeString[module_event.type];
      lzt::debug_ack_event(debug_session, &module_event);
    }

    uint64_t gpu_buffer_va = 0;
    synchro->wait_for_application();
    if (!synchro->get_app_gpu_buffer_address(gpu_buffer_va)) {
      FAIL() << "[Debugger] Could not get a valid GPU buffer VA";
      debug_helper.terminate();
      lzt::debug_detach(debug_session);
    }
    LOG_INFO << "[Debugger] Accessing application GPU buffer VA: " << std::hex
             << gpu_buffer_va;

    ze_device_thread_t thread;
    thread.slice = UINT32_MAX;
    thread.subslice = UINT32_MAX;
    thread.eu = UINT32_MAX;
    thread.thread = UINT32_MAX;

    std::this_thread::sleep_for(std::chrono::seconds(5));
    LOG_INFO << "[Debugger] Interrupting all threads";
    lzt::debug_interrupt(debug_session, thread);
    std::vector<ze_device_thread_t> stopped_threads;
    if (find_stopped_threads(debug_session, device, stopped_threads)) {

      zet_debug_memory_space_desc_t desc;
      desc.type = ZET_DEBUG_MEMORY_SPACE_TYPE_DEFAULT;
      uint64_t sizeToRead = 512;
      uint8_t *buffer = (uint8_t *)malloc(sizeToRead);

      desc.address = gpu_buffer_va;

      LOG_INFO << "[Debugger] Reading/Writing on interrupted threads";
      for (auto &stopped_thread : stopped_threads) {
        print_thread("[Debugger] Reading and writting from Stopped thread ",
                     stopped_thread);

        readWriteModuleMemory(debug_session, stopped_thread, module_event);

        lzt::debug_read_memory(debug_session, stopped_thread, desc, sizeToRead,
                               buffer);

        // set buffer[0] to 0 to break the loop. Seel debug_loop.cl
        buffer[0] = 0;
        lzt::debug_write_memory(debug_session, thread, desc, sizeToRead,
                                buffer);
      }

      LOG_INFO << "[Debugger] resuming interrupted threads";
      lzt::debug_resume(debug_session, thread);
      free(buffer);
    } else {
      FAIL() << "[Debugger] Could not find a stopped thread";
    }

    debug_helper.wait();
    ASSERT_EQ(debug_helper.exit_code(), 0);
    lzt::debug_detach(debug_session);
  }
}

TEST(zetDebugRegisterSetTest,
     GivenDeviceWhenGettingRegisterSetPropertiesThenValidPropertiesReturned) {

  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);

  for (auto &device : devices) {
    auto properties = lzt::get_register_set_properties(device);

    zet_debug_regset_properties_t empty_properties = {};
    for (auto &property : properties) {
      EXPECT_NE(memcmp(&property, &empty_properties,
                       sizeof(zet_debug_regset_properties_t)),
                0);
    }
  }
}

class zetDebugReadWriteRegistersTest : public zetDebugMemAccessTest {
protected:
  void SetUp() override { zetDebugMemAccessTest::SetUp(); }
  void TearDown() override { zetDebugMemAccessTest::TearDown(); }
};

TEST_F(
    zetDebugReadWriteRegistersTest,
    GivenActiveDebugSessionWhenReadingAndWritingRegistersThenValidDataReadAndDataWrittenSuccessfully) {
  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);

  for (auto &device : devices) {

    if (!is_debug_supported(device))
      continue;
    auto debug_helper =
        launch_process(LONG_RUNNING_KERNEL_INTERRUPTED, device, false);

    zet_debug_event_t module_event;
    attachAndGetModuleEvent(debug_helper.id(), device, module_event);
    CLEAN_AND_ASSERT(module_event.info.module.load, debug_session,
                     debug_helper);

    if (module_event.flags & ZET_DEBUG_EVENT_FLAG_NEED_ACK) {
      LOG_DEBUG << "[Debugger] Acking event: "
                << lzt::debuggerEventTypeString[module_event.type];
      lzt::debug_ack_event(debug_session, &module_event);
    }

    LOG_INFO << "[Debugger] Stopping all device threads";
    ze_device_thread_t device_threads = {};
    device_threads.slice = UINT32_MAX;
    device_threads.subslice = UINT32_MAX;
    device_threads.eu = UINT32_MAX;
    device_threads.thread = UINT32_MAX;

    std::this_thread::sleep_for(std::chrono::seconds(5));
    lzt::debug_interrupt(debug_session, device_threads);

    std::vector<ze_device_thread_t> stopped_threads;
    if (!find_stopped_threads(debug_session, device, stopped_threads)) {
      FAIL() << "Failed to stop device thread";
    }
    LOG_INFO << "[Debugger] Stopped device thread";

    auto register_set_properties = lzt::get_register_set_properties(device);
    for (auto &stopped_thread : stopped_threads) {
      for (auto &register_set : register_set_properties) {
        auto buffer_size = register_set.byteSize * register_set.count;
        void *buffer = lzt::allocate_host_memory(register_set.byteSize *
                                                 register_set.count);
        void *buffer_copy = lzt::allocate_host_memory(register_set.byteSize *
                                                      register_set.count);

        std::memset(buffer, 0xaa, buffer_size);

        auto can_verify_write = false;
        if (register_set.generalFlags & ZET_DEBUG_REGSET_FLAG_READABLE) {
          LOG_INFO << "[Debugger] Register set is readable";
          can_verify_write = true;
          // read all registers in this register set
          lzt::debug_read_registers(debug_session, stopped_thread,
                                    register_set.type, 0, register_set.count,
                                    register_set.byteSize, buffer);

          // save the contents for write test
          std::memcpy(buffer_copy, buffer, buffer_size);
        } else {
          LOG_INFO << "[Debugger] Register set not readable";
        }

        if (register_set.generalFlags & ZET_DEBUG_REGSET_FLAG_WRITEABLE) {
          LOG_INFO << "[Debugger] Register set is writeable";
          std::memset(buffer, 0xaa, buffer_size);

          lzt::debug_write_registers(debug_session, stopped_thread,
                                     register_set.type, 0, register_set.count,
                                     buffer);

          if (can_verify_write) {
            LOG_INFO << "[Debugger] Validating register written successfully";
            lzt::debug_read_registers(debug_session, stopped_thread,
                                      register_set.type, 0, register_set.count,
                                      register_set.byteSize, buffer);

            for (int i = 0; i < buffer_size; i++) {
              ASSERT_EQ(static_cast<char>(0xaa),
                        static_cast<char *>(buffer)[i]);
            }

            // write back the original contents
            lzt::debug_write_registers(debug_session, stopped_thread,
                                       register_set.type, 0, register_set.count,
                                       buffer_copy);
          }
        } else {
          LOG_INFO << "[Debugger] Register set not writeable";
        }

        lzt::free_memory(buffer);
        lzt::free_memory(buffer_copy);
      }
    }

    lzt::debug_resume(debug_session, device_threads);
  }
}

} // namespace
