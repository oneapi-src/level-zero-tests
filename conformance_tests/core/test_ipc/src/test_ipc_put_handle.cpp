/*
 *
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */
#ifdef __linux__
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"
#include "net/test_ipc_comm.hpp"

#include <level_zero/ze_api.h>

namespace {
#ifdef __linux__

class PutIpcMemoryAccessTest : public ::testing::Test {
public:
  virtual void run_child(int size, ze_ipc_memory_flags_t flags);
  void run_parent(int size, bool reserved, bool getFromFd);

protected:
  pid_t pid;
  bool is_parent;
  bool child_exited = false;

  void SetUp() override {
    pid = fork();
    if (pid < 0) {
      throw std::runtime_error("Failed to fork child process");
    }
    is_parent = pid > 0;
  }

  void TearDown() override {
    if (is_parent && !child_exited) {
      kill(pid, SIGTERM);
      wait(nullptr);
    }
  }
};

void PutIpcMemoryAccessTest::run_child(int size, ze_ipc_memory_flags_t flags) {
  ze_result_t result = zeInit(0);
  if (result) {
    throw std::runtime_error("zeInit failed: " +
                             level_zero_tests::to_string(result));
  }
  LOG_DEBUG << "[Server] Driver initialized\n";

  auto driver = lzt::get_default_driver();
  auto context = lzt::create_context(driver);
  auto device = lzt::zeDevice::get_instance()->get_device();
  auto cl = lzt::create_command_list(device);
  auto cq = lzt::create_command_queue(device);
  ze_ipc_mem_handle_t ipc_handle;
  void *memory = nullptr;

  int ipc_descriptor =
      lzt::receive_ipc_handle<ze_ipc_mem_handle_t>(ipc_handle.data);
  memcpy(&ipc_handle, static_cast<void *>(&ipc_descriptor),
         sizeof(ipc_descriptor));

  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeMemOpenIpcHandle(context, device, ipc_handle, flags, &memory));

  void *buffer = lzt::allocate_host_memory(size, 1, context);
  memset(buffer, 0, size);
  lzt::append_memory_copy(cl, buffer, memory, size);
  lzt::close_command_list(cl);
  lzt::execute_command_lists(cq, 1, &cl, nullptr);
  lzt::synchronize(cq, UINT64_MAX);

  LOG_DEBUG << "[Server] Validating buffer received correctly";
  lzt::validate_data_pattern(buffer, size, 1);

  EXPECT_EQ(ZE_RESULT_SUCCESS, zeMemCloseIpcHandle(context, memory));
  lzt::free_memory(context, buffer);
  lzt::destroy_command_list(cl);
  lzt::destroy_command_queue(cq);
  lzt::destroy_context(context);

  if (::testing::Test::HasFailure()) {
    exit(1);
  } else {
    exit(0);
  }
}

void PutIpcMemoryAccessTest::run_parent(int size, bool reserved,
                                        bool getFromFd) {
  ze_result_t result = zeInit(0);
  if (result) {
    throw std::runtime_error("zeInit failed: " +
                             level_zero_tests::to_string(result));
  }
  LOG_DEBUG << "[Client] Driver initialized\n";

  auto driver = lzt::get_default_driver();
  auto context = lzt::create_context(driver);
  auto device = lzt::zeDevice::get_instance()->get_device();
  auto cl = lzt::create_command_list(device);
  auto cq = lzt::create_command_queue(device);
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
  lzt::append_memory_copy(cl, memory, buffer, size);
  lzt::close_command_list(cl);
  lzt::execute_command_lists(cq, 1, &cl, nullptr);
  lzt::synchronize(cq, UINT64_MAX);

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
  ze_ipc_mem_handle_t ipc_handle_zero = {};
  ASSERT_NE(0, memcmp((void *)&ipc_handle, (void *)&ipc_handle_zero,
                      sizeof(ipc_handle)));
  lzt::send_ipc_handle(ipc_handle);

  // free device memory once receiver is done
  int child_status;
  pid_t clientPId = wait(&child_status);
  if (clientPId < 0) {
    std::cerr << "Error waiting for receiver process " << strerror(errno)
              << "\n";
  }

  child_exited = true;
  if (WIFEXITED(child_status)) {
    if (WEXITSTATUS(child_status)) {
      FAIL() << "Receiver process failed memory verification\n";
    }
  }

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeMemPutIpcHandle(context, ipc_handle));

  if (reserved) {
    lzt::unmap_and_free_reserved_memory(context, memory, reservedPhysicalMemory,
                                        allocSize);
  } else {
    lzt::free_memory(context, memory);
  }
  lzt::free_memory(context, buffer);
  lzt::destroy_command_list(cl);
  lzt::destroy_command_queue(cq);
  lzt::destroy_context(context);
}

TEST_F(
    PutIpcMemoryAccessTest,
    GivenL0MemoryAllocatedInParentProcessWhenUsingL0IPCThenChildProcessReadsMemoryCorrectlyWithPutIpcHandle) {
  const auto size = 4096;
  if (is_parent) {
    run_parent(size, false, false);
  } else {
    run_child(size, ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED);
  }
}

TEST_F(
    PutIpcMemoryAccessTest,
    GivenL0MemoryAllocatedInParentProcessBiasCachedWhenUsingL0IPCThenChildProcessReadsMemoryCorrectlyWithPutIpcHandle) {
  const auto size = 4096;
  if (is_parent) {
    run_parent(size, false, false);
  } else {
    run_child(size, ZE_IPC_MEMORY_FLAG_BIAS_CACHED);
  }
}

TEST_F(
    PutIpcMemoryAccessTest,
    GivenL0PhysicalMemoryAllocatedAndReservedInParentProcessWhenUsingL0IPCThenChildProcessReadsMemoryCorrectlyWithPutIpcHandle) {
  const auto size = 4096;
  if (is_parent) {
    run_parent(size, true, false);
  } else {
    run_child(size, ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED);
  }
}

TEST_F(
    PutIpcMemoryAccessTest,
    GivenL0PhysicalMemoryAllocatedAndReservedInParentProcessBiasCachedWhenUsingL0IPCThenChildProcessReadsMemoryCorrectlyWithPutIpcHandle) {
  const auto size = 4096;
  if (is_parent) {
    run_parent(size, true, false);
  } else {
    run_child(size, ZE_IPC_MEMORY_FLAG_BIAS_CACHED);
  }
}

TEST_F(
    PutIpcMemoryAccessTest,
    GivenL0MemoryAllocatedInParentProcessWhenUsingL0IPCThenChildProcessReadsMemoryCorrectlyWithGetHandleFromFd) {
  const auto size = 4096;
  if (is_parent) {
    run_parent(size, false, true);
  } else {
    run_child(size, ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED);
  }
}

TEST_F(
    PutIpcMemoryAccessTest,
    GivenL0MemoryAllocatedInParentProcessBiasCachedWhenUsingL0IPCThenChildProcessReadsMemoryCorrectlyWithGetHandleFromFd) {
  const auto size = 4096;
  if (is_parent) {
    run_parent(size, false, true);
  } else {
    run_child(size, ZE_IPC_MEMORY_FLAG_BIAS_CACHED);
  }
}

TEST_F(
    PutIpcMemoryAccessTest,
    GivenL0PhysicalMemoryAllocatedAndReservedInParentProcessWhenUsingL0IPCThenChildProcessReadsMemoryCorrectlyWithGetHandleFromFd) {
  const auto size = 4096;
  if (is_parent) {
    run_parent(size, true, true);
  } else {
    run_child(size, ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED);
  }
}

TEST_F(
    PutIpcMemoryAccessTest,
    GivenL0PhysicalMemoryAllocatedAndReservedInParentProcessBiasCachedWhenUsingL0IPCThenChildProcessReadsMemoryCorrectlyWithGetHandleFromFd) {
  const auto size = 4096;
  if (is_parent) {
    run_parent(size, true, true);
  } else {
    run_child(size, ZE_IPC_MEMORY_FLAG_BIAS_CACHED);
  }
}

class PutIpcMemoryAccessSubDeviceTest : public PutIpcMemoryAccessTest {
public:
  void run_child(int size, ze_ipc_memory_flags_t flags) override;

protected:
  pid_t pid;
  bool is_parent;
  bool child_exited = false;

  void SetUp() override {
    pid = fork();
    if (pid < 0) {
      throw std::runtime_error("Failed to fork child process");
    }
    is_parent = pid > 0;
  }

  void TearDown() override {
    if (is_parent && !child_exited) {
      kill(pid, SIGTERM);
      wait(nullptr);
    }
  }
};

void PutIpcMemoryAccessSubDeviceTest::run_child(int size,
                                                ze_ipc_memory_flags_t flags) {
  ze_result_t result = zeInit(0);
  if (result) {
    throw std::runtime_error("zeInit failed: " +
                             level_zero_tests::to_string(result));
  }
  LOG_DEBUG << "[Server] Driver initialized\n";

  auto driver = lzt::get_default_driver();
  auto context = lzt::create_context(driver);
  auto device = lzt::zeDevice::get_instance()->get_device();
  auto sub_devices = lzt::get_ze_sub_devices(device);

  auto sub_device_count = sub_devices.size();

  ze_ipc_mem_handle_t ipc_handle;
  void *memory = nullptr;

  int ipc_descriptor =
      lzt::receive_ipc_handle<ze_ipc_mem_handle_t>(ipc_handle.data);
  memcpy(&ipc_handle, static_cast<void *>(&ipc_descriptor),
         sizeof(ipc_descriptor));

  // Open IPc buffer with root device
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeMemOpenIpcHandle(context, device, ipc_handle, flags, &memory));

  void *buffer = lzt::allocate_host_memory(size, 1, context);
  memset(buffer, 0, size);

  // For each sub device found, use IPC buffer in a copy operation and validate
  for (auto i = 0; i < sub_device_count; i++) {
    auto cl = lzt::create_command_list(sub_devices[i]);
    auto cq = lzt::create_command_queue(sub_devices[i]);

    lzt::append_memory_copy(cl, buffer, memory, size);
    lzt::close_command_list(cl);
    lzt::execute_command_lists(cq, 1, &cl, nullptr);
    lzt::synchronize(cq, UINT64_MAX);

    LOG_DEBUG << "[Server] Validating buffer received correctly";
    lzt::validate_data_pattern(buffer, size, 1);
    lzt::destroy_command_list(cl);
    lzt::destroy_command_queue(cq);
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

TEST_F(
    PutIpcMemoryAccessSubDeviceTest,
    GivenL0PhysicalMemoryAllocatedReservedInParentProcessWhenUsingL0IPCThenChildProcessReadsMemoryCorrectlyUsingSubDeviceQueueWithPutIpcHandle) {
  const auto size = 4096;
  if (is_parent) {
    run_parent(size, true, false);
  } else {
    run_child(size, ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED);
  }
}

TEST_F(
    PutIpcMemoryAccessSubDeviceTest,
    GivenL0PhysicalMemoryAllocatedReservedInParentProcessWhenUsingL0IPCThenChildProcessReadsMemoryCorrectlyUsingSubDeviceQueueWithGetHandleFromFd) {
  const auto size = 4096;
  if (is_parent) {
    run_parent(size, true, true);
  } else {
    run_child(size, ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED);
  }
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