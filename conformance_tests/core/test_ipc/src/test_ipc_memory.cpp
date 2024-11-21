/*
 *
 * Copyright (C) 2020-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */
#ifdef __linux__
#include <unistd.h>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/sync/named_semaphore.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/process.hpp>
#endif

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"
#include "test_ipc_memory.hpp"
#include "net/test_ipc_comm.hpp"

#include <level_zero/ze_api.h>

#ifdef __linux__

namespace bipc = boost::interprocess;

namespace {

static void run_ipc_mem_access_test(ipc_mem_access_test_t test_type, int size,
                                    bool reserved, ze_ipc_memory_flags_t flags,
                                    bool is_immediate) {
  ze_result_t result = zeInit(0);
  if (result != ZE_RESULT_SUCCESS) {
    throw std::runtime_error("Parent zeInit failed: " +
                             level_zero_tests::to_string(result));
  }
  LOG_DEBUG << "[Parent] Driver initialized\n";
  lzt::print_platform_overview();

  bipc::named_semaphore::remove("ipc_memory_test_semaphore");
  bipc::named_semaphore semaphore(bipc::create_only, "ipc_memory_test_semaphore", 0);

  bipc::shared_memory_object::remove("ipc_memory_test");
  // launch child
  boost::process::child c("./ipc/test_ipc_memory_helper");

  shared_data_t test_data = {test_type, size, flags, is_immediate};
  bipc::shared_memory_object shm(bipc::create_only, "ipc_memory_test",
                                 bipc::read_write);
  shm.truncate(sizeof(shared_data_t));
  bipc::mapped_region region(shm, bipc::read_write);
  std::memcpy(region.get_address(), &test_data, sizeof(shared_data_t));

  auto driver = lzt::get_default_driver();
  auto context = lzt::create_context(driver);
  auto device = lzt::zeDevice::get_instance()->get_device();
  auto cmd_bundle = lzt::create_command_bundle(context, device, is_immediate);
  ze_ipc_mem_handle_t ipc_handle = {};

  void *buffer = lzt::allocate_host_memory(size, 1, context);
  memset(buffer, 0, size);
  lzt::write_data_pattern(buffer, size, 1);
  size_t allocSize = size;
  ze_physical_mem_handle_t reservedPhysicalMemory = {};
  void *memory = nullptr;
  if (reserved) {
    memory = lzt::reserve_allocate_and_map_memory(context, device, allocSize,
                                                  &reservedPhysicalMemory);
  } else {
    memory = lzt::allocate_device_memory(size, 1, 0, context);
  }
  lzt::append_memory_copy(cmd_bundle.list, memory, buffer, size);
  lzt::close_command_list(cmd_bundle.list);
  lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeMemGetIpcHandle(context, memory, &ipc_handle));
  ze_ipc_mem_handle_t ipc_handle_zero{};
  ASSERT_NE(0, memcmp((void *)&ipc_handle, (void *)&ipc_handle_zero,
                      sizeof(ipc_handle)));

  // Wait until the child is ready to receive the IPC handle
  semaphore.wait();

  lzt::send_ipc_handle(ipc_handle);

  // Free device memory once receiver is done
  c.wait();
  EXPECT_EQ(c.exit_code(), 0);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeMemPutIpcHandle(context, ipc_handle));
  bipc::shared_memory_object::remove("ipc_memory_test");

  if (reserved) {
    lzt::unmap_and_free_reserved_memory(context, memory, reservedPhysicalMemory,
                                        allocSize);
  } else {
    lzt::free_memory(context, memory);
  }
  lzt::free_memory(context, buffer);
  lzt::destroy_command_bundle(cmd_bundle);
  lzt::destroy_context(context);
}

TEST(
    IpcMemoryAccessTest,
    GivenL0MemoryAllocatedInChildProcessWhenUsingL0IPCThenParentProcessReadsMemoryCorrectly) {
  run_ipc_mem_access_test(TEST_DEVICE_ACCESS, 4096, false,
                          ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED, false);
}

TEST(
    IpcMemoryAccessTest,
    GivenL0MemoryAllocatedInChildProcessWhenUsingL0IPCOnImmediateCmdListThenParentProcessReadsMemoryCorrectly) {
  run_ipc_mem_access_test(TEST_DEVICE_ACCESS, 4096, false,
                          ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED, true);
}

TEST(
    IpcMemoryAccessTest,
    GivenL0MemoryAllocatedInChildProcessBiasCachedWhenUsingL0IPCThenParentProcessReadsMemoryCorrectly) {
  run_ipc_mem_access_test(TEST_DEVICE_ACCESS, 4096, false,
                          ZE_IPC_MEMORY_FLAG_BIAS_CACHED, false);
}

TEST(
    IpcMemoryAccessTest,
    GivenL0MemoryAllocatedInChildProcessBiasCachedWhenUsingL0IPCOnImmediateCmdListThenParentProcessReadsMemoryCorrectly) {
  run_ipc_mem_access_test(TEST_DEVICE_ACCESS, 4096, false,
                          ZE_IPC_MEMORY_FLAG_BIAS_CACHED, true);
}

TEST(
    IpcMemoryAccessTest,
    GivenL0PhysicalMemoryAllocatedAndReservedInParentProcessWhenUsingL0IPCThenChildProcessReadsMemoryCorrectly) {
  run_ipc_mem_access_test(TEST_DEVICE_ACCESS, 4096, true,
                          ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED, false);
}

TEST(
    IpcMemoryAccessTest,
    GivenL0PhysicalMemoryAllocatedAndReservedInParentProcessWhenUsingL0IPCOnImmediateCmdListThenChildProcessReadsMemoryCorrectly) {
  run_ipc_mem_access_test(TEST_DEVICE_ACCESS, 4096, true,
                          ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED, true);
}

TEST(
    IpcMemoryAccessTest,
    GivenL0PhysicalMemoryAllocatedAndReservedInParentProcessBiasCachedWhenUsingL0IPCThenChildProcessReadsMemoryCorrectly) {
  run_ipc_mem_access_test(TEST_DEVICE_ACCESS, 4096, true,
                          ZE_IPC_MEMORY_FLAG_BIAS_CACHED, false);
}

TEST(
    IpcMemoryAccessTest,
    GivenL0PhysicalMemoryAllocatedAndReservedInParentProcessBiasCachedWhenUsingL0IPCOnImmediateCmdListThenChildProcessReadsMemoryCorrectly) {
  run_ipc_mem_access_test(TEST_DEVICE_ACCESS, 4096, true,
                          ZE_IPC_MEMORY_FLAG_BIAS_CACHED, true);
}

TEST(
    IpcMemoryAccessSubDeviceTest,
    GivenL0PhysicalMemoryAllocatedReservedInParentProcessWhenUsingL0IPCThenChildProcessReadsMemoryCorrectlyUsingSubDeviceQueue) {
  run_ipc_mem_access_test(TEST_SUBDEVICE_ACCESS, 4096, true,
                          ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED, false);
}

TEST(
    IpcMemoryAccessSubDeviceTest,
    GivenL0PhysicalMemoryAllocatedReservedInParentProcessWhenUsingL0IPCOnImmediateCmdListThenChildProcessReadsMemoryCorrectlyUsingSubDeviceQueue) {
  run_ipc_mem_access_test(TEST_SUBDEVICE_ACCESS, 4096, true,
                          ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED, true);
}

} // namespace

#endif // __linux__

// We put the main here because L0 doesn't currently specify how
// zeInit should be handled with fork(), so each process must call
// zeInit
int main(int argc, char **argv) {
  ::testing::InitGoogleMock(&argc, argv);
  std::vector<std::string> command_line(argv + 1, argv + argc);
  level_zero_tests::init_logging(command_line);

  try {
    auto result = RUN_ALL_TESTS();
    return result;
  } catch (std::runtime_error &e) {
    std::cerr << "Test failed with exception: " << e.what() << std::endl;
    return 1;
  }
}
