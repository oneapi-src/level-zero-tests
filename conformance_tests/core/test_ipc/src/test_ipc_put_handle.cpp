/*
 *
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */
#ifdef __linux__
#include <unistd.h>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/process.hpp>
#endif

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"
#include "test_ipc_put_handle.hpp"
#include "net/test_ipc_comm.hpp"

#include <level_zero/ze_api.h>

namespace {
#ifdef __linux__

namespace bipc = boost::interprocess;

static void run_ipc_put_handle_test(ipc_put_mem_access_test_t test_type,
                                    int size, bool reserved, bool getFromFd,
                                    ze_ipc_memory_flags_t flags,
                                    bool is_immediate) {
  ze_result_t result = zeInit(0);
  if (result != ZE_RESULT_SUCCESS) {
    throw std::runtime_error("Parent zeInit failed: " +
                             level_zero_tests::to_string(result));
  }
  LOG_DEBUG << "[Parent] Driver initialized\n";
  lzt::print_platform_overview();

  bipc::shared_memory_object::remove("ipc_put_handle_test");
  // launch child
  boost::process::child c("./ipc/test_ipc_put_handle_helper");

  shared_data_t test_data = {test_type, size, flags, is_immediate};
  bipc::shared_memory_object shm(bipc::create_only, "ipc_put_handle_test",
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
  if (is_immediate) {
    lzt::synchronize_command_list_host(cmd_bundle.list, UINT64_MAX);
  } else {
    lzt::close_command_list(cmd_bundle.list);
    lzt::execute_command_lists(cmd_bundle.queue, 1, &cmd_bundle.list, nullptr);
    lzt::synchronize(cmd_bundle.queue, UINT64_MAX);
  }

  if (getFromFd) {
    // set up request to export the external memory handle
    ze_external_memory_export_fd_t export_fd = {};
    export_fd.stype = ZE_STRUCTURE_TYPE_EXTERNAL_MEMORY_EXPORT_FD;
    export_fd.flags = ZE_EXTERNAL_MEMORY_TYPE_FLAG_DMA_BUF;
    ze_memory_allocation_properties_t alloc_props = {};
    alloc_props.stype = ZE_STRUCTURE_TYPE_MEMORY_ALLOCATION_PROPERTIES;
    alloc_props.pNext = &export_fd;
    lzt::get_mem_alloc_properties(context, memory, &alloc_props);
    ASSERT_EQ(ZE_RESULT_SUCCESS, zeMemGetIpcHandleFromFileDescriptorExp(
                                     context, export_fd.fd, &ipc_handle));
  } else {
    ASSERT_EQ(ZE_RESULT_SUCCESS,
              zeMemGetIpcHandle(context, memory, &ipc_handle));
  }
  ze_ipc_mem_handle_t ipc_handle_zero{};
  ASSERT_NE(0, memcmp((void *)&ipc_handle, (void *)&ipc_handle_zero,
                      sizeof(ipc_handle)));
  lzt::send_ipc_handle(ipc_handle);

  // Free device memory once receiver is done
  int child_status;
  pid_t clientPId = wait(&child_status);
  if (clientPId < 0) {
    std::cerr << "Error waiting for receiver process " << strerror(errno)
              << "\n";
  }

  if (WIFEXITED(child_status)) {
    if (WEXITSTATUS(child_status)) {
      FAIL() << "Receiver process failed memory verification\n";
    }
  }

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeMemPutIpcHandle(context, ipc_handle));
  bipc::shared_memory_object::remove("ipc_put_handle_test");

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
    PutIpcMemoryAccessTest,
    GivenL0MemoryAllocatedInParentProcessWhenUsingL0IPCThenChildProcessReadsMemoryCorrectlyWithPutIpcHandle) {
  run_ipc_put_handle_test(TEST_PUT_DEVICE_ACCESS, 4096, false, false,
                          ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED, false);
}

TEST(
    PutIpcMemoryAccessTest,
    GivenL0MemoryAllocatedInParentProcessWhenUsingL0IPCOnImmediateCmdListThenChildProcessReadsMemoryCorrectlyWithPutIpcHandle) {
  run_ipc_put_handle_test(TEST_PUT_DEVICE_ACCESS, 4096, false, false,
                          ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED, true);
}

TEST(
    PutIpcMemoryAccessTest,
    GivenL0MemoryAllocatedInParentProcessBiasCachedWhenUsingL0IPCThenChildProcessReadsMemoryCorrectlyWithPutIpcHandle) {
  run_ipc_put_handle_test(TEST_PUT_DEVICE_ACCESS, 4096, false, false,
                          ZE_IPC_MEMORY_FLAG_BIAS_CACHED, false);
}

TEST(
    PutIpcMemoryAccessTest,
    GivenL0MemoryAllocatedInParentProcessBiasCachedWhenUsingL0IPCOnImmediateCmdListThenChildProcessReadsMemoryCorrectlyWithPutIpcHandle) {
  run_ipc_put_handle_test(TEST_PUT_DEVICE_ACCESS, 4096, false, false,
                          ZE_IPC_MEMORY_FLAG_BIAS_CACHED, true);
}

TEST(
    PutIpcMemoryAccessTest,
    GivenL0PhysicalMemoryAllocatedAndReservedInParentProcessWhenUsingL0IPCThenChildProcessReadsMemoryCorrectlyWithPutIpcHandle) {
  run_ipc_put_handle_test(TEST_PUT_DEVICE_ACCESS, 4096, true, false,
                          ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED, false);
}

TEST(
    PutIpcMemoryAccessTest,
    GivenL0PhysicalMemoryAllocatedAndReservedInParentProcessWhenUsingL0IPCOnImmediateCmdListThenChildProcessReadsMemoryCorrectlyWithPutIpcHandle) {
  run_ipc_put_handle_test(TEST_PUT_DEVICE_ACCESS, 4096, true, false,
                          ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED, true);
}

TEST(
    PutIpcMemoryAccessTest,
    GivenL0PhysicalMemoryAllocatedAndReservedInParentProcessBiasCachedWhenUsingL0IPCThenChildProcessReadsMemoryCorrectlyWithPutIpcHandle) {
  run_ipc_put_handle_test(TEST_PUT_DEVICE_ACCESS, 4096, true, false,
                          ZE_IPC_MEMORY_FLAG_BIAS_CACHED, false);
}

TEST(
    PutIpcMemoryAccessTest,
    GivenL0PhysicalMemoryAllocatedAndReservedInParentProcessBiasCachedWhenUsingL0IPCOnImmediateCmdListThenChildProcessReadsMemoryCorrectlyWithPutIpcHandle) {
  run_ipc_put_handle_test(TEST_PUT_DEVICE_ACCESS, 4096, true, false,
                          ZE_IPC_MEMORY_FLAG_BIAS_CACHED, true);
}

TEST(
    PutIpcMemoryAccessTest,
    GivenL0MemoryAllocatedInParentProcessWhenUsingL0IPCThenChildProcessReadsMemoryCorrectlyWithGetHandleFromFd) {
  run_ipc_put_handle_test(TEST_PUT_DEVICE_ACCESS, 4096, false, true,
                          ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED, false);
}

TEST(
    PutIpcMemoryAccessTest,
    GivenL0MemoryAllocatedInParentProcessWhenUsingL0IPCOnImmediateCmdListThenChildProcessReadsMemoryCorrectlyWithGetHandleFromFd) {
  run_ipc_put_handle_test(TEST_PUT_DEVICE_ACCESS, 4096, false, true,
                          ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED, true);
}

TEST(
    PutIpcMemoryAccessTest,
    GivenL0MemoryAllocatedInParentProcessBiasCachedWhenUsingL0IPCThenChildProcessReadsMemoryCorrectlyWithGetHandleFromFd) {
  run_ipc_put_handle_test(TEST_PUT_DEVICE_ACCESS, 4096, false, true,
                          ZE_IPC_MEMORY_FLAG_BIAS_CACHED, false);
}

TEST(
    PutIpcMemoryAccessTest,
    GivenL0MemoryAllocatedInParentProcessBiasCachedWhenUsingL0IPCOnImmediateCmdListThenChildProcessReadsMemoryCorrectlyWithGetHandleFromFd) {
  run_ipc_put_handle_test(TEST_PUT_DEVICE_ACCESS, 4096, false, true,
                          ZE_IPC_MEMORY_FLAG_BIAS_CACHED, true);
}

TEST(
    PutIpcMemoryAccessTest,
    GivenL0PhysicalMemoryAllocatedAndReservedInParentProcessWhenUsingL0IPCThenChildProcessReadsMemoryCorrectlyWithGetHandleFromFd) {
  run_ipc_put_handle_test(TEST_PUT_DEVICE_ACCESS, 4096, true, true,
                          ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED, false);
}

TEST(
    PutIpcMemoryAccessTest,
    GivenL0PhysicalMemoryAllocatedAndReservedInParentProcessWhenUsingL0IPCOnImmediateCmdListThenChildProcessReadsMemoryCorrectlyWithGetHandleFromFd) {
  run_ipc_put_handle_test(TEST_PUT_DEVICE_ACCESS, 4096, true, true,
                          ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED, true);
}

TEST(
    PutIpcMemoryAccessTest,
    GivenL0PhysicalMemoryAllocatedAndReservedInParentProcessBiasCachedWhenUsingL0IPCThenChildProcessReadsMemoryCorrectlyWithGetHandleFromFd) {
  run_ipc_put_handle_test(TEST_PUT_DEVICE_ACCESS, 4096, true, true,
                          ZE_IPC_MEMORY_FLAG_BIAS_CACHED, false);
}

TEST(
    PutIpcMemoryAccessTest,
    GivenL0PhysicalMemoryAllocatedAndReservedInParentProcessBiasCachedWhenUsingL0IPCOnImmediateCmdListThenChildProcessReadsMemoryCorrectlyWithGetHandleFromFd) {
  run_ipc_put_handle_test(TEST_PUT_DEVICE_ACCESS, 4096, true, true,
                          ZE_IPC_MEMORY_FLAG_BIAS_CACHED, true);
}

TEST(
    PutIpcMemoryAccessSubDeviceTest,
    GivenL0PhysicalMemoryAllocatedReservedInParentProcessWhenUsingL0IPCThenChildProcessReadsMemoryCorrectlyUsingSubDeviceQueueWithPutIpcHandle) {
  run_ipc_put_handle_test(TEST_PUT_SUBDEVICE_ACCESS, 4096, true, false,
                          ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED, false);
}

TEST(
    PutIpcMemoryAccessSubDeviceTest,
    GivenL0PhysicalMemoryAllocatedReservedInParentProcessWhenUsingL0IPCOnImmediateCmdListThenChildProcessReadsMemoryCorrectlyUsingSubDeviceQueueWithPutIpcHandle) {
  run_ipc_put_handle_test(TEST_PUT_SUBDEVICE_ACCESS, 4096, true, false,
                          ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED, true);
}

TEST(
    PutIpcMemoryAccessSubDeviceTest,
    GivenL0PhysicalMemoryAllocatedReservedInParentProcessWhenUsingL0IPCThenChildProcessReadsMemoryCorrectlyUsingSubDeviceQueueWithGetHandleFromFd) {
  run_ipc_put_handle_test(TEST_PUT_SUBDEVICE_ACCESS, 4096, true, true,
                          ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED, false);
}

TEST(
    PutIpcMemoryAccessSubDeviceTest,
    GivenL0PhysicalMemoryAllocatedReservedInParentProcessWhenUsingL0IPCOnImmediateCmdListThenChildProcessReadsMemoryCorrectlyUsingSubDeviceQueueWithGetHandleFromFd) {
  run_ipc_put_handle_test(TEST_PUT_SUBDEVICE_ACCESS, 4096, true, true,
                          ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED, true);
}

#endif // __linux__

class zePutIpcMemHandleTests : public ::testing::Test {
protected:
  void *memory_ = nullptr;
  void *ipc_memory_ = nullptr;
  ze_ipc_mem_handle_t ipc_mem_handle_;
  ze_context_handle_t context_;
};

TEST_F(
    zePutIpcMemHandleTests,
    GivenDeviceMemoryAllocationWhenGettingIpcMemoryHandleThenReleasingHandleWithPutThenSuccessIsReturned) {
  ze_result_t result = zeInit(0);
  if (result) {
    throw std::runtime_error("zeInit failed: " +
                             level_zero_tests::to_string(result));
  }
  LOG_TRACE << "Driver initialized";
  level_zero_tests::print_platform_overview();
  context_ = lzt::create_context();
  memory_ = lzt::allocate_device_memory(1, 1, 0, context_);

  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeMemGetIpcHandle(context_, memory_, &ipc_mem_handle_));
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeMemPutIpcHandle(context_, ipc_mem_handle_));
  lzt::free_memory(context_, memory_);
  lzt::destroy_context(context_);
}

TEST_F(
    zePutIpcMemHandleTests,
    GivenDeviceMemoryAllocationWhenCallingGettingIpcMemoryHandleMultipleTimesThenReleasingHandleWithMuliplePutsThenSuccessIsReturned) {
  ze_result_t result = zeInit(0);
  if (result) {
    throw std::runtime_error("zeInit failed: " +
                             level_zero_tests::to_string(result));
  }
  LOG_TRACE << "Driver initialized";
  level_zero_tests::print_platform_overview();
  context_ = lzt::create_context();
  memory_ = lzt::allocate_device_memory(1, 1, 0, context_);

  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeMemGetIpcHandle(context_, memory_, &ipc_mem_handle_));
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeMemGetIpcHandle(context_, memory_, &ipc_mem_handle_));
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeMemPutIpcHandle(context_, ipc_mem_handle_));
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeMemPutIpcHandle(context_, ipc_mem_handle_));
  lzt::free_memory(context_, memory_);
  lzt::destroy_context(context_);
}

TEST_F(
    zePutIpcMemHandleTests,
    GivenDeviceMemoryAllocationWhenCallingGettingIpcMemoryHandleMultipleTimesThenReleasingHandleWithMuliplePutsThenTrailingGetSuccessIsReturned) {
  ze_result_t result = zeInit(0);
  if (result) {
    throw std::runtime_error("zeInit failed: " +
                             level_zero_tests::to_string(result));
  }
  LOG_TRACE << "Driver initialized";
  level_zero_tests::print_platform_overview();
  context_ = lzt::create_context();
  memory_ = lzt::allocate_device_memory(1, 1, 0, context_);

  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeMemGetIpcHandle(context_, memory_, &ipc_mem_handle_));
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeMemGetIpcHandle(context_, memory_, &ipc_mem_handle_));
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeMemPutIpcHandle(context_, ipc_mem_handle_));
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeMemPutIpcHandle(context_, ipc_mem_handle_));
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeMemGetIpcHandle(context_, memory_, &ipc_mem_handle_));
  lzt::free_memory(context_, memory_);
  lzt::destroy_context(context_);
}

TEST_F(
    zePutIpcMemHandleTests,
    GivenDeviceMemoryAllocationWhenCallingGettingIpcMemoryHandleMultipleTimesThenReleasingHandleWithSinglePutThenSuccessIsReturned) {
  ze_result_t result = zeInit(0);
  if (result) {
    throw std::runtime_error("zeInit failed: " +
                             level_zero_tests::to_string(result));
  }
  LOG_TRACE << "Driver initialized";
  level_zero_tests::print_platform_overview();
  context_ = lzt::create_context();
  memory_ = lzt::allocate_device_memory(1, 1, 0, context_);

  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeMemGetIpcHandle(context_, memory_, &ipc_mem_handle_));
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeMemGetIpcHandle(context_, memory_, &ipc_mem_handle_));
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeMemPutIpcHandle(context_, ipc_mem_handle_));
  lzt::free_memory(context_, memory_);
  lzt::destroy_context(context_);
}

TEST_F(
    zePutIpcMemHandleTests,
    GivenDeviceMemoryAllocationWhenCallingPutIpcMemoryWithDifferentContextsReturnsSuccess) {
  ze_result_t result = zeInit(0);
  if (result) {
    throw std::runtime_error("zeInit failed: " +
                             level_zero_tests::to_string(result));
  }
  LOG_TRACE << "Driver initialized";
  level_zero_tests::print_platform_overview();
  context_ = lzt::create_context();
  auto context2 = lzt::create_context();
  memory_ = lzt::allocate_device_memory(1, 1, 0, context_);

  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeMemGetIpcHandle(context_, memory_, &ipc_mem_handle_));
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeMemPutIpcHandle(context_, ipc_mem_handle_));
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeMemPutIpcHandle(context2, ipc_mem_handle_));
  lzt::free_memory(context_, memory_);
  lzt::destroy_context(context_);
  lzt::destroy_context(context2);
}

TEST_F(
    zePutIpcMemHandleTests,
    GivenDeviceMemoryAllocationWhenCallingGettingFileDescriptorFromIpcHandleThenFileDescriptorReturnedMatchingAllocation) {
  ze_result_t result = zeInit(0);
  if (result) {
    throw std::runtime_error("zeInit failed: " +
                             level_zero_tests::to_string(result));
  }
  LOG_TRACE << "Driver initialized";
  level_zero_tests::print_platform_overview();
  auto device = lzt::get_default_device(lzt::get_default_driver());

  auto external_memory_properties = lzt::get_external_memory_properties(device);
  if (!(external_memory_properties.memoryAllocationExportTypes &
        ZE_EXTERNAL_MEMORY_TYPE_FLAG_DMA_BUF)) {
    GTEST_SKIP() << "Device does not support exporting DMA_BUF";
  }

  context_ = lzt::create_context();
  ze_external_memory_export_desc_t export_desc = {};
  export_desc.stype = ZE_STRUCTURE_TYPE_EXTERNAL_MEMORY_EXPORT_DESC;
  export_desc.flags = ZE_EXTERNAL_MEMORY_TYPE_FLAG_DMA_BUF;

  memory_ =
      lzt::allocate_device_memory(1, 1, 0, &export_desc, 0, device, context_);
  // set up request to export the external memory handle
  ze_external_memory_export_fd_t export_fd = {};
  export_fd.stype = ZE_STRUCTURE_TYPE_EXTERNAL_MEMORY_EXPORT_FD;
  export_fd.flags = ZE_EXTERNAL_MEMORY_TYPE_FLAG_DMA_BUF;
  ze_memory_allocation_properties_t alloc_props = {};
  alloc_props.stype = ZE_STRUCTURE_TYPE_MEMORY_ALLOCATION_PROPERTIES;
  alloc_props.pNext = &export_fd;
  lzt::get_mem_alloc_properties(context_, memory_, &alloc_props);

  uint64_t fd_handle = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeMemGetIpcHandle(context_, memory_, &ipc_mem_handle_));
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeMemGetFileDescriptorFromIpcHandleExp(
                                   context_, ipc_mem_handle_, &fd_handle));
  EXPECT_EQ(export_fd.fd, fd_handle);
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeMemPutIpcHandle(context_, ipc_mem_handle_));
  lzt::free_memory(context_, memory_);
  lzt::destroy_context(context_);
}

TEST_F(
    zePutIpcMemHandleTests,
    GivenDeviceMemoryAllocationWhenCallingGettingIpcHandleFromFileDescriptorThenOpenIpcHandleIsSuccessfull) {
  ze_result_t result = zeInit(0);
  if (result) {
    throw std::runtime_error("zeInit failed: " +
                             level_zero_tests::to_string(result));
  }
  LOG_TRACE << "Driver initialized";
  level_zero_tests::print_platform_overview();
  auto device = lzt::get_default_device(lzt::get_default_driver());

  auto external_memory_properties = lzt::get_external_memory_properties(device);
  if (!(external_memory_properties.memoryAllocationExportTypes &
        ZE_EXTERNAL_MEMORY_TYPE_FLAG_DMA_BUF)) {
    GTEST_SKIP() << "Device does not support exporting DMA_BUF";
  }

  context_ = lzt::create_context();
  ze_external_memory_export_desc_t export_desc = {};
  export_desc.stype = ZE_STRUCTURE_TYPE_EXTERNAL_MEMORY_EXPORT_DESC;
  export_desc.flags = ZE_EXTERNAL_MEMORY_TYPE_FLAG_DMA_BUF;

  memory_ =
      lzt::allocate_device_memory(1, 1, 0, &export_desc, 0, device, context_);
  // set up request to export the external memory handle
  ze_external_memory_export_fd_t export_fd = {};
  export_fd.stype = ZE_STRUCTURE_TYPE_EXTERNAL_MEMORY_EXPORT_FD;
  export_fd.flags = ZE_EXTERNAL_MEMORY_TYPE_FLAG_DMA_BUF;
  ze_memory_allocation_properties_t alloc_props = {};
  alloc_props.stype = ZE_STRUCTURE_TYPE_MEMORY_ALLOCATION_PROPERTIES;
  alloc_props.pNext = &export_fd;
  lzt::get_mem_alloc_properties(context_, memory_, &alloc_props);

  uint64_t fd_handle = 0;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeMemGetIpcHandleFromFileDescriptorExp(
                                   context_, export_fd.fd, &ipc_mem_handle_));
  void *memory = nullptr;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeMemOpenIpcHandle(context_, device, ipc_mem_handle_,
                               ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED, &memory));
  EXPECT_NE(nullptr, memory);
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeMemCloseIpcHandle(context_, memory));
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeMemPutIpcHandle(context_, ipc_mem_handle_));
  lzt::free_memory(context_, memory_);
  lzt::destroy_context(context_);
}

} // namespace

// We put the main here because L0 doesn't currently specify how
// zeInit should be handled with fork(), so each process must call
// zeInit
int main(int argc, char **argv) {
  ::testing::InitGoogleMock(&argc, argv);
  std::vector<std::string> command_line(argv + 1, argv + argc);
  level_zero_tests::init_logging(command_line);

  return RUN_ALL_TESTS();
}
