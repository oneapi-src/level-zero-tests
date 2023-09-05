/*
 *
 * Copyright (C) 2019-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"
#include "test_ipc_event.hpp"
#include "net/test_ipc_comm.hpp"

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

ze_event_pool_desc_t defaultEventPoolDesc = {
    ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, nullptr,
    (ZE_EVENT_POOL_FLAG_HOST_VISIBLE | ZE_EVENT_POOL_FLAG_IPC), 10};

static void child_host_reads(ze_event_pool_handle_t hEventPool) {
  ze_event_handle_t hEvent;
  zeEventCreate(hEventPool, &defaultEventDesc, &hEvent);
  zeEventHostSynchronize(hEvent, UINT64_MAX);
  // cleanup
  zeEventDestroy(hEvent);
}

static void child_device_reads(ze_event_pool_handle_t hEventPool,
                               bool device_events, ze_context_handle_t context,
                               bool isImmediate) {
  ze_event_handle_t hEvent;
  if (device_events) {
    zeEventCreate(hEventPool, &defaultDeviceEventDesc, &hEvent);
  } else {
    zeEventCreate(hEventPool, &defaultEventDesc, &hEvent);
  }
  auto driver = lzt::get_default_driver();
  auto device = lzt::get_default_device(driver);
  auto cmdbundle = lzt::create_command_bundle(context, device, isImmediate);
  lzt::append_wait_on_events(cmdbundle.list, 1, &hEvent);
  lzt::close_command_list(cmdbundle.list);
  lzt::execute_and_sync_command_bundle(cmdbundle, UINT64_MAX);

  // cleanup
  lzt::destroy_command_bundle(cmdbundle);
  zeEventDestroy(hEvent);
}

static void child_device2_reads(ze_event_pool_handle_t hEventPool,
                                ze_context_handle_t context, bool isImmediate) {
  ze_event_handle_t hEvent;
  zeEventCreate(hEventPool, &defaultEventDesc, &hEvent);
  auto devices = lzt::get_ze_devices();
  auto cmdbundle = lzt::create_command_bundle(context, devices[1], isImmediate);
  lzt::append_wait_on_events(cmdbundle.list, 1, &hEvent);
  printf("execute second device\n");
  lzt::close_command_list(cmdbundle.list);
  lzt::execute_and_sync_command_bundle(cmdbundle, UINT64_MAX);

  // cleanup
  lzt::reset_command_list(cmdbundle.list);
  lzt::destroy_command_bundle(cmdbundle);
  zeEventDestroy(hEvent);
}

static void child_multi_device_reads(ze_event_pool_handle_t hEventPool,
                                     ze_context_handle_t context,
                                     bool isImmediate) {
  ze_event_handle_t hEvent;
  zeEventCreate(hEventPool, &defaultEventDesc, &hEvent);
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
  zeEventDestroy(hEvent);
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
  auto context = lzt::create_context(lzt::get_default_driver());
  shm.truncate(sizeof(shared_data_t));
  bipc::mapped_region region(shm, bipc::read_only);
  std::memcpy(&shared_data, region.get_address(), sizeof(shared_data_t));

  ze_ipc_event_pool_handle_t hIpcEventPool{};
  int ipc_descriptor =
      lzt::receive_ipc_handle<ze_ipc_event_pool_handle_t>(hIpcEventPool.data);
  memcpy(&(hIpcEventPool), static_cast<void *>(&ipc_descriptor),
         sizeof(ipc_descriptor));

  const bool isImmediate = shared_data.is_immediate;
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
  default:
    LOG_DEBUG << "Unrecognized test case";
    lzt::destroy_context(context);
    exit(1);
  }
  lzt::close_ipc_event_handle(hEventPool);
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
