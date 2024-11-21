/*
 *
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"
#include "test_ipc_memory.hpp"
#include "net/test_ipc_comm.hpp"

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/sync/named_semaphore.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/process.hpp>

#include <level_zero/ze_api.h>

namespace bipc = boost::interprocess;

#ifdef __linux__
static void child_device_access_test(int size, ze_ipc_memory_flags_t flags,
                                     bool is_immediate) {
  auto driver = lzt::get_default_driver();
  auto context = lzt::create_context(driver);
  auto device = lzt::zeDevice::get_instance()->get_device();
  auto cmd_bundle = lzt::create_command_bundle(context, device, is_immediate);
  ze_ipc_mem_handle_t ipc_handle;
  void *memory = nullptr;

  bipc::named_semaphore semaphore(bipc::open_only, "ipc_memory_test_semaphore");
  // Signal parent to send IPC handle
  semaphore.post();

  int ipc_descriptor =
      lzt::receive_ipc_handle<ze_ipc_mem_handle_t>(ipc_handle.data);
  memcpy(&ipc_handle, static_cast<void *>(&ipc_descriptor),
         sizeof(ipc_descriptor));

  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeMemOpenIpcHandle(context, device, ipc_handle, flags, &memory));

  void *buffer = lzt::allocate_host_memory(size, 1, context);
  memset(buffer, 0, size);
  lzt::append_memory_copy(cmd_bundle.list, buffer, memory, size);
  lzt::close_command_list(cmd_bundle.list);
  lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

  LOG_DEBUG << "[Child] Validating buffer received correctly";
  lzt::validate_data_pattern(buffer, size, 1);

  EXPECT_EQ(ZE_RESULT_SUCCESS, zeMemCloseIpcHandle(context, memory));
  lzt::free_memory(context, buffer);
  lzt::destroy_command_bundle(cmd_bundle);
  lzt::destroy_context(context);

  if (::testing::Test::HasFailure()) {
    exit(1);
  } else {
    exit(0);
  }
}

static void child_subdevice_access_test(int size, ze_ipc_memory_flags_t flags,
                                        bool is_immediate) {
  auto driver = lzt::get_default_driver();
  auto context = lzt::create_context(driver);
  auto device = lzt::zeDevice::get_instance()->get_device();
  auto sub_devices = lzt::get_ze_sub_devices(device);

  auto sub_device_count = sub_devices.size();

  ze_ipc_mem_handle_t ipc_handle;
  void *memory = nullptr;

  bipc::named_semaphore semaphore(bipc::open_only, "ipc_memory_test_semaphore");
  // Signal parent to send IPC handle
  semaphore.post();

  int ipc_descriptor =
      lzt::receive_ipc_handle<ze_ipc_mem_handle_t>(ipc_handle.data);
  memcpy(&ipc_handle, static_cast<void *>(&ipc_descriptor),
         sizeof(ipc_descriptor));

  // Open IPC buffer with root device
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeMemOpenIpcHandle(context, device, ipc_handle, flags, &memory));

  void *buffer = lzt::allocate_host_memory(size, 1, context);
  memset(buffer, 0, size);

  // For each sub device found, use IPC buffer in a copy operation and validate
  for (auto i = 0; i < sub_device_count; i++) {
    auto cmd_bundle =
        lzt::create_command_bundle(context, sub_devices[i], is_immediate);

    lzt::append_memory_copy(cmd_bundle.list, buffer, memory, size);
    lzt::close_command_list(cmd_bundle.list);
    lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

    LOG_DEBUG << "[Child] Validating buffer received correctly";
    lzt::validate_data_pattern(buffer, size, 1);
    lzt::destroy_command_bundle(cmd_bundle);
  }

  EXPECT_EQ(ZE_RESULT_SUCCESS, zeMemCloseIpcHandle(context, memory));
  lzt::free_memory(context, buffer);
  lzt::destroy_context(context);

  if (::testing::Test::HasFailure()) {
    exit(1);
  } else {
    exit(0);
  }
}

int main() {
  ze_result_t result = zeInit(0);
  if (result != ZE_RESULT_SUCCESS) {
    throw std::runtime_error("Child zeInit failed: " +
                             level_zero_tests::to_string(result));
  }
  LOG_INFO << "[Child] Driver initialized\n";

  shared_data_t shared_data;
  int count = 0;
  int retries = 1000;
  bipc::shared_memory_object shm;
  while (true) {
    try {
      shm = bipc::shared_memory_object(bipc::open_only, "ipc_memory_test",
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
  bipc::mapped_region region(shm, bipc::read_only);
  std::memcpy(&shared_data, region.get_address(), sizeof(shared_data_t));

  switch (shared_data.test_type) {
  case TEST_DEVICE_ACCESS:
    child_device_access_test(shared_data.size, shared_data.flags,
                             shared_data.is_immediate);
    break;
  case TEST_SUBDEVICE_ACCESS:
    child_subdevice_access_test(shared_data.size, shared_data.flags,
                                shared_data.is_immediate);
    break;
  default:
    LOG_DEBUG << "Unrecognized test case";
    exit(1);
  }

  if (testing::Test::HasFailure()) {
    LOG_DEBUG << "IPC Child Failed GTEST Check";
    exit(1);
  }
  exit(0);
}
#else // Windows
int main() { exit(0); }
#endif
