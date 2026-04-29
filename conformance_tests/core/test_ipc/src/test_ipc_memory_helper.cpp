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
static void child_device_access_test(size_t size, ze_ipc_memory_flags_t flags,
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

  EXPECT_ZE_RESULT_SUCCESS(
      zeMemOpenIpcHandle(context, device, ipc_handle, flags, &memory));

  void *buffer = lzt::allocate_host_memory(size, 1, context);
  memset(buffer, 0, size);
  lzt::append_memory_copy(cmd_bundle.list, buffer, memory, size);
  lzt::close_command_list(cmd_bundle.list);
  lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

  LOG_DEBUG << "[Child] Validating buffer received correctly";
  lzt::validate_data_pattern(buffer, size, 1);

  EXPECT_ZE_RESULT_SUCCESS(zeMemCloseIpcHandle(context, memory));
  lzt::free_memory(context, buffer);
  lzt::destroy_command_bundle(cmd_bundle);
  lzt::destroy_context(context);

  if (::testing::Test::HasFailure()) {
    exit(1);
  } else {
    exit(0);
  }
}

static void child_subdevice_access_test(size_t size,
                                        ze_ipc_memory_flags_t flags,
                                        bool is_immediate) {
  auto driver = lzt::get_default_driver();
  auto context = lzt::create_context(driver);
  auto device = lzt::zeDevice::get_instance()->get_device();
  auto sub_devices = lzt::get_ze_sub_devices(device);

  size_t sub_device_count = sub_devices.size();

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
  EXPECT_ZE_RESULT_SUCCESS(
      zeMemOpenIpcHandle(context, device, ipc_handle, flags, &memory));

  void *buffer = lzt::allocate_host_memory(size, 1, context);
  memset(buffer, 0, size);

  // For each sub device found, use IPC buffer in a copy operation and validate
  for (size_t i = 0U; i < sub_device_count; i++) {
    auto cmd_bundle =
        lzt::create_command_bundle(context, sub_devices[i], is_immediate);

    lzt::append_memory_copy(cmd_bundle.list, buffer, memory, size);
    lzt::close_command_list(cmd_bundle.list);
    lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

    LOG_DEBUG << "[Child] Validating buffer received correctly";
    lzt::validate_data_pattern(buffer, size, 1);
    lzt::destroy_command_bundle(cmd_bundle);
  }

  EXPECT_ZE_RESULT_SUCCESS(zeMemCloseIpcHandle(context, memory));
  lzt::free_memory(context, buffer);
  lzt::destroy_context(context);

  if (::testing::Test::HasFailure()) {
    exit(1);
  } else {
    exit(0);
  }
}
#endif

static void child_device_access_test_opaque(size_t size,
                                            ze_ipc_memory_flags_t flags,
                                            bool is_immediate,
                                            ze_ipc_mem_handle_t ipc_handle,
                                            bool use_copy_engine) {
  auto driver = lzt::get_default_driver();
  auto context = lzt::create_context(driver);
  auto device = lzt::zeDevice::get_instance()->get_device();

  uint32_t ordinal = 0;
  if (use_copy_engine) {
    auto cmd_queue_group_props =
        lzt::get_command_queue_group_properties(device);
    auto copy_ordinal = lzt::get_queue_ordinal(
        cmd_queue_group_props, ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COPY,
        ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE);
    if (!copy_ordinal.has_value()) {
      LOG_WARNING << "[Child] No copy-only engine available. Skipping.";
      lzt::destroy_context(context);
      exit(0);
    }
    ordinal = *copy_ordinal;
  }

  auto cmd_bundle =
      use_copy_engine
          ? lzt::create_command_bundle(context, device, 0, ordinal,
                                       is_immediate)
          : lzt::create_command_bundle(context, device, is_immediate);
  void *memory = nullptr;

  EXPECT_ZE_RESULT_SUCCESS(
      zeMemOpenIpcHandle(context, device, ipc_handle, flags, &memory));

  void *buffer = lzt::allocate_host_memory(size, 1, context);
  memset(buffer, 0, size);
  lzt::append_memory_copy(cmd_bundle.list, buffer, memory, size);
  lzt::close_command_list(cmd_bundle.list);
  lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

  LOG_DEBUG << "[Child] Validating buffer received correctly";
  lzt::validate_data_pattern(buffer, size, 1);

  EXPECT_ZE_RESULT_SUCCESS(zeMemCloseIpcHandle(context, memory));
  lzt::free_memory(context, buffer);
  lzt::destroy_command_bundle(cmd_bundle);
  lzt::destroy_context(context);

  if (::testing::Test::HasFailure()) {
    exit(1);
  } else {
    exit(0);
  }
}

static void child_subdevice_access_test_opaque(size_t size,
                                               ze_ipc_memory_flags_t flags,
                                               bool is_immediate,
                                               ze_ipc_mem_handle_t ipc_handle,
                                               bool use_copy_engine) {
  auto driver = lzt::get_default_driver();
  auto context = lzt::create_context(driver);
  auto device = lzt::zeDevice::get_instance()->get_device();
  auto sub_devices = lzt::get_ze_sub_devices(device);

  size_t sub_device_count = sub_devices.size();

  auto cmd_bundle = lzt::create_command_bundle(context, device, is_immediate);
  void *memory = nullptr;

  EXPECT_ZE_RESULT_SUCCESS(
      zeMemOpenIpcHandle(context, device, ipc_handle, flags, &memory));

  void *buffer = lzt::allocate_host_memory(size, 1, context);
  memset(buffer, 0, size);
  // For each sub device found, use IPC buffer in a copy operation and validate
  for (size_t i = 0U; i < sub_device_count; i++) {
    uint32_t ordinal = 0;
    ze_device_handle_t sub_device = sub_devices[i];
    if (use_copy_engine) {
      auto cmd_queue_group_props =
          lzt::get_command_queue_group_properties(sub_device);
      auto copy_ordinal = lzt::get_queue_ordinal(
          cmd_queue_group_props, ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COPY,
          ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE);
      if (!copy_ordinal.has_value()) {
        LOG_WARNING << "[Child] No copy-only engine on sub-device " << i
                    << ". Skipping sub-device.";
        continue;
      }
      ordinal = *copy_ordinal;
    }

    auto sub_bundle =
        use_copy_engine
            ? lzt::create_command_bundle(context, sub_device, 0, ordinal,
                                         is_immediate)
            : lzt::create_command_bundle(context, sub_device, is_immediate);

    lzt::append_memory_copy(sub_bundle.list, buffer, memory, size);
    lzt::close_command_list(sub_bundle.list);
    lzt::execute_and_sync_command_bundle(sub_bundle, UINT64_MAX);

    LOG_DEBUG << "[Child] Validating buffer received correctly";
    lzt::validate_data_pattern(buffer, size, 1);
    lzt::destroy_command_bundle(sub_bundle);
  }

  EXPECT_ZE_RESULT_SUCCESS(zeMemCloseIpcHandle(context, memory));
  lzt::free_memory(context, buffer);
  lzt::destroy_command_bundle(cmd_bundle);
  lzt::destroy_context(context);

  if (::testing::Test::HasFailure()) {
    exit(1);
  } else {
    exit(0);
  }
}

static void child_host_access_test_opaque(size_t size,
                                          ze_ipc_memory_flags_t flags,
                                          ze_ipc_mem_handle_t ipc_handle) {
  auto driver = lzt::get_default_driver();
  auto context = lzt::create_context(driver);
  auto device = lzt::zeDevice::get_instance()->get_device();
  void *buffer = nullptr;

  EXPECT_ZE_RESULT_SUCCESS(
      zeMemOpenIpcHandle(context, device, ipc_handle, flags, &buffer));

  LOG_DEBUG << "[Child] Validating buffer received correctly";
  lzt::validate_data_pattern(buffer, size, 1);

  EXPECT_ZE_RESULT_SUCCESS(zeMemCloseIpcHandle(context, buffer));
  lzt::destroy_context(context);

  if (::testing::Test::HasFailure()) {
    exit(1);
  } else {
    exit(0);
  }
}

static void child_multidevice_access_test_opaque(
    size_t size, ze_ipc_memory_flags_t flags, bool is_immediate,
    ze_ipc_mem_handle_t ipc_handle, uint32_t device_id_parent,
    uint32_t device_id_child, bool use_copy_engine) {
  auto driver = lzt::get_default_driver();
  auto context = lzt::create_context(driver);
  auto devices = lzt::get_ze_devices(driver);

  if (devices.size() <= device_id_child || devices.size() <= device_id_parent) {
    LOG_WARNING << "[Child] Device indices out of range. Required devices: "
                << device_id_parent << " and " << device_id_child
                << ", available: " << devices.size();
    exit(0);
  }

  // Use the device specified by parent for accessing memory
  auto device = devices[device_id_child];

  uint32_t ordinal = 0;
  if (use_copy_engine) {
    auto cmd_queue_group_props =
        lzt::get_command_queue_group_properties(device);
    auto copy_ordinal = lzt::get_queue_ordinal(
        cmd_queue_group_props, ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COPY,
        ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE);
    if (!copy_ordinal.has_value()) {
      LOG_WARNING << "[Child] No copy-only engine available on device "
                  << device_id_child << ". Skipping.";
      lzt::destroy_context(context);
      exit(0);
    }
    ordinal = *copy_ordinal;
  }

  auto cmd_bundle =
      use_copy_engine
          ? lzt::create_command_bundle(context, device, 0, ordinal,
                                       is_immediate)
          : lzt::create_command_bundle(context, device, is_immediate);
  void *memory = nullptr;

  EXPECT_ZE_RESULT_SUCCESS(
      zeMemOpenIpcHandle(context, device, ipc_handle, flags, &memory));

  void *buffer = lzt::allocate_host_memory(size, 1, context);
  memset(buffer, 0, size);
  lzt::append_memory_copy(cmd_bundle.list, buffer, memory, size);
  lzt::close_command_list(cmd_bundle.list);
  lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

  LOG_DEBUG << "[Child] Validating buffer received correctly on device "
            << device_id_child;
  lzt::validate_data_pattern(buffer, size, 1);

  EXPECT_ZE_RESULT_SUCCESS(zeMemCloseIpcHandle(context, memory));
  lzt::free_memory(context, buffer);
  lzt::destroy_command_bundle(cmd_bundle);
  lzt::destroy_context(context);

  if (::testing::Test::HasFailure()) {
    exit(1);
  } else {
    exit(0);
  }
}

// Child side of the IPC loop test.
// Phase 1: open all IPC handles simultaneously (accumulate remote pointers).
// Phase 2: for each iteration, copy device memory to a host buffer and verify
//          that every int32_t element equals the iteration index.
// Phase 3: close all handles in one batch – this exercises the driver's
//          ability to sustain many concurrently open IPC handles.
static void child_device_access_loop_test(const shared_data_loop_t &loop_data) {
  auto driver = lzt::get_default_driver();
  auto context = lzt::create_context(driver);
  auto device = lzt::zeDevice::get_instance()->get_device();

  const uint32_t n = loop_data.num_iterations;
  std::vector<void *> remote_ptrs(n, nullptr);

  // Phase 1: open all IPC handles (keep them all resident simultaneously)
  for (uint32_t i = 0; i < n; i++) {
    EXPECT_ZE_RESULT_SUCCESS(
        zeMemOpenIpcHandle(context, device, loop_data.ipc_handles[i],
                           loop_data.flags, &remote_ptrs[i]));
    LOG_DEBUG << "[Child] Opened IPC handle " << i;
  }

  // Phase 2: copy each remote buffer to host and verify the data pattern
  for (uint32_t i = 0; i < n; i++) {
    if (remote_ptrs[i] == nullptr) {
      continue;
    }
    auto cmd_bundle =
        lzt::create_command_bundle(context, device, loop_data.is_immediate);
    void *buffer = lzt::allocate_host_memory(loop_data.size, 1, context);
    memset(buffer, 0, loop_data.size);
    lzt::append_memory_copy(cmd_bundle.list, buffer, remote_ptrs[i],
                            loop_data.size);
    lzt::close_command_list(cmd_bundle.list);
    lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

    const int32_t *ptr = static_cast<const int32_t *>(buffer);
    bool match = true;
    for (size_t j = 0; j < loop_data.size / sizeof(int32_t); j++) {
      if (ptr[j] != static_cast<int32_t>(i)) {
        LOG_DEBUG << "[Child] Data mismatch at iteration " << i << " index "
                  << j << ": got " << ptr[j] << " expected " << i;
        match = false;
        break;
      }
    }
    EXPECT_TRUE(match) << "[Child] Data mismatch at loop iteration " << i;

    lzt::free_memory(context, buffer);
    lzt::destroy_command_bundle(cmd_bundle);
  }

  // Phase 3: close all handles in a batch (the key robustness check)
  LOG_DEBUG << "[Child] Closing all " << n << " IPC handles";
  for (uint32_t i = 0; i < n; i++) {
    if (remote_ptrs[i] != nullptr) {
      EXPECT_ZE_RESULT_SUCCESS(zeMemCloseIpcHandle(context, remote_ptrs[i]));
    }
  }

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
      std::this_thread::yield();
      std::this_thread::sleep_for(std::chrono::seconds(1));
      if (++count == retries)
        throw ex;
    }
  }

  shm.truncate(sizeof(shared_data_t));
  bipc::mapped_region region(shm, bipc::read_only);
  std::memcpy(&shared_data, region.get_address(), sizeof(shared_data_t));

  switch (shared_data.test_type) {
  case TEST_DEVICE_ACCESS:
    if (shared_data.test_sock_type == TEST_NONSOCK) {
      child_device_access_test_opaque(
          shared_data.size, shared_data.flags, shared_data.is_immediate,
          shared_data.ipc_handle, shared_data.use_copy_engine);
#ifdef __linux__
    } else {
      child_device_access_test(shared_data.size, shared_data.flags,
                               shared_data.is_immediate);
#endif
    }
    break;
  case TEST_SUBDEVICE_ACCESS:
    if (shared_data.test_sock_type == TEST_NONSOCK) {
      child_subdevice_access_test_opaque(
          shared_data.size, shared_data.flags, shared_data.is_immediate,
          shared_data.ipc_handle, shared_data.use_copy_engine);
    } else {
      break; // Currently supporting only device access test scenario
    }
    break;
  case TEST_MULTIDEVICE_ACCESS:
    if (shared_data.test_sock_type == TEST_NONSOCK) {
      child_multidevice_access_test_opaque(
          shared_data.size, shared_data.flags, shared_data.is_immediate,
          shared_data.ipc_handle, shared_data.device_id_parent,
          shared_data.device_id_child, shared_data.use_copy_engine);
    } else {
      break; // Currently supporting only opaque test scenario
    }
    break;
  case TEST_HOST_ACCESS:
    child_host_access_test_opaque(shared_data.size, shared_data.flags,
                                  shared_data.ipc_handle);
    break;
  case TEST_LOOP_ACCESS: {
    // Open the secondary shared memory that carries all IPC handles
    bipc::shared_memory_object shm_loop(
        bipc::open_only, "ipc_memory_loop_handles", bipc::read_only);
    bipc::mapped_region region_loop(shm_loop, bipc::read_only);
    const shared_data_loop_t *loop_data_ptr =
        static_cast<const shared_data_loop_t *>(region_loop.get_address());
    child_device_access_loop_test(*loop_data_ptr);
    break;
  }
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
