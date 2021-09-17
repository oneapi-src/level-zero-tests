/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */
#ifdef __linux__
#include <sys/socket.h>
#include <sys/types.h>
#endif

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"
#include "test_ipc_comm.hpp"

#include <level_zero/ze_api.h>

namespace {
#ifdef __linux__

void run_child(int size, ze_ipc_memory_flags_t flags) {
  ze_result_t result = zeInit(0);
  if (result) {
    throw std::runtime_error("zeInit failed: " +
                             level_zero_tests::to_string(result));
  }
  LOG_DEBUG << "[Server] Driver initialized\n";

  auto driver = lzt::get_default_driver();
  auto context = lzt::get_default_context();
  auto device = lzt::zeDevice::get_instance()->get_device();
  auto cl = lzt::create_command_list();
  auto cq = lzt::create_command_queue();
  ze_ipc_mem_handle_t ipc_handle;
  void *memory = nullptr;

  int ipc_descriptor = lzt::receive_ipc_handle<ze_ipc_mem_handle_t>();
  memcpy(&ipc_handle, static_cast<void *>(&ipc_descriptor),
         sizeof(ipc_descriptor));

  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeMemOpenIpcHandle(context, device, ipc_handle, flags, &memory));

  void *buffer = lzt::allocate_host_memory(size);
  memset(buffer, 0, size);
  lzt::append_memory_copy(cl, buffer, memory, size);
  lzt::close_command_list(cl);
  lzt::execute_command_lists(cq, 1, &cl, nullptr);
  lzt::synchronize(cq, UINT64_MAX);

  LOG_DEBUG << "[Server] Validating buffer received correctly";
  lzt::validate_data_pattern(buffer, size, 1);

  EXPECT_EQ(ZE_RESULT_SUCCESS, zeMemCloseIpcHandle(context, memory));
  lzt::free_memory(buffer);
  lzt::destroy_command_list(cl);
  lzt::destroy_command_queue(cq);
  lzt::destroy_context(context);

  if (::testing::Test::HasFailure()) {
    exit(1);
  } else {
    exit(0);
  }
}

void run_parent(int size) {
  ze_result_t result = zeInit(0);
  if (result) {
    throw std::runtime_error("zeInit failed: " +
                             level_zero_tests::to_string(result));
  }
  LOG_DEBUG << "[Client] Driver initialized\n";

  auto driver = lzt::get_default_driver();
  auto context = lzt::get_default_context();
  auto device = lzt::zeDevice::get_instance()->get_device();
  auto cl = lzt::create_command_list();
  auto cq = lzt::create_command_queue();
  ze_ipc_mem_handle_t ipc_handle;

  void *buffer = lzt::allocate_host_memory(size);
  memset(buffer, 0, size);
  lzt::write_data_pattern(buffer, size, 1);
  void *memory = lzt::allocate_device_memory(size);
  lzt::append_memory_copy(cl, memory, buffer, size);
  lzt::close_command_list(cl);
  lzt::execute_command_lists(cq, 1, &cl, nullptr);
  lzt::synchronize(cq, UINT64_MAX);

  ASSERT_EQ(ZE_RESULT_SUCCESS, zeMemGetIpcHandle(context, memory, &ipc_handle));
  lzt::send_ipc_handle(ipc_handle);

  // free device memory once receiver is done
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

  lzt::free_memory(memory);
  lzt::free_memory(buffer);
  lzt::destroy_command_list(cl);
  lzt::destroy_command_queue(cq);
  lzt::destroy_context(context);
}

TEST(
    IpcMemoryAccessTest,
    GivenL0MemoryAllocatedInChildProcessWhenUsingL0IPCThenParentProcessReadsMemoryCorrectly) {
  const auto size = 4096;
  pid_t pid = fork();
  if (pid < 0) {
    throw std::runtime_error("Failed to fork child process");
  } else if (pid > 0) {
    run_parent(size);
  } else {
    run_child(size, ZE_IPC_MEMORY_FLAG_BIAS_UNCACHED);
  }
}

TEST(
    IpcMemoryAccessTest,
    GivenL0MemoryAllocatedInChildProcessBiasCachedWhenUsingL0IPCThenParentProcessReadsMemoryCorrectly) {
  const auto size = 4096;
  pid_t pid = fork();
  if (pid < 0) {
    throw std::runtime_error("Failed to fork child process");
  } else if (pid > 0) {
    run_parent(size);
  } else {
    run_child(size, ZE_IPC_MEMORY_FLAG_BIAS_CACHED);
  }
}
#endif

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