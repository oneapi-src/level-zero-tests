/*
 *
 * Copyright (C) 2019-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

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

namespace bipc = boost::interprocess;

#ifdef __linux__
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

static void child_host_reads(ze_event_pool_handle_t hEventPool) {
  ze_event_handle_t hEvent;
  EXPECT_ZE_RESULT_SUCCESS(
      zeEventCreate(hEventPool, &defaultEventDesc, &hEvent));
  EXPECT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(hEvent, UINT64_MAX));
  // cleanup
  EXPECT_ZE_RESULT_SUCCESS(zeEventDestroy(hEvent));
}

static void child_query_event_status(ze_event_pool_handle_t hEventPool) {
  ze_event_handle_t hEvent = nullptr;
  EXPECT_ZE_RESULT_SUCCESS(
      zeEventCreate(hEventPool, &defaultEventDesc, &hEvent));
  EXPECT_EQ(ZE_RESULT_NOT_READY, zeEventQueryStatus(hEvent));
  EXPECT_ZE_RESULT_SUCCESS(zeEventHostSignal(hEvent));
  EXPECT_ZE_RESULT_SUCCESS(zeEventQueryStatus(hEvent));
  EXPECT_ZE_RESULT_SUCCESS(zeEventDestroy(hEvent));
}

static void child_device_reads(ze_event_pool_handle_t hEventPool,
                               bool device_events, ze_context_handle_t context,
                               bool isImmediate) {
  ze_event_handle_t hEvent;
  if (device_events) {
    EXPECT_ZE_RESULT_SUCCESS(
        zeEventCreate(hEventPool, &defaultDeviceEventDesc, &hEvent));
  } else {
    EXPECT_ZE_RESULT_SUCCESS(
        zeEventCreate(hEventPool, &defaultEventDesc, &hEvent));
  }
  auto driver = lzt::get_default_driver();
  auto device = lzt::get_default_device(driver);
  auto cmdbundle = lzt::create_command_bundle(context, device, isImmediate);
  lzt::append_wait_on_events(cmdbundle.list, 1, &hEvent);
  lzt::close_command_list(cmdbundle.list);
  lzt::execute_and_sync_command_bundle(cmdbundle, UINT64_MAX);

  // cleanup
  lzt::destroy_command_bundle(cmdbundle);
  EXPECT_ZE_RESULT_SUCCESS(zeEventDestroy(hEvent));
}

static void child_device2_reads(ze_event_pool_handle_t hEventPool,
                                ze_context_handle_t context, bool isImmediate) {
  ze_event_handle_t hEvent;
  EXPECT_ZE_RESULT_SUCCESS(
      zeEventCreate(hEventPool, &defaultEventDesc, &hEvent));
  auto devices = lzt::get_ze_devices();
  auto cmdbundle = lzt::create_command_bundle(context, devices[1], isImmediate);
  lzt::append_wait_on_events(cmdbundle.list, 1, &hEvent);
  printf("execute second device\n");
  lzt::close_command_list(cmdbundle.list);
  lzt::execute_and_sync_command_bundle(cmdbundle, UINT64_MAX);

  // cleanup
  lzt::reset_command_list(cmdbundle.list);
  lzt::destroy_command_bundle(cmdbundle);
  EXPECT_ZE_RESULT_SUCCESS(zeEventDestroy(hEvent));
}

static void child_multi_device_reads(ze_event_pool_handle_t hEventPool,
                                     ze_context_handle_t context,
                                     bool isImmediate) {
  ze_event_handle_t hEvent;
  EXPECT_ZE_RESULT_SUCCESS(
      zeEventCreate(hEventPool, &defaultEventDesc, &hEvent));
  auto devices = lzt::get_ze_devices();
  auto cmdbundle1 =
      lzt::create_command_bundle(context, devices[0], isImmediate);
  lzt::append_wait_on_events(cmdbundle1.list, 1, &hEvent);
  printf("execute device[0]\n");
  lzt::close_command_list(cmdbundle1.list);
  lzt::execute_and_sync_command_bundle(cmdbundle1, UINT64_MAX);

  auto cmdbundle2 =
      lzt::create_command_bundle(context, devices[1], isImmediate);
  lzt::append_wait_on_events(cmdbundle2.list, 1, &hEvent);
  printf("execute device[1]\n");
  lzt::close_command_list(cmdbundle2.list);
  lzt::execute_and_sync_command_bundle(cmdbundle2, UINT64_MAX);
  // cleanup
  lzt::reset_command_list(cmdbundle1.list);
  lzt::reset_command_list(cmdbundle2.list);
  lzt::destroy_command_bundle(cmdbundle1);
  lzt::destroy_command_bundle(cmdbundle2);
  EXPECT_ZE_RESULT_SUCCESS(zeEventDestroy(hEvent));
}

static void child_host_query_timestamp(const ze_event_pool_handle_t &hEventPool,
                                       shared_data_t &shared_data,
                                       bool mapped_timestamp) {

  auto driver = lzt::get_default_driver();
  auto device = lzt::get_default_device(driver);

  ze_event_handle_t hEvent;
  EXPECT_ZE_RESULT_SUCCESS(
      zeEventCreate(hEventPool, &defaultEventDesc, &hEvent));
  EXPECT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(hEvent, UINT64_MAX));

  ze_kernel_timestamp_result_t tsResult;
  if (mapped_timestamp) {
    std::vector<ze_kernel_timestamp_result_t> kernel_timestamp_buffer{};
    std::vector<ze_synchronized_timestamp_result_ext_t>
        synchronized_timestamp_buffer{};
    lzt::get_event_kernel_timestamps_from_mapped_timestamp_event(
        hEvent, device, kernel_timestamp_buffer, synchronized_timestamp_buffer);

    for (size_t i = 0U; i < kernel_timestamp_buffer.size(); i++) {
      if (!i) {
        LOG_INFO << "IPC Child set timestamp value";
        shared_data.start_time =
            synchronized_timestamp_buffer[i].global.kernelStart;
        shared_data.end_time =
            synchronized_timestamp_buffer[i].global.kernelEnd;
      }

      EXPECT_GT(kernel_timestamp_buffer[i].global.kernelStart, 0);
      EXPECT_GT(kernel_timestamp_buffer[i].global.kernelEnd, 0);
      EXPECT_GT(kernel_timestamp_buffer[i].context.kernelStart, 0);
      EXPECT_GT(kernel_timestamp_buffer[i].context.kernelEnd, 0);
      EXPECT_GT(kernel_timestamp_buffer[i].context.kernelEnd,
                kernel_timestamp_buffer[i].context.kernelStart);
      EXPECT_GT(kernel_timestamp_buffer[i].global.kernelEnd,
                kernel_timestamp_buffer[i].global.kernelStart);

      EXPECT_GT(synchronized_timestamp_buffer[i].global.kernelStart, 0);
      EXPECT_GT(synchronized_timestamp_buffer[i].global.kernelEnd, 0);
      EXPECT_GT(synchronized_timestamp_buffer[i].context.kernelStart, 0);
      EXPECT_GT(synchronized_timestamp_buffer[i].context.kernelEnd, 0);
      EXPECT_GT(synchronized_timestamp_buffer[i].context.kernelEnd,
                synchronized_timestamp_buffer[i].context.kernelStart);
      EXPECT_GT(synchronized_timestamp_buffer[i].global.kernelEnd,
                synchronized_timestamp_buffer[i].global.kernelStart);
    }
  } else {
    tsResult = lzt::get_event_kernel_timestamp(hEvent);

    LOG_INFO << "IPC Child set timestamp value";
    shared_data.start_time = tsResult.global.kernelStart;
    shared_data.end_time = tsResult.global.kernelEnd;

    EXPECT_GT(tsResult.global.kernelStart, 0);
    EXPECT_GT(tsResult.global.kernelEnd, 0);
    EXPECT_GT(tsResult.context.kernelStart, 0);
    EXPECT_GT(tsResult.context.kernelEnd, 0);
    EXPECT_GT(tsResult.context.kernelEnd, tsResult.context.kernelStart);
    EXPECT_GT(tsResult.global.kernelEnd, tsResult.global.kernelStart);
  }

  // cleanup
  EXPECT_ZE_RESULT_SUCCESS(zeEventDestroy(hEvent));
}

static void
child_device_query_timestamp(const ze_event_pool_handle_t &hEventPool,
                             bool device_events,
                             const ze_context_handle_t &context,
                             shared_data_t &shared_data, bool isImmediate) {

  ze_event_handle_t hEvent;
  if (device_events) {
    EXPECT_ZE_RESULT_SUCCESS(
        zeEventCreate(hEventPool, &defaultDeviceEventDesc, &hEvent));
  } else {
    EXPECT_ZE_RESULT_SUCCESS(
        zeEventCreate(hEventPool, &defaultEventDesc, &hEvent));
  }
  auto driver = lzt::get_default_driver();
  auto device = lzt::get_devices(driver)[0];
  auto cmdbundle = lzt::create_command_bundle(context, device, isImmediate);

  ze_kernel_timestamp_result_t *tsResult;
  tsResult =
      static_cast<ze_kernel_timestamp_result_t *>(lzt::allocate_host_memory(
          sizeof(ze_kernel_timestamp_result_t), 1, context));

  EXPECT_ZE_RESULT_SUCCESS(zeCommandListAppendQueryKernelTimestamps(
      cmdbundle.list, 1, &hEvent, tsResult, nullptr, nullptr, 1, &hEvent));

  lzt::close_command_list(cmdbundle.list);
  lzt::execute_and_sync_command_bundle(cmdbundle, UINT64_MAX);

  shared_data.start_time = tsResult->global.kernelStart;
  shared_data.end_time = tsResult->global.kernelEnd;

  EXPECT_GT(tsResult->global.kernelStart, 0);
  EXPECT_GT(tsResult->global.kernelEnd, 0);
  EXPECT_GT(tsResult->context.kernelStart, 0);
  EXPECT_GT(tsResult->context.kernelEnd, 0);
  EXPECT_GT(tsResult->context.kernelEnd, tsResult->context.kernelStart);
  EXPECT_GT(tsResult->global.kernelEnd, tsResult->global.kernelStart);

  // cleanup
  lzt::free_memory(context, tsResult);
  lzt::destroy_command_bundle(cmdbundle);
  EXPECT_ZE_RESULT_SUCCESS(zeEventDestroy(hEvent));
}

int main() {
  ze_result_t result = zeInit(0);
  if (result != ZE_RESULT_SUCCESS) {
    LOG_DEBUG << "Child exit due to zeInit failure";
    exit(1);
  }

  LOG_INFO << "IPC Child zeInit";

  shared_data_t shared_data;
  int count = 0;
  int retries = 1000;
  bipc::shared_memory_object shm;
  while (true) {
    try {
      shm = bipc::shared_memory_object(bipc::open_only, "ipc_event_test",
                                       bipc::read_write);
      break;
    } catch (const bipc::interprocess_exception &ex) {
      sched_yield();
      sleep(1);
      if (++count == retries)
        throw ex;
    }
  }
  shm.truncate(sizeof(shared_data_t));
  bipc::mapped_region region(shm, bipc::read_write);
  std::memcpy(&shared_data, region.get_address(), sizeof(shared_data_t));

  ze_ipc_event_pool_handle_t hIpcEventPool{};
  int ipc_descriptor =
      lzt::receive_ipc_handle<ze_ipc_event_pool_handle_t>(hIpcEventPool.data);
  memcpy(&(hIpcEventPool), static_cast<void *>(&ipc_descriptor),
         sizeof(ipc_descriptor));

  std::vector<ze_device_handle_t> devices;
  if (shared_data.multi_device) {
    devices = lzt::get_devices(lzt::get_default_driver());
  } else {
    ze_device_handle_t device =
        lzt::get_default_device(lzt::get_default_driver());
    devices.push_back(device);
  }
  auto context =
      lzt::create_context_ex(lzt::get_default_driver(), std::move(devices));
  ze_event_pool_handle_t hEventPool = 0;
  LOG_INFO << "IPC Child open event handle";
  lzt::open_ipc_event_handle(context, hIpcEventPool, &hEventPool);

  if (!hEventPool) {
    LOG_DEBUG << "Child exit due to null event pool";
    lzt::destroy_context(context);
    exit(1);
  }
  bool device_events = false;
  if (shared_data.parent_type == PARENT_TEST_DEVICE_SIGNALS &&
      shared_data.child_type == CHILD_TEST_DEVICE_READS) {
    device_events = true;
  }

  if (shared_data.parent_type == PARENT_TEST_HOST_LAUNCHES_KERNEL) {
    // Wait until the child is ready to query the time stamp
    bipc::named_semaphore semaphore(bipc::open_only,
                                    "ipc_event_test_semaphore");
    semaphore.wait();
  }

  const bool isImmediate = shared_data.is_immediate;
  LOG_INFO << "IPC Child open event handle success";
  switch (shared_data.child_type) {

  case CHILD_TEST_HOST_READS:
    child_host_reads(hEventPool);
    break;
  case CHILD_TEST_DEVICE_READS:
    child_device_reads(hEventPool, device_events, context, isImmediate);
    break;
  case CHILD_TEST_DEVICE2_READS:
    child_device2_reads(hEventPool, context, isImmediate);
    break;
  case CHILD_TEST_MULTI_DEVICE_READS:
    child_multi_device_reads(hEventPool, context, isImmediate);
    break;
  case CHILD_TEST_HOST_TIMESTAMP_READS:
    child_host_query_timestamp(hEventPool, shared_data, false);
    std::memcpy(region.get_address(), &shared_data, sizeof(shared_data_t));
    break;
  case CHILD_TEST_DEVICE_TIMESTAMP_READS:
    child_device_query_timestamp(hEventPool, device_events, context,
                                 shared_data, isImmediate);
    std::memcpy(region.get_address(), &shared_data, sizeof(shared_data_t));
    break;
  case CHILD_TEST_HOST_MAPPED_TIMESTAMP_READS:
    child_host_query_timestamp(hEventPool, shared_data, true);
    std::memcpy(region.get_address(), &shared_data, sizeof(shared_data_t));
    break;
  case CHILD_TEST_QUERY_EVENT_STATUS:
    child_query_event_status(hEventPool);
    break;
  default:
    LOG_DEBUG << "Unrecognized test case";
    lzt::destroy_context(context);
    exit(1);
  }
  try {
    lzt::close_ipc_event_handle(hEventPool);
  } catch (const std::runtime_error &ex) {
    LOG_ERROR << "Error closing IPC event handle: " << ex.what();
  }
  lzt::destroy_context(context);
  if (testing::Test::HasFailure()) {
    LOG_DEBUG << "IPC Child Failed GTEST Check";
    exit(1);
  }
  exit(0);
}
#else // Windows
int main() { exit(0); }
#endif
