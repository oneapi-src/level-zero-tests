/*
 *
 * Copyright (C) 2019-2025 Intel Corporation
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
#include <boost/interprocess/sync/named_semaphore.hpp>

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

ze_event_pool_desc_t timestampEventPoolDesc = {
    ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, nullptr,
    (ZE_EVENT_POOL_FLAG_HOST_VISIBLE | ZE_EVENT_POOL_FLAG_IPC |
     ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP),
    10};

ze_event_pool_desc_t mappedTimestampEventPoolDesc = {
    ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, nullptr,
    (ZE_EVENT_POOL_FLAG_HOST_VISIBLE | ZE_EVENT_POOL_FLAG_IPC |
     ZE_EVENT_POOL_FLAG_KERNEL_MAPPED_TIMESTAMP),
    10};

typedef enum {
  NO_TIMESTAMP,
  KERNEL_TIMESTAMP,
  MAPPED_KERNEL_TIMESTAMP
} timestamp_type_t;

static ze_event_pool_handle_t get_event_pool(bool multi_device,
                                             bool device_events,
                                             ze_context_handle_t context,
                                             timestamp_type_t timestamp_type) {
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
  ze_event_pool_desc_t *eventPoolDesc = &defaultEventPoolDesc;
  switch (timestamp_type) {
  case KERNEL_TIMESTAMP:
    eventPoolDesc = &timestampEventPoolDesc;
    break;
  case MAPPED_KERNEL_TIMESTAMP:
    eventPoolDesc = &mappedTimestampEventPoolDesc;
    break;
  }
  return lzt::create_event_pool(context, *eventPoolDesc, devices);
}

static void parent_host_signals(ze_event_handle_t hEvent) {
  lzt::signal_event_from_host(hEvent);
}

static void parent_device_signals(ze_event_handle_t hEvent,
                                  ze_context_handle_t context,
                                  bool isImmediate) {
  auto driver = lzt::get_default_driver();
  auto device = lzt::get_default_device(driver);
  auto cmdbundle = lzt::create_command_bundle(context, device, isImmediate);
  lzt::append_signal_event(cmdbundle.list, hEvent);
  lzt::close_command_list(cmdbundle.list);
  lzt::execute_and_sync_command_bundle(cmdbundle, UINT64_MAX);

  // cleanup
  lzt::destroy_command_bundle(cmdbundle);
}

ze_kernel_handle_t get_matrix_multiplication_kernel(
    const ze_context_handle_t &context, ze_device_handle_t device,
    ze_group_count_t *tg, void **a_buffer, void **b_buffer, void **c_buffer,
    int dimensions = 1024) {
  int m, k, n;
  m = k = n = dimensions;
  std::vector<float> a(m * k, 1);
  std::vector<float> b(k * n, 1);
  std::vector<float> c(m * n, 0);
  *a_buffer = lzt::allocate_host_memory(m * k * sizeof(float));
  *b_buffer = lzt::allocate_host_memory(k * n * sizeof(float));
  *c_buffer = lzt::allocate_host_memory(m * n * sizeof(float));

  std::memcpy(*a_buffer, a.data(), a.size() * sizeof(float));
  std::memcpy(*b_buffer, b.data(), b.size() * sizeof(float));

  int group_count_x = m / 16;
  int group_count_y = n / 16;

  tg->groupCountX = group_count_x;
  tg->groupCountY = group_count_y;
  tg->groupCountZ = 1;

  ze_module_handle_t module =
      lzt::create_module(context, device, "ze_matrix_multiplication_ipc.spv",
                         ZE_MODULE_FORMAT_IL_SPIRV, nullptr, nullptr);
  ze_kernel_handle_t function =
      lzt::create_function(module, "matrix_multiplication");
  lzt::set_group_size(function, 16, 16, 1);
  lzt::set_argument_value(function, 0, sizeof(*a_buffer), a_buffer);
  lzt::set_argument_value(function, 1, sizeof(*b_buffer), b_buffer);
  lzt::set_argument_value(function, 2, sizeof(m), &m);
  lzt::set_argument_value(function, 3, sizeof(k), &k);
  lzt::set_argument_value(function, 4, sizeof(n), &n);
  lzt::set_argument_value(function, 5, sizeof(*c_buffer), c_buffer);
  return function;
}

static void run_workload(ze_event_handle_t &timestamp_event,
                         ze_context_handle_t &context, uint64_t &start,
                         uint64_t &end, bool mapped_timestamp) {

  auto driver = lzt::get_default_driver();
  auto device = lzt::get_default_device(driver);

  auto command_list = lzt::create_command_list(context, device, 0);
  auto command_queue = lzt::create_command_queue(
      context, device, 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);

  auto device_properties = lzt::get_device_properties(device);
  const auto max_threads =
      device_properties.numSlices * device_properties.numSubslicesPerSlice *
      device_properties.numEUsPerSubslice * device_properties.numThreadsPerEU;
  auto dimensions = max_threads > 4096 ? 1024 : 16;
  ze_group_count_t group_count;
  void *a_buffer, *b_buffer, *c_buffer;
  auto kernel =
      get_matrix_multiplication_kernel(context, device, &group_count, &a_buffer,
                                       &b_buffer, &c_buffer, dimensions);

  lzt::append_launch_function(command_list, kernel, &group_count,
                              timestamp_event, 0, nullptr);
  lzt::close_command_list(command_list);
  lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);

  lzt::synchronize(command_queue, UINT64_MAX);

  bipc::named_semaphore semaphore(bipc::open_only, "ipc_event_test_semaphore");
  semaphore.post();

  // get time data
  if (mapped_timestamp) {
    std::vector<ze_kernel_timestamp_result_t> kernel_timestamp_buffer{};
    std::vector<ze_synchronized_timestamp_result_ext_t>
        synchronized_timestamp_buffer{};
    lzt::get_event_kernel_timestamps_from_mapped_timestamp_event(
        timestamp_event, device, kernel_timestamp_buffer,
        synchronized_timestamp_buffer);

    ASSERT_GT(kernel_timestamp_buffer.size(), 0);
    start = synchronized_timestamp_buffer[0].global.kernelStart;
    end = synchronized_timestamp_buffer[0].global.kernelEnd;
  } else {
    auto tsResult = lzt::get_event_kernel_timestamp(timestamp_event);
    start = tsResult.global.kernelStart;
    end = tsResult.global.kernelEnd;
  }

  lzt::destroy_command_list(command_list);
  lzt::destroy_command_queue(command_queue);
}

static void run_ipc_event_test(parent_test_t parent_test,
                               child_test_t child_test, bool multi_device,
                               bool isImmediate) {
#ifdef __linux__
  bipc::named_semaphore::remove("ipc_event_test_semaphore");
  bipc::named_semaphore semaphore(bipc::create_only, "ipc_event_test_semaphore",
                                  0);

  bipc::shared_memory_object::remove("ipc_event_test");
  // launch child
  boost::process::child c("./ipc/test_ipc_event_helper");

  ze_result_t result = zeInit(0);
  if (result) {
    throw std::runtime_error("zeInit failed: " +
                             level_zero_tests::to_string(result));
  }
  LOG_INFO << "IPC Parent zeInit";
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

  auto timestamp_type = NO_TIMESTAMP;
  if (child_test == CHILD_TEST_HOST_TIMESTAMP_READS ||
      child_test == CHILD_TEST_DEVICE_TIMESTAMP_READS) {
    timestamp_type = KERNEL_TIMESTAMP;
  } else if (child_test == CHILD_TEST_HOST_MAPPED_TIMESTAMP_READS) {
    timestamp_type = MAPPED_KERNEL_TIMESTAMP;
  }
  auto ep =
      get_event_pool(multi_device, device_events, context, timestamp_type);
  ze_ipc_event_pool_handle_t hIpcEventPool;
  EXPECT_ZE_RESULT_SUCCESS(zeEventPoolGetIpcHandle(ep, &hIpcEventPool));
  if (testing::Test::HasFatalFailure())
    return; // Abort test if IPC Event handle failed

  ze_event_handle_t hEvent;
  if (device_events) {
    hEvent = lzt::create_event(ep, defaultDeviceEventDesc);
  } else {
    hEvent = lzt::create_event(ep, defaultEventDesc);
  }
  shared_data_t test_data = {parent_test, child_test, multi_device,
                             isImmediate};
  bipc::shared_memory_object shm(bipc::create_only, "ipc_event_test",
                                 bipc::read_write);
  shm.truncate(sizeof(shared_data_t));
  bipc::mapped_region region(shm, bipc::read_write);
  std::memcpy(region.get_address(), &test_data, sizeof(shared_data_t));

  lzt::send_ipc_handle(hIpcEventPool);
  uint64_t startTime = 0;
  uint64_t endTime = 0;
  switch (parent_test) {
  case PARENT_TEST_HOST_SIGNALS:
    parent_host_signals(hEvent);
    break;
  case PARENT_TEST_DEVICE_SIGNALS:
    parent_device_signals(hEvent, context, isImmediate);
    break;
  case PARENT_TEST_HOST_LAUNCHES_KERNEL:
    run_workload(hEvent, context, startTime, endTime,
                 (timestamp_type == MAPPED_KERNEL_TIMESTAMP));
    break;
  case PARENT_TEST_QUERY_EVENT_STATUS:
    break;
  default:
    FAIL() << "Fatal test error";
  }

  c.wait(); // wait for the process to exit
  ASSERT_EQ(c.exit_code(), 0);
  if (parent_test == PARENT_TEST_QUERY_EVENT_STATUS) {
    EXPECT_ZE_RESULT_SUCCESS(zeEventQueryStatus(hEvent));
  }
  if (parent_test == PARENT_TEST_HOST_LAUNCHES_KERNEL) {
    // ensure the timestamps match
    std::memcpy(&test_data, region.get_address(), sizeof(shared_data_t));
    EXPECT_EQ(test_data.start_time, startTime);
    EXPECT_EQ(test_data.end_time, endTime);
  }

  // cleanup
  bipc::shared_memory_object::remove("ipc_event_test");
  lzt::destroy_event(hEvent);
  lzt::destroy_event_pool(ep);
  lzt::destroy_context(context);
#endif // linux
}

LZT_TEST(
    zeIPCEventTests,
    GivenTwoProcessesWhenEventSignaledByHostInParentThenEventSetinChildFromHostPerspective) {
  run_ipc_event_test(PARENT_TEST_HOST_SIGNALS, CHILD_TEST_HOST_READS, false,
                     false);
}

LZT_TEST(
    zeIPCEventTests,
    GivenTwoProcessesWhenEventSignaledByDeviceInParentThenEventSetinChildFromHostPerspective) {
  run_ipc_event_test(PARENT_TEST_DEVICE_SIGNALS, CHILD_TEST_HOST_READS, false,
                     false);
}

LZT_TEST(
    zeIPCEventTests,
    GivenTwoProcessesWhenEventSignaledByDeviceInParentOnImmediateCmdListThenEventSetinChildFromHostPerspective) {
  run_ipc_event_test(PARENT_TEST_DEVICE_SIGNALS, CHILD_TEST_HOST_READS, false,
                     true);
}

LZT_TEST(
    zeIPCEventTests,
    GivenTwoProcessesWhenEventSignaledByDeviceInParentThenEventSetinChildFromDevicePerspective) {
  run_ipc_event_test(PARENT_TEST_DEVICE_SIGNALS, CHILD_TEST_DEVICE_READS, false,
                     false);
}

LZT_TEST(
    zeIPCEventTests,
    GivenTwoProcessesWhenEventSignaledByDeviceInParentOnImmediateCmdListThenEventSetinChildFromDevicePerspective) {
  run_ipc_event_test(PARENT_TEST_DEVICE_SIGNALS, CHILD_TEST_DEVICE_READS, false,
                     true);
}

LZT_TEST(
    zeIPCEventTests,
    GivenTwoProcessesWhenEventSignaledByHostInParentThenEventSetinChildFromDevicePerspective) {
  run_ipc_event_test(PARENT_TEST_HOST_SIGNALS, CHILD_TEST_DEVICE_READS, false,
                     false);
}

LZT_TEST(
    zeIPCEventTests,
    GivenTwoProcessesWhenEventSignaledByHostInParentOnImmediateCmdListThenEventSetinChildFromDevicePerspective) {
  run_ipc_event_test(PARENT_TEST_HOST_SIGNALS, CHILD_TEST_DEVICE_READS, false,
                     true);
}

LZT_TEST(
    zeIPCEventTests,
    GivenTwoProcessesWhenTimestampEventSignalledThenEventSetInChildFromHostPerspective) {
  run_ipc_event_test(PARENT_TEST_HOST_LAUNCHES_KERNEL,
                     CHILD_TEST_HOST_TIMESTAMP_READS, false, false);
}

LZT_TEST(
    zeIPCEventTests,
    GivenTwoProcessesWhenTimestampEventSignalledThenEventSetInChildFromDevicePerspective) {
  run_ipc_event_test(PARENT_TEST_HOST_LAUNCHES_KERNEL,
                     CHILD_TEST_DEVICE_TIMESTAMP_READS, false, false);
}

LZT_TEST(
    zeIPCEventTests,
    GivenTwoProcessesWhenMappedTimestampEventSignalledThenEventSetInChildFromHostPerspective) {
  run_ipc_event_test(PARENT_TEST_HOST_LAUNCHES_KERNEL,
                     CHILD_TEST_HOST_MAPPED_TIMESTAMP_READS, false, false);
}

LZT_TEST(zeIPCEventTests,
         GivenTwoProcessesWhenEventSignaledByChildThenEventQueryStatusSuccess) {
  run_ipc_event_test(PARENT_TEST_QUERY_EVENT_STATUS,
                     CHILD_TEST_QUERY_EVENT_STATUS, false, false);
}

LZT_TEST(
    zeIPCEventMultipleDeviceTests,
    GivenTwoProcessesWhenEventSignaledByDeviceInParentThenEventSetinChildFromSecondDevicePerspective) {
  run_ipc_event_test(PARENT_TEST_DEVICE_SIGNALS, CHILD_TEST_DEVICE2_READS, true,
                     false);
}

LZT_TEST(
    zeIPCEventMultipleDeviceTests,
    GivenTwoProcessesWhenEventSignaledByDeviceInParentOnImmediateCmdListThenEventSetinChildFromSecondDevicePerspective) {
  run_ipc_event_test(PARENT_TEST_DEVICE_SIGNALS, CHILD_TEST_DEVICE2_READS, true,
                     true);
}

LZT_TEST(
    zeIPCEventMultipleDeviceTests,
    GivenTwoProcessesWhenEventSignaledByDeviceInParentThenEventSetinChildFromMultipleDevicePerspective) {
  run_ipc_event_test(PARENT_TEST_DEVICE_SIGNALS, CHILD_TEST_MULTI_DEVICE_READS,
                     true, false);
}

LZT_TEST(
    zeIPCEventMultipleDeviceTests,
    GivenTwoProcessesWhenEventSignaledByDeviceInParentOnImmediateCmdListThenEventSetinChildFromMultipleDevicePerspective) {
  run_ipc_event_test(PARENT_TEST_DEVICE_SIGNALS, CHILD_TEST_MULTI_DEVICE_READS,
                     true, true);
}

LZT_TEST(
    zeIPCEventMultipleDeviceTests,
    GivenTwoProcessesWhenEventSignaledByDeviceInParentThenEventSetinChildFromDevicePerspectiveForSingleThenMultipleDevice) {
  run_ipc_event_test(PARENT_TEST_DEVICE_SIGNALS, CHILD_TEST_DEVICE_READS, false,
                     false);
  run_ipc_event_test(PARENT_TEST_DEVICE_SIGNALS, CHILD_TEST_MULTI_DEVICE_READS,
                     true, false);
}

LZT_TEST(
    zeIPCEventMultipleDeviceTests,
    GivenTwoProcessesWhenEventSignaledByDeviceInParentOnImmediateCmdListThenEventSetinChildFromDevicePerspectiveForSingleThenMultipleDevice) {
  run_ipc_event_test(PARENT_TEST_DEVICE_SIGNALS, CHILD_TEST_DEVICE_READS, false,
                     true);
  run_ipc_event_test(PARENT_TEST_DEVICE_SIGNALS, CHILD_TEST_MULTI_DEVICE_READS,
                     true, true);
}

LZT_TEST(
    zeIPCEventMultipleDeviceTests,
    GivenTwoProcessesWhenEventSignaledByHostInParentThenEventSetinChildFromMultipleDevicePerspective) {
  run_ipc_event_test(PARENT_TEST_HOST_SIGNALS, CHILD_TEST_MULTI_DEVICE_READS,
                     true, false);
}

LZT_TEST(
    zeIPCEventMultipleDeviceTests,
    GivenTwoProcessesWhenEventSignaledByHostInParentThenEventSetinChildOnImmediateCmdListFromMultipleDevicePerspective) {
  run_ipc_event_test(PARENT_TEST_HOST_SIGNALS, CHILD_TEST_MULTI_DEVICE_READS,
                     true, true);
}

} // namespace
