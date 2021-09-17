/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"
#include "test_ipc_event.hpp"
#include "test_ipc_comm.hpp"

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/process.hpp>

#include <level_zero/ze_api.h>

namespace lzt = level_zero_tests;
namespace bipc = boost::interprocess;

#ifdef __linux__
static const ze_event_desc_t defaultEventDesc = {
    ZE_STRUCTURE_TYPE_EVENT_DESC, nullptr, 5, 0,
    ZE_EVENT_SCOPE_FLAG_HOST // ensure memory coherency across device
                             // and Host after event signalled
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

static void child_device_reads(ze_event_pool_handle_t hEventPool) {
  ze_event_handle_t hEvent;
  zeEventCreate(hEventPool, &defaultEventDesc, &hEvent);

  auto cmdlist = lzt::create_command_list();
  auto cmdqueue = lzt::create_command_queue();
  lzt::append_wait_on_events(cmdlist, 1, &hEvent);
  lzt::close_command_list(cmdlist);
  lzt::execute_command_lists(cmdqueue, 1, &cmdlist, nullptr);
  lzt::synchronize(cmdqueue, UINT64_MAX);

  // cleanup
  lzt::destroy_command_list(cmdlist);
  lzt::destroy_command_queue(cmdqueue);
  zeEventDestroy(hEvent);
}

static void child_device2_reads(ze_event_pool_handle_t hEventPool) {
  ze_event_handle_t hEvent;
  zeEventCreate(hEventPool, &defaultEventDesc, &hEvent);
  auto devices = lzt::get_ze_devices();
  auto cmdlist = lzt::create_command_list(devices[1]);
  auto cmdqueue = lzt::create_command_queue(devices[1]);
  lzt::append_wait_on_events(cmdlist, 1, &hEvent);
  lzt::execute_command_lists(cmdqueue, 1, &cmdlist, nullptr);
  lzt::synchronize(cmdqueue, UINT64_MAX);

  // cleanup
  lzt::destroy_command_list(cmdlist);
  lzt::destroy_command_queue(cmdqueue);
  zeEventDestroy(hEvent);
}
static void child_multi_device_reads(ze_event_pool_handle_t hEventPool) {
  ze_event_handle_t hEvent;
  zeEventCreate(hEventPool, &defaultEventDesc, &hEvent);
  auto devices = lzt::get_ze_devices();
  auto cmdlist1 = lzt::create_command_list(devices[0]);
  auto cmdqueue1 = lzt::create_command_queue(devices[0]);
  lzt::append_wait_on_events(cmdlist1, 1, &hEvent);
  lzt::execute_command_lists(cmdqueue1, 1, &cmdlist1, nullptr);
  lzt::synchronize(cmdqueue1, UINT64_MAX);

  auto cmdlist2 = lzt::create_command_list(devices[1]);
  auto cmdqueue2 = lzt::create_command_queue(devices[1]);
  lzt::append_wait_on_events(cmdlist2, 1, &hEvent);
  lzt::execute_command_lists(cmdqueue2, 1, &cmdlist2, nullptr);
  lzt::synchronize(cmdqueue2, UINT64_MAX);
  // cleanup
  lzt::destroy_command_list(cmdlist1);
  lzt::destroy_command_queue(cmdqueue1);
  lzt::destroy_command_list(cmdlist2);
  lzt::destroy_command_queue(cmdqueue2);
  zeEventDestroy(hEvent);
}

int main() {

  ze_result_t result;
  if (zeInit(0) != ZE_RESULT_SUCCESS) {
    LOG_DEBUG << "Child exit due to zeInit failure";
    exit(1);
  }
  shared_data_t shared_data;
  bipc::shared_memory_object shm(bipc::open_only, "ipc_event_test",
                                 bipc::read_write);
  shm.truncate(sizeof(shared_data_t));
  bipc::mapped_region region(shm, bipc::read_only);
  std::memcpy(&shared_data, region.get_address(), sizeof(shared_data_t));

  int ipc_descriptor = lzt::receive_ipc_handle<ze_ipc_event_pool_handle_t>();
  ze_ipc_event_pool_handle_t hIpcEventPool = {};
  memcpy(&(hIpcEventPool), static_cast<void *>(&ipc_descriptor),
         sizeof(ipc_descriptor));

  ze_event_pool_handle_t hEventPool = 0;
  lzt::open_ipc_event_handle(lzt::get_default_context(), hIpcEventPool,
                             &hEventPool);

  if (!hEventPool) {
    LOG_DEBUG << "Child exit due to null event pool";
    exit(1);
  }
  switch (shared_data.child_type) {

  case CHILD_TEST_HOST_READS:
    child_host_reads(hEventPool);
    break;
  case CHILD_TEST_DEVICE_READS:
    child_device_reads(hEventPool);
    break;
  case CHILD_TEST_DEVICE2_READS:
    child_device2_reads(hEventPool);
    break;
  case CHILD_TEST_MULTI_DEVICE_READS:
    child_multi_device_reads(hEventPool);
    break;
  default:
    LOG_DEBUG << "Unrecognized test case";
    exit(1);
  }
  lzt::close_ipc_event_handle(hEventPool);
  exit(0);
}
#else // windows
int main() { exit(0); }
#endif
