/*
 *
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <boost/filesystem.hpp>
#include <boost/process.hpp>
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
    auto device_properties = lzt::get_device_properties(device);
    auto properties = lzt::get_debug_properties(device);
    if (ZET_DEVICE_DEBUG_PROPERTY_FLAG_ATTACH & properties.flags == 0) {
      LOG_INFO << "Device " << device_properties.name
               << " does not support debug";
    } else {
      LOG_INFO << "Device " << device_properties.name << " supports debug";
    }
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
      if (ZET_DEVICE_DEBUG_PROPERTY_FLAG_ATTACH & properties.flags == 0) {
        LOG_INFO << "Device " << device_properties.name
                 << " does not support debug";
      } else {
        LOG_INFO << "Device " << device_properties.name << " supports debug";
      }
    }
  }
}

class zetDebugAttachDetachTest : public ::testing::Test {
protected:
  void SetUp() override {
    bi::shared_memory_object::remove("debug_bool");
    shm = new bi::shared_memory_object(bi::create_only, "debug_bool",
                                       bi::read_write);

    shm->truncate(sizeof(debug_signals_t));
    region = new bi::mapped_region(*shm, bi::read_write);
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

    // notify debugged process that this process has attached
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

  std::vector<ze_device_handle_t> all_sub_devices = {};
  for (auto &device : devices) {
    auto sub_devices = lzt::get_ze_sub_devices(device);
    sub_devices.insert(all_sub_devices.end(), sub_devices.begin(),
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
                         bool use_sub_devices, debug_test_type_t test_type);
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

    // notify debugged process that this process has attached
    mutex->lock();
    static_cast<debug_signals_t *>(region->get_address())->debugger_signal =
        true;
    mutex->unlock();
    condition->notify_all();

    // listen for events generated by child process operation
    std::vector<zet_debug_event_type_t> expected_event = {
        ZET_DEBUG_EVENT_TYPE_PROCESS_ENTRY, ZET_DEBUG_EVENT_TYPE_MODULE_LOAD,
        ZET_DEBUG_EVENT_TYPE_MODULE_UNLOAD, ZET_DEBUG_EVENT_TYPE_PROCESS_EXIT,
        ZET_DEBUG_EVENT_TYPE_DETACHED};
    auto event_num = 0;
    while (true) {
      auto debug_event = lzt::debug_read_event(
          debug_session, std::numeric_limits<uint64_t>::max());

      EXPECT_EQ(debug_event.type, expected_event[event_num++]);

      if (debug_event.flags & ZET_DEBUG_EVENT_FLAG_NEED_ACK) {
        lzt::debug_ack_event(debug_session, &debug_event);
      }

      if (ZET_DEBUG_EVENT_TYPE_DETACHED == debug_event.type) {
        EXPECT_EQ(debug_event.info.detached.reason,
                  ZET_DEBUG_DETACH_REASON_HOST_EXIT);
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
    sub_devices.insert(all_sub_devices.end(), sub_devices.begin(),
                       sub_devices.end());
  }

  run_test(all_sub_devices, true);
}

//=====================================================================================================
void zetDebugEventReadTest::run_advanced_test(
    std::vector<ze_device_handle_t> devices, bool use_sub_devices,
    debug_test_type_t test_type) {

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
        (use_sub_devices ? "--use_sub_devices" : ""), "--test_type" + test_type,
        bp::std_in < child_input);

    zet_debug_config_t debug_config = {};
    debug_config.pid = debug_helper.id();
    auto debug_session = lzt::debug_attach(device, debug_config);

    // notify debugged process that this process has attached
    mutex->lock();
    (static_cast<debug_signals_t *>(region->get_address()))->debugger_signal =
        true;
    mutex->unlock();
    condition->notify_all();

    std::vector<zet_debug_event_type_t> events = {};
    auto event_num = 0;
    auto timeout = std::numeric_limits<uint64_t>::max();
    while (true) {
      auto debug_event = lzt::debug_read_event(debug_session, timeout);

      events.push_back(debug_event.type);

      if (debug_event.flags & ZET_DEBUG_EVENT_FLAG_NEED_ACK) {
        lzt::debug_ack_event(debug_session, &debug_event);
      }

      if (test_type == KERNEL_RESUME &&
          ZET_DEBUG_EVENT_TYPE_INVALID == debug_event.type) {
        // kernel did not complete
        ADD_FAILURE() << "Kernel failed to complete after resuming";
      }

      if (test_type == KERNEL_RESUME &&
          ZET_DEBUG_EVENT_TYPE_THREAD_STOPPED == debug_event.type) {
        // resume kernel
        ze_device_thread_t all_device_threads = {};
        all_device_threads.slice = UINT32_MAX;
        all_device_threads.subslice = UINT32_MAX;
        all_device_threads.eu = UINT32_MAX;
        all_device_threads.thread = UINT32_MAX;

        lzt::debug_resume(debug_session, all_device_threads);

        timeout = 45000000000; // wait 45 seconds
      }

      if (ZET_DEBUG_EVENT_TYPE_DETACHED == debug_event.type) {
        EXPECT_EQ(debug_event.info.detached.reason,
                  ZET_DEBUG_DETACH_REASON_HOST_EXIT);
        break;
      }

      if (test_type == LONG_RUNNING_KERNEL_INTERRUPTED &&
          ZET_DEBUG_EVENT_TYPE_INVALID == debug_event.type) {
        // test unsuccessful, exit
        lzt::debug_detach(debug_session);
        debug_helper.terminate();
        ADD_FAILURE() << "Did not receive interrupt events after interrupting";
      }

      if (test_type == LONG_RUNNING_KERNEL_INTERRUPTED &&
          ZET_DEBUG_EVENT_TYPE_MODULE_LOAD == debug_event.type) {
        // interrupt all threads on the device
        ze_device_thread_t all_device_threads = {};
        all_device_threads.slice = UINT32_MAX;
        all_device_threads.subslice = UINT32_MAX;
        all_device_threads.eu = UINT32_MAX;
        all_device_threads.thread = UINT32_MAX;

        lzt::debug_interrupt(debug_session, all_device_threads);

        // expect that we will get an interrupt event next
        timeout = std::numeric_limits<uint32_t>::max();
      }

      if (test_type == LONG_RUNNING_KERNEL_INTERRUPTED &&
          ZET_DEBUG_EVENT_TYPE_THREAD_STOPPED == debug_event.type) {
        // test successful, exit
        lzt::debug_detach(debug_session);
        debug_helper.terminate();
        return;
      }

      if (ZET_DEBUG_EVENT_TYPE_INVALID == debug_event.type) {
        ADD_FAILURE() << "Received Invalid Event";
      }
    }

    switch (test_type) {
    case MULTIPLE_MODULES_CREATED:
      ASSERT_TRUE(std::count(events.begin(), events.end(),
                             ZET_DEBUG_EVENT_TYPE_MODULE_LOAD) == 2);
      break;
    case THREAD_STOPPED:
      ASSERT_TRUE(std::find(events.begin(), events.end(),
                            ZET_DEBUG_EVENT_TYPE_THREAD_STOPPED) !=
                  events.end());
      break;
    case THREAD_UNAVAILABLE:
      ASSERT_TRUE(std::find(events.begin(), events.end(),
                            ZET_DEBUG_EVENT_TYPE_THREAD_UNAVAILABLE) !=
                  events.end());
      break;
    case PAGE_FAULT:
      ASSERT_TRUE(std::find(events.begin(), events.end(),
                            ZET_DEBUG_EVENT_TYPE_PAGE_FAULT) != events.end());
      break;
    default:
      break;
    }

    lzt::debug_detach(debug_session);
    debug_helper.wait();
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
        "--test_type=" + ATTACH_AFTER_MODULE_CREATED, bp::std_in < child_input);

    zet_debug_config_t debug_config = {};
    debug_config.pid = debug_helper.id();

    bi::scoped_lock<bi::named_mutex> lock(*mutex);
    // wait until child says module created
    LOG_INFO << "Waiting for Child to create module";
    condition->wait(lock, [&] {
      return (static_cast<debug_signals_t *>(region->get_address())
                  ->debugee_signal);
    });
    LOG_INFO << "Debugged process proceeding";

    auto debug_session = lzt::debug_attach(device, debug_config);

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

      if (ZET_DEBUG_EVENT_TYPE_DETACHED == debug_event.type) {
        EXPECT_EQ(debug_event.info.detached.reason,
                  ZET_DEBUG_DETACH_REASON_HOST_EXIT);
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
       GivenDebugAttachedWhenInterruptLongRunningKernelThenStopEventReceived) {

  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);

  run_advanced_test(devices, false, LONG_RUNNING_KERNEL_INTERRUPTED);
}

TEST_F(zetDebugEventReadTest,
       GivenCalledInterruptAndResumeWhenKernelExecutingThenKernelCompletes) {

  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);

  run_advanced_test(devices, false, KERNEL_RESUME);
}

TEST_F(zetDebugEventReadTest,
       GivenDeviceExceptionOccursWhenAttachedThenThreadStoppedEventReceived) {

  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);

  run_advanced_test(devices, false, THREAD_STOPPED);
}

TEST_F(zetDebugEventReadTest,
       GivenThreadUnavailableWhenDebugEnabledThenThreadUnavailableEventRead) {

  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);

  run_advanced_test(devices, false, THREAD_UNAVAILABLE);
}

TEST_F(zetDebugEventReadTest,
       GivenPageFaultInHelperWhenDebugEnabledThenPageFaultEventRead) {

  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);

  run_advanced_test(devices, false, PAGE_FAULT);
}

} // namespace
