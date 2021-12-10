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

std::map<zet_debug_event_type_t, std::string> eventTypeString = {
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

TEST(
    zetDeviceGetDebugPropertiesTest,
    GivenValidDeviceWhenGettingDebugPropertiesThenPropertiesReturnedSuccessfully) {
  auto driver = lzt::get_default_driver();

  for (auto device : lzt::get_devices(driver)) {
    auto device_properties = lzt::get_device_properties(device);
    auto properties = lzt::get_debug_properties(device);

    ASSERT_TRUE((ZET_DEVICE_DEBUG_PROPERTY_FLAG_ATTACH & properties.flags))
        << "[Debugger] Device " << device_properties.name
        << " does not support debug";
  }
}

TEST(
    zetDeviceGetDebugPropertiesTest,
    GivenSubDeviceWhenGettingDebugPropertiesThenPropertiesReturnedSuccessfully) {
  auto driver = lzt::get_default_driver();

  for (auto &device : lzt::get_devices(driver)) {
    for (auto &sub_device : lzt::get_ze_sub_devices(device)) {
      auto device_properties = lzt::get_device_properties(sub_device);
      auto properties = lzt::get_debug_properties(sub_device);

      ASSERT_TRUE((ZET_DEVICE_DEBUG_PROPERTY_FLAG_ATTACH & properties.flags))
          << "[Debugger] Device " << device_properties.name
          << " does not support debug";
    }
  }
}

class zetDebugAttachDetachTest : public ::testing::Test {
protected:
  void SetUp() override {
    bi::shared_memory_object::remove("debug_bool");
    shm = new bi::shared_memory_object(bi::create_only, "debug_bool",
                                       bi::read_write);

    if (!shm) {
      FAIL()
          << "[Debugger] Could not create condition variable for debug tests";
    }

    shm->truncate(sizeof(debug_signals_t));
    region = new bi::mapped_region(*shm, bi::read_write);
    if (!region || !(region->get_address())) {
      FAIL() << "[Debugger] Could not create signal variables for debug tests";
    }
    static_cast<debug_signals_t *>(region->get_address())->debugger_signal =
        false;
    static_cast<debug_signals_t *>(region->get_address())->debugee_signal =
        false;

    bi::named_mutex::remove("debugger_mutex");
    mutex = new bi::named_mutex(bi::create_only, "debugger_mutex");

    bi::named_condition::remove("debug_bool_set");
    condition = new bi::named_condition(bi::create_only, "debug_bool_set");
  }

  void TearDown() override {
    bi::shared_memory_object::remove("debug_bool");
    bi::named_mutex::remove("debugger_mutex");
    bi::named_condition::remove("debug_bool_set");

    delete shm;
    delete region;
    delete mutex;
    delete condition;
  }

  void run_test(std::vector<ze_device_handle_t> devices, bool use_sub_devices);

  bi::shared_memory_object *shm;
  bi::mapped_region *region;
  bi::named_mutex *mutex;
  bi::named_condition *condition;
};

void zetDebugAttachDetachTest::run_test(std::vector<ze_device_handle_t> devices,
                                        bool use_sub_devices) {

  for (auto &device : devices) {
    auto device_properties = lzt::get_device_properties(device);
    auto debug_properties = lzt::get_debug_properties(device);
    ASSERT_TRUE(ZET_DEVICE_DEBUG_PROPERTY_FLAG_ATTACH & debug_properties.flags);

    fs::path helper_path(fs::current_path() / "debug");
    std::vector<fs::path> paths;
    paths.push_back(helper_path);
    fs::path helper = bp::search_path("test_debug_helper", paths);
    bp::opstream child_input;
    bp::child debug_helper(
        helper, "--device_id=" + lzt::to_string(device_properties.uuid),
        (use_sub_devices ? "--use_sub_devices" : ""), bp::std_in < child_input);

    zet_debug_config_t debug_config = {};
    debug_config.pid = debug_helper.id();
    auto debug_session = lzt::debug_attach(device, debug_config);
    if (!debug_session) {
      FAIL() << "[Debugger] Failed to attach to start a debug session";
    }

    LOG_INFO << "[Debugger] Notifying application after attaching";
    mutex->lock();
    static_cast<debug_signals_t *>(region->get_address())->debugger_signal =
        true;
    mutex->unlock();
    condition->notify_all();

    debug_helper.wait(); // we don't care about the child processes exit code at
                         // the moment
    lzt::debug_detach(debug_session);
  }
}

TEST_F(
    zetDebugAttachDetachTest,
    GivenDeviceSupportsDebugAttachWhenAttachingThenAttachAndDetachIsSuccessful) {

  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);
  run_test(devices, false);
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

  run_test(all_sub_devices, true);
}

class zetDebugEventReadTest : public zetDebugAttachDetachTest {
protected:
  void SetUp() override { zetDebugAttachDetachTest::SetUp(); }
  void TearDown() override { zetDebugAttachDetachTest::TearDown(); }
  void run_test(std::vector<ze_device_handle_t> devices, bool use_sub_devices);
  void run_advanced_test(std::vector<ze_device_handle_t> devices,
                         bool use_sub_devices, debug_test_type_t test_type,
                         num_threads_t threads = SINGLE_THREAD);
};

void zetDebugEventReadTest::run_test(std::vector<ze_device_handle_t> devices,
                                     bool use_sub_devices) {
  for (auto &device : devices) {
    auto device_properties = lzt::get_device_properties(device);
    auto debug_properties = lzt::get_debug_properties(device);
    ASSERT_TRUE(ZET_DEVICE_DEBUG_PROPERTY_FLAG_ATTACH & debug_properties.flags);

    fs::path helper_path(fs::current_path() / "debug");
    std::vector<fs::path> paths;
    paths.push_back(helper_path);
    fs::path helper = bp::search_path("test_debug_helper", paths);
    bp::opstream child_input;

    bp::child debug_helper(
        helper, "--device_id=" + lzt::to_string(device_properties.uuid),
        (use_sub_devices ? "--use_sub_devices" : ""), bp::std_in < child_input);

    zet_debug_config_t debug_config = {};
    debug_config.pid = debug_helper.id();
    auto debug_session = lzt::debug_attach(device, debug_config);
    if (!debug_session) {
      FAIL() << "[Debugger] Failed to attach to start a debug session";
    }

    LOG_INFO << "[Debugger] Notifying application after attaching";
    mutex->lock();
    static_cast<debug_signals_t *>(region->get_address())->debugger_signal =
        true;
    mutex->unlock();
    condition->notify_all();
    LOG_INFO << "[Debugger] Listening for events";

    // listen for events generated by child process operation
    std::vector<zet_debug_event_type_t> expected_event = {
        ZET_DEBUG_EVENT_TYPE_PROCESS_ENTRY, ZET_DEBUG_EVENT_TYPE_MODULE_LOAD,
        ZET_DEBUG_EVENT_TYPE_MODULE_UNLOAD, ZET_DEBUG_EVENT_TYPE_PROCESS_EXIT};
    auto event_num = 0;

    std::chrono::time_point<std::chrono::system_clock> start, checkpoint;
    start = std::chrono::system_clock::now();

    while (event_num < expected_event.size()) {
      auto debug_event = lzt::debug_read_event(
          debug_session, std::numeric_limits<uint64_t>::max());

      LOG_INFO << "[Debugger] received event: "
               << eventTypeString[debug_event.type];
      EXPECT_EQ(eventTypeString[debug_event.type],
                eventTypeString[expected_event[event_num++]]);

      if (debug_event.flags & ZET_DEBUG_EVENT_FLAG_NEED_ACK) {
        lzt::debug_ack_event(debug_session, &debug_event);
      }

      if (ZET_DEBUG_EVENT_TYPE_PROCESS_EXIT == debug_event.type) {
        break;
      }

      checkpoint = std::chrono::system_clock::now();
      std::chrono::duration<double> secondsLooping = checkpoint - start;
      if (secondsLooping.count() > 10) {
        LOG_ERROR << "[Debugger] Timed out waiting for events";
        break;
      }
    }
    lzt::debug_detach(debug_session);
    debug_helper.wait();
  }
}

TEST_F(
    zetDebugEventReadTest,
    GivenDebugCapableDeviceWhenProcessIsBeingDebuggedThenCorrectEventsAreRead) {

  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);
  run_test(devices, false);
}

TEST_F(
    zetDebugEventReadTest,
    GivenDebugCapableSubDeviceWhenProcessIsBeingDebuggedThenCorrectEventsAreRead) {

  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);

  std::vector<ze_device_handle_t> all_sub_devices = {};
  for (auto &device : devices) {
    auto sub_devices = lzt::get_ze_sub_devices(device);
    all_sub_devices.insert(all_sub_devices.end(), sub_devices.begin(),
                           sub_devices.end());
  }

  run_test(all_sub_devices, true);
}

//=====================================================================================================

int kernel_resume_test(zet_debug_session_handle_t &debug_session,
                       zet_debug_event_t &debug_event, num_threads_t threads,
                       uint64_t &timeout) {
  int result = 0;
  ze_device_thread_t device_threads = {};
  switch (threads) {
  case SINGLE_THREAD:
    device_threads.slice = 0;
    device_threads.subslice = 0;
    device_threads.eu = 0;
    device_threads.thread = 0;
    break;
  case GROUP_OF_THREADS:
    // TODO: Different groups, e.g. thread 0 on all slices
    device_threads.slice = 0;
    device_threads.subslice = 0;
    device_threads.eu = 0;
    device_threads.thread = UINT32_MAX;
    break;
  case ALL_THREADS:
    device_threads.slice = UINT32_MAX;
    device_threads.subslice = UINT32_MAX;
    device_threads.eu = UINT32_MAX;
    device_threads.thread = UINT32_MAX;
    break;
  default:
    break;
  }

  switch (debug_event.type) {
  case ZET_DEBUG_EVENT_TYPE_MODULE_LOAD: {
    // ack event again to ensure it is running
    lzt::debug_ack_event(debug_session, &debug_event);
    // interrupt threads on the device
    lzt::debug_interrupt(debug_session, device_threads);
    break;
  }
  case ZET_DEBUG_EVENT_TYPE_THREAD_STOPPED: {
    // resume kernel
    lzt::debug_resume(debug_session, device_threads);
    timeout = 45000000000; // wait 45 seconds for next event
    break;
  }
  case ZET_DEBUG_EVENT_TYPE_THREAD_UNAVAILABLE: {
    LOG_WARNING << "[Debugger] No threads were available for resume test";
    result = 1;
    break;
  }
  default:
    break;
  }

  return result;
}

int interrupt_test(zet_debug_session_handle_t &debug_session,
                   zet_debug_event_t &debug_event, num_threads_t threads,
                   uint64_t &timeout, bp::child &debug_helper) {
  int result = 0;
  switch (debug_event.type) {
  case ZET_DEBUG_EVENT_TYPE_MODULE_LOAD: {
    ze_device_thread_t device_threads = {};
    switch (threads) {
    case SINGLE_THREAD:
      device_threads.slice = 0;
      device_threads.subslice = 0;
      device_threads.eu = 0;
      device_threads.thread = 0;
      break;
    case GROUP_OF_THREADS:
      // TODO: Different groups, e.g. thread 0 on all slices
      device_threads.slice = 0;
      device_threads.subslice = 0;
      device_threads.eu = 0;
      device_threads.thread = UINT32_MAX;
      break;
    case ALL_THREADS:
      device_threads.slice = UINT32_MAX;
      device_threads.subslice = UINT32_MAX;
      device_threads.eu = UINT32_MAX;
      device_threads.thread = UINT32_MAX;
      break;
    default:
      break;
    }
    lzt::debug_interrupt(debug_session, device_threads);
    timeout = 45000000000; // expect next event immediately
    break;
  }
  case ZET_DEBUG_EVENT_TYPE_THREAD_STOPPED: {
    // test successful, exit
    lzt::debug_detach(debug_session);
    debug_helper.terminate();
    result = 1;
    break;
  }
  case ZET_DEBUG_EVENT_TYPE_THREAD_UNAVAILABLE: {
    ADD_FAILURE() << "[Debugger] No threads were available for interrupt test";
    lzt::debug_detach(debug_session);
    debug_helper.terminate();
    result = 1;
    break;
  }
  default:
    break;
  }
  return result;
}

void zetDebugEventReadTest::run_advanced_test(
    std::vector<ze_device_handle_t> devices, bool use_sub_devices,
    debug_test_type_t test_type, num_threads_t threads) {

  std::string test_options = "";

  for (auto &device : devices) {
    auto device_properties = lzt::get_device_properties(device);
    auto debug_properties = lzt::get_debug_properties(device);
    ASSERT_TRUE(ZET_DEVICE_DEBUG_PROPERTY_FLAG_ATTACH & debug_properties.flags);

    fs::path helper_path(fs::current_path() / "debug");
    std::vector<fs::path> paths;
    paths.push_back(helper_path);
    fs::path helper = bp::search_path("test_debug_helper", paths);
    bp::opstream child_input;
    bp::child debug_helper(
        helper, "--device_id=" + lzt::to_string(device_properties.uuid),
        (use_sub_devices ? "--use_sub_devices" : ""),
        "--test_type=" + std::to_string(test_type), bp::std_in < child_input);

    zet_debug_config_t debug_config = {};
    debug_config.pid = debug_helper.id();
    auto debug_session = lzt::debug_attach(device, debug_config);
    if (!debug_session) {
      FAIL() << "[Debugger] Failed to attach to start a debug session";
    }

    LOG_INFO << "[Debugger] Notifying application after attaching";
    mutex->lock();
    (static_cast<debug_signals_t *>(region->get_address()))->debugger_signal =
        true;
    mutex->unlock();
    condition->notify_all();

    std::vector<zet_debug_event_type_t> events = {};
    auto event_num = 0;
    uint64_t timeout = std::numeric_limits<uint64_t>::max();

    // debug event loop
    while (true) {
      auto debug_event = lzt::debug_read_event(debug_session, timeout);
      events.push_back(debug_event.type);

      if (debug_event.flags & ZET_DEBUG_EVENT_FLAG_NEED_ACK) {
        lzt::debug_ack_event(debug_session, &debug_event);
      }

      switch (test_type) {
      case KERNEL_RESUME: {
        if (::testing::Test::HasFailure()) {
          FAIL() << "[Debugger] Kernel failed to resume within expected time";
        } else {
          auto result =
              kernel_resume_test(debug_session, debug_event, threads, timeout);
          if (result) {
            FAIL();
          }

          break;
        }
      }
      case LONG_RUNNING_KERNEL_INTERRUPTED: {
        if (::testing::Test::HasFailure()) {
          lzt::debug_detach(debug_session);
          debug_helper.terminate();
          FAIL() << "[Debugger] Failed to receive either stop or unavailable "
                    "result after "
                    "interrupting threads";
        } else {
          auto result = interrupt_test(debug_session, debug_event, threads,
                                       timeout, debug_helper);
          if (result) {
            return;
          }
          break;
        }
      }
      case MULTIPLE_MODULES_CREATED:
        switch (debug_event.type) {
        case ZET_DEBUG_EVENT_TYPE_PROCESS_EXIT:
          ASSERT_EQ(2, std::count(events.begin(), events.end(),
                                  ZET_DEBUG_EVENT_TYPE_MODULE_LOAD));
          break;
        }
        break;
      case THREAD_UNAVAILABLE: {
        // attempt to stop a thread that is not running
        ze_device_thread_t device_thread = {};
        device_thread.slice = 0;
        device_thread.subslice = 0;
        device_thread.eu = 0;
        device_thread.thread = 0;
        lzt::debug_interrupt(debug_session, device_thread);
        switch (debug_event.type) {
        case ZET_DEBUG_EVENT_TYPE_PROCESS_EXIT:
          ASSERT_TRUE(std::find(events.begin(), events.end(),
                                ZET_DEBUG_EVENT_TYPE_THREAD_UNAVAILABLE) !=
                      events.end());
          break;
        }
        break;
      }
      }

      if (ZET_DEBUG_EVENT_TYPE_PROCESS_EXIT == debug_event.type) {
        break;
      }
    }

    // cleanup
    if (test_type != LONG_RUNNING_KERNEL_INTERRUPTED) {
      lzt::debug_detach(debug_session);
      debug_helper.wait();
    }
  }
}

TEST_F(zetDebugEventReadTest,
       GivenDeviceWhenThenAttachingAfterModuleCreatedThenEventReceived) {

  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);

  for (auto &device : devices) {
    auto device_properties = lzt::get_device_properties(device);
    auto debug_properties = lzt::get_debug_properties(device);
    ASSERT_TRUE(ZET_DEVICE_DEBUG_PROPERTY_FLAG_ATTACH & debug_properties.flags);

    fs::path helper_path(fs::current_path() / "debug");
    std::vector<fs::path> paths;
    paths.push_back(helper_path);
    fs::path helper = bp::search_path("test_debug_helper", paths);
    bp::opstream child_input;
    bp::child debug_helper(
        helper, "--device_id=" + lzt::to_string(device_properties.uuid),
        "--test_type=" + std::to_string(ATTACH_AFTER_MODULE_CREATED),
        bp::std_in < child_input);

    zet_debug_config_t debug_config = {};
    debug_config.pid = debug_helper.id();

    bi::scoped_lock<bi::named_mutex> lock(*mutex);
    // wait until child says module created
    LOG_INFO << "[Debugger] Waiting for Child to create module";
    condition->wait(lock, [&] {
      return (static_cast<debug_signals_t *>(region->get_address())
                  ->debugee_signal);
    });
    LOG_INFO << "[Debugger] Debugged process proceeding";

    auto debug_session = lzt::debug_attach(device, debug_config);
    if (!debug_session) {
      FAIL() << "[Debugger] Failed to attach to start a debug session";
    }

    // notify debugged process that this process has attached
    mutex->lock();
    static_cast<debug_signals_t *>(region->get_address())->debugger_signal =
        true;
    mutex->unlock();
    condition->notify_all();

    auto event_found = false;
    while (true) {
      auto debug_event = lzt::debug_read_event(
          debug_session, std::numeric_limits<uint64_t>::max());

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

TEST_F(zetDebugEventReadTest,
       GivenDeviceWhenCreatingMultipleModulesThenMultipleEventsReceived) {

  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);

  run_advanced_test(devices, false, MULTIPLE_MODULES_CREATED);
}

TEST_F(
    zetDebugEventReadTest,
    GivenDebugEnabledDeviceWhenAttachingAfterCreatingAndDestroyingModuleThenNoEventReceived) {

  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);

  run_advanced_test(devices, false, ATTACH_AFTER_MODULE_DESTROYED);
}

TEST_F(zetDebugEventReadTest,
       GivenDebugAttachedWhenInterruptKernelSingleThreadThenStopEventReceived) {

  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);

  run_advanced_test(devices, false, LONG_RUNNING_KERNEL_INTERRUPTED,
                    SINGLE_THREAD);
}

TEST_F(
    zetDebugEventReadTest,
    GivenDebugAttachedWhenInterruptKernelGroupOfThreadsThenStopEventReceived) {

  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);

  run_advanced_test(devices, false, LONG_RUNNING_KERNEL_INTERRUPTED,
                    GROUP_OF_THREADS);
}

TEST_F(zetDebugEventReadTest,
       GivenDebugAttachedWhenInterruptKernelAllThreadsThenStopEventReceived) {

  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);

  run_advanced_test(devices, false, LONG_RUNNING_KERNEL_INTERRUPTED,
                    ALL_THREADS);
}

TEST_F(
    zetDebugEventReadTest,
    GivenCalledInterruptAndResumeSingleThreadWhenKernelExecutingThenKernelCompletes) {

  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);

  run_advanced_test(devices, false, KERNEL_RESUME, SINGLE_THREAD);
}
TEST_F(
    zetDebugEventReadTest,
    GivenCalledInterruptAndResumeGroupOfThreadsWhenKernelExecutingThenKernelCompletes) {

  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);

  run_advanced_test(devices, false, KERNEL_RESUME, GROUP_OF_THREADS);
}
TEST_F(
    zetDebugEventReadTest,
    GivenCalledInterruptAndResumeAllThreadsWhenKernelExecutingThenKernelCompletes) {

  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);

  run_advanced_test(devices, false, KERNEL_RESUME, ALL_THREADS);
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
  void attachAndGetModuleEvent(uint32_t pid, ze_device_handle_t &device,
                               zet_debug_event_t &module_event);
  void readWriteModuleMemory(const zet_debug_session_handle_t &debug_session,
                             const ze_device_thread_t &thread,
                             zet_debug_event_t &module_event);

  zet_debug_session_handle_t debug_session;
  static constexpr uint8_t bufferSize = 16;
};

void zetDebugMemAccessTest::attachAndGetModuleEvent(
    uint32_t pid, ze_device_handle_t &device, zet_debug_event_t &module_event) {

  zet_debug_config_t debug_config = {};
  debug_config.pid = pid;
  debug_session = lzt::debug_attach(device, debug_config);

  LOG_INFO << "[Debugger] Notifying application after attaching";
  mutex->lock();
  static_cast<debug_signals_t *>(region->get_address())->debugger_signal = true;
  mutex->unlock();
  condition->notify_all();

  bool module_loaded = false;
  std::chrono::time_point<std::chrono::system_clock> start, checkpoint;
  start = std::chrono::system_clock::now();

  while (!module_loaded) {
    auto debug_event = lzt::debug_read_event(debug_session, 5000);

    if (ZET_DEBUG_EVENT_TYPE_MODULE_LOAD == debug_event.type) {
      module_loaded = true;
      module_event = debug_event;
    }
    checkpoint = std::chrono::system_clock::now();
    std::chrono::duration<double> secondsLooping = checkpoint - start;
    if (secondsLooping.count() > 5) {
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

TEST_F(zetDebugMemAccessTest,
       GivenDebuggerAttachedAndModuleLoadedAccessISAAndELFMemory) {

  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);
  for (auto &device : devices) {

    auto device_properties = lzt::get_device_properties(device);
    auto debug_properties = lzt::get_debug_properties(device);
    ASSERT_TRUE(ZET_DEVICE_DEBUG_PROPERTY_FLAG_ATTACH & debug_properties.flags);

    fs::path helper_path(fs::current_path() / "debug");
    std::vector<fs::path> paths;
    paths.push_back(helper_path);
    fs::path helper = bp::search_path("test_debug_helper", paths);
    bp::opstream child_input;
    bool use_sub_devices = false;
    bp::child debug_helper(
        helper, "--device_id=" + lzt::to_string(device_properties.uuid),
        (use_sub_devices ? "--use_sub_devices" : ""),
        "--test_type=" + std::to_string(BASIC), bp::std_in < child_input);
    zet_debug_event_t module_event;
    attachAndGetModuleEvent(debug_helper.id(), device, module_event);

    lzt::debug_clean_assert_true(module_event.info.module.load,
                                 debug_helper); // ISA
    lzt::debug_clean_assert_true(module_event.info.module.moduleBegin,
                                 debug_helper); // ELF start
    lzt::debug_clean_assert_true(module_event.info.module.moduleEnd,
                                 debug_helper); // ELF end

    // ALL threads
    ze_device_thread_t thread;
    thread.slice = UINT32_MAX;
    thread.subslice = UINT32_MAX;
    thread.eu = UINT32_MAX;
    thread.thread = UINT32_MAX;
    readWriteModuleMemory(debug_session, thread, module_event);

    // Single thread
    thread.slice = 0;
    thread.subslice = 0;
    thread.eu = 0;
    thread.thread = 0;
    readWriteModuleMemory(debug_session, thread, module_event);

    lzt::debug_detach(debug_session);
    debug_helper.terminate();
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

} // namespace
