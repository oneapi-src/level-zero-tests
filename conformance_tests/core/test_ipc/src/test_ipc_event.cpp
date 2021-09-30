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
    ZE_STRUCTURE_TYPE_EVENT_DESC, nullptr, 5, 0,
    ZE_EVENT_SCOPE_FLAG_HOST // ensure memory coherency across device
                             // and Host after event signalled
};

ze_event_pool_desc_t defaultEventPoolDesc = {
    ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, nullptr,
    (ZE_EVENT_POOL_FLAG_HOST_VISIBLE | ZE_EVENT_POOL_FLAG_IPC), 10};

static lzt::zeEventPool get_event_pool(bool multi_device) {
  lzt::zeEventPool ep;
  if (multi_device) {
    auto devices = lzt::get_devices(lzt::get_default_driver());
    ep.InitEventPool(defaultEventPoolDesc, devices);
  } else {
    ze_device_handle_t device =
        lzt::get_default_device(lzt::get_default_driver());
    ep.InitEventPool(defaultEventPoolDesc);
  }
  return ep;
}

static void parent_host_signals(ze_event_handle_t hEvent) {
  lzt::signal_event_from_host(hEvent);
}

static void parent_device_signals(ze_event_handle_t hEvent) {
  auto cmdlist = lzt::create_command_list();
  auto cmdqueue = lzt::create_command_queue();
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
  auto ep = get_event_pool(multi_device);
  ze_ipc_event_pool_handle_t hIpcEventPool;
  ep.get_ipc_handle(&hIpcEventPool);
  if (testing::Test::HasFatalFailure())
    return; // Abort test if IPC Event handle failed

  ze_event_handle_t hEvent;
  ep.create_event(hEvent, defaultEventDesc);
  shared_data_t test_data = {parent_test, child_test, multi_device};
  bipc::shared_memory_object::remove("ipc_event_test");
  bipc::shared_memory_object shm(bipc::create_only, "ipc_event_test",
                                 bipc::read_write);
  shm.truncate(sizeof(shared_data_t));
  bipc::mapped_region region(shm, bipc::read_write);
  std::memcpy(region.get_address(), &test_data, sizeof(shared_data_t));

  // launch child
  boost::process::child c("./ipc/test_ipc_event_helper");
  lzt::send_ipc_handle(hIpcEventPool);

  switch (parent_test) {
  case PARENT_TEST_HOST_SIGNALS:
    parent_host_signals(hEvent);
    break;
  case PARENT_TEST_DEVICE_SIGNALS:
    parent_device_signals(hEvent);
    break;
  default:
    FAIL() << "Fatal test error";
  }

  c.wait(); // wait for the process to exit
  ASSERT_EQ(c.exit_code(), 0);

  // cleanup
  bipc::shared_memory_object::remove("ipc_event_test");
  ep.destroy_event(hEvent);
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
  if (lzt::get_ze_device_count() < 2) {
    SUCCEED();
    LOG_INFO << "WARNING:  Exiting as multiple devices do not exist";
    return;
  }
  run_ipc_event_test(PARENT_TEST_DEVICE_SIGNALS, CHILD_TEST_DEVICE2_READS,
                     true);
}

TEST(
    zeIPCEventMultiDeviceTests,
    GivenTwoProcessesWhenEventSignaledByHostInParentThenEventSetinChildFromMultipleDevicePerspective) {
  if (lzt::get_ze_device_count() < 2) {
    SUCCEED();
    LOG_INFO << "WARNING:  Exiting as multiple devices do not exist";
    return;
  }
  run_ipc_event_test(PARENT_TEST_HOST_SIGNALS, CHILD_TEST_MULTI_DEVICE_READS,
                     true);
}

} // namespace
