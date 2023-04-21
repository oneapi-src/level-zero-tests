/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"
#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"
#include "test_ipc_event.hpp"
#include "net/test_ipc_comm.hpp"

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/process.hpp>

#include <level_zero/ze_api.h>

namespace lzt = level_zero_tests;
namespace bipc = boost::interprocess;
namespace {

static const ze_event_desc_t defaultEventDesc = {
    ZE_STRUCTURE_TYPE_EVENT_DESC, nullptr, 5, ZE_EVENT_SCOPE_FLAG_HOST,
    ZE_EVENT_SCOPE_FLAG_HOST // ensure memory coherency across device
                             // and Host after event signalled
};
static const ze_event_desc_t defaultDeviceEventDesc = {
    ZE_STRUCTURE_TYPE_EVENT_DESC, nullptr, 5, ZE_EVENT_SCOPE_FLAG_DEVICE,
    ZE_EVENT_SCOPE_FLAG_DEVICE // ensure memory coherency across device
                               // after event signalled
};

ze_event_pool_desc_t defaultEventPoolDesc = {
    ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, nullptr,
    (ZE_EVENT_POOL_FLAG_HOST_VISIBLE | ZE_EVENT_POOL_FLAG_IPC), 10};

static ze_event_pool_handle_t get_event_pool(bool multi_device,
                                             bool device_events,
                                             ze_context_handle_t context) {
  if (device_events) {
    defaultEventPoolDesc = {ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, nullptr,
                            ZE_EVENT_POOL_FLAG_IPC, 10};
    LOG_INFO << "Testing device only events";
  }
  std::vector<ze_device_handle_t> devices;
  if (multi_device) {
    devices = lzt::get_devices(lzt::get_default_driver());
  } else {
    ze_device_handle_t device =
        lzt::get_default_device(lzt::get_default_driver());
    devices.push_back(device);
  }
  return lzt::create_event_pool(context, defaultEventPoolDesc, devices);
}

static void parent_host_signals(ze_event_handle_t hEvent) {
  lzt::signal_event_from_host(hEvent);
}

static void parent_device_signals(ze_event_handle_t hEvent,
                                  ze_context_handle_t context) {
  auto driver = lzt::get_default_driver();
  auto device = lzt::get_default_device(driver);
  auto cmdlist = lzt::create_command_list(context, device);
  auto cmdqueue = lzt::create_command_queue(context, device);
  lzt::append_signal_event(cmdlist, hEvent);
  lzt::close_command_list(cmdlist);
  lzt::execute_command_lists(cmdqueue, 1, &cmdlist, nullptr);
  lzt::synchronize(cmdqueue, UINT64_MAX);

  // cleanup
  lzt::destroy_command_list(cmdlist);
  lzt::destroy_command_queue(cmdqueue);
}

static void run_ipc_event_test(parent_test_t parent_test,
                               child_test_t child_test, bool multi_device) {
#ifdef __linux__
  bipc::shared_memory_object::remove("ipc_event_test");
  // launch child
  boost::process::child c("./ipc/test_ipc_event_helper");

  ze_result_t result = zeInit(0);
  if (result) {
    throw std::runtime_error("zeInit failed: " +
                             level_zero_tests::to_string(result));
  }
  LOG_INFO << "IPC Parent zeinit";
  level_zero_tests::print_platform_overview();

  if (multi_device && lzt::get_ze_device_count() < 2) {
    LOG_INFO << "WARNING:  Exiting as multiple devices do not exist";
    GTEST_SKIP();
  }
  bool device_events = false;
  if (parent_test == PARENT_TEST_DEVICE_SIGNALS &&
      child_test == CHILD_TEST_DEVICE_READS) {
    device_events = true;
  }
  auto driver = lzt::get_default_driver();
  auto context = lzt::create_context(driver);

  auto ep = get_event_pool(multi_device, device_events, context);
  ze_ipc_event_pool_handle_t hIpcEventPool;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeEventPoolGetIpcHandle(ep, &hIpcEventPool));
  if (testing::Test::HasFatalFailure())
    return; // Abort test if IPC Event handle failed

  ze_event_handle_t hEvent;
  if (device_events) {
    hEvent = lzt::create_event(ep, defaultDeviceEventDesc);
  } else {
    hEvent = lzt::create_event(ep, defaultEventDesc);
  }
  shared_data_t test_data = {parent_test, child_test, multi_device,
                             hIpcEventPool};
  bipc::shared_memory_object shm(bipc::create_only, "ipc_event_test",
                                 bipc::read_write);
  shm.truncate(sizeof(shared_data_t));
  bipc::mapped_region region(shm, bipc::read_write);
  std::memcpy(region.get_address(), &test_data, sizeof(shared_data_t));

  lzt::send_ipc_handle(hIpcEventPool);

  switch (parent_test) {
  case PARENT_TEST_HOST_SIGNALS:
    parent_host_signals(hEvent);
    break;
  case PARENT_TEST_DEVICE_SIGNALS:
    parent_device_signals(hEvent, context);
    break;
  default:
    FAIL() << "Fatal test error";
  }

  c.wait(); // wait for the process to exit
  ASSERT_EQ(c.exit_code(), 0);

  // cleanup
  bipc::shared_memory_object::remove("ipc_event_test");
  lzt::destroy_event(hEvent);
  lzt::destroy_event_pool(ep);
  lzt::destroy_context(context);
#endif // linux
}

TEST(
    zeIPCEventTests,
    GivenTwoProcessesWhenEventSignaledByHostInParentThenEventSetinChildFromHostPerspective) {
  run_ipc_event_test(PARENT_TEST_HOST_SIGNALS, CHILD_TEST_HOST_READS, false);
}

TEST(
    zeIPCEventTests,
    GivenTwoProcessesWhenEventSignaledByDeviceInParentThenEventSetinChildFromHostPerspective) {
  run_ipc_event_test(PARENT_TEST_DEVICE_SIGNALS, CHILD_TEST_HOST_READS, false);
}

TEST(
    zeIPCEventTests,
    GivenTwoProcessesWhenEventSignaledByDeviceInParentThenEventSetinChildFromDevicePerspective) {
  run_ipc_event_test(PARENT_TEST_DEVICE_SIGNALS, CHILD_TEST_DEVICE_READS,
                     false);
}

TEST(
    zeIPCEventTests,
    GivenTwoProcessesWhenEventSignaledByHostInParentThenEventSetinChildFromDevicePerspective) {
  run_ipc_event_test(PARENT_TEST_HOST_SIGNALS, CHILD_TEST_DEVICE_READS, false);
}

TEST(
    zeIPCEventMultiDeviceTests,
    GivenTwoProcessesWhenEventSignaledByDeviceInParentThenEventSetinChildFromSecondDevicePerspective) {
  run_ipc_event_test(PARENT_TEST_DEVICE_SIGNALS, CHILD_TEST_DEVICE2_READS,
                     true);
}

TEST(
    zeIPCEventMultiDeviceTests,
    GivenTwoProcessesWhenEventSignaledByDeviceInParentThenEventSetinChildFromMultipleDevicePerspective) {
  run_ipc_event_test(PARENT_TEST_DEVICE_SIGNALS, CHILD_TEST_MULTI_DEVICE_READS,
                     true);
}

TEST(
    zeIPCEventMultiDeviceTests,
    GivenTwoProcessesWhenEventSignaledByDeviceInParentThenEventSetinChildFromDevicePerspectiveForSingleThenMultiDevice) {
  run_ipc_event_test(PARENT_TEST_DEVICE_SIGNALS, CHILD_TEST_DEVICE_READS,
                     false);
  run_ipc_event_test(PARENT_TEST_DEVICE_SIGNALS, CHILD_TEST_MULTI_DEVICE_READS,
                     true);
}

TEST(
    zeIPCEventMultiDeviceTests,
    GivenTwoProcessesWhenEventSignaledByHostInParentThenEventSetinChildFromMultipleDevicePerspective) {
  run_ipc_event_test(PARENT_TEST_HOST_SIGNALS, CHILD_TEST_MULTI_DEVICE_READS,
                     true);
}

} // namespace
