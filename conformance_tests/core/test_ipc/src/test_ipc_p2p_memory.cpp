/*
 *
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <chrono>
#include <thread>
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
#include <math.h>

namespace {
#ifdef __linux__

uint8_t initial_pattern = 0;
uint8_t first_pattern = 1;
uint8_t second_pattern = 2;

bool check_p2p_capabilities(std::vector<ze_device_handle_t> devices) {
  ze_bool_t can_access_peer = false;
  zeDeviceCanAccessPeer(devices[0], devices[1], &can_access_peer);

  if (can_access_peer == false) {
    LOG_WARNING << "Two devices found but no P2P capabilities detected\n";
  } else {
    LOG_DEBUG << "Two devices found and P2P capabilities detected\n";
  }

  return can_access_peer;
}

void sigint_handler(int signal_number) {
  LOG_DEBUG << "Received Signal: " << signal_number << "\n";
}

void init_driver() {
  ze_result_t result = zeInit(0);
  if (result) {
    throw std::runtime_error("zeInit failed: " +
                             level_zero_tests::to_string(result));
  }
  LOG_DEBUG << "Driver initialized\n";
}

void fill_expected_buffer(char *expected_buffer, int size,
                          int concurrency_offset) {
  if (concurrency_offset > 0) {
    for (size_t i = 0; i < size - concurrency_offset; ++i) {
      expected_buffer[i] = first_pattern;
    }
    for (size_t i = concurrency_offset; i < size; ++i) {
      expected_buffer[i] = second_pattern;
    }
  } else {
    for (size_t i = 0; i < size; ++i) {
      expected_buffer[i] = first_pattern;
    }
  }
}

inline void init_process(ze_context_handle_t &context,
                         ze_device_handle_t &device,
                         ze_command_queue_handle_t &cq,
                         ze_command_list_handle_t &cl, bool is_server,
                         int concurrency_offset) {
  init_driver();

  auto driver = lzt::get_default_driver();
  context = lzt::get_default_context();

  uint32_t device_count = 0;
  zeDeviceGet(driver, &device_count, nullptr);
  std::vector<ze_device_handle_t> devices(device_count);
  zeDeviceGet(driver, &device_count, devices.data());

  if (device_count > 1) {
    if (check_p2p_capabilities(devices) == true) {
      if (is_server) {
        device = devices[0];
      } else {
        device = devices[1];
      }
    } else {
      std::exit(0);
    }
  } else {
    LOG_WARNING << "Only one device found, requires two for P2P test\n";
    std::exit(0);
  }

  if (concurrency_offset > 0) {
    ze_device_p2p_properties_t peerProperties;

    zeDeviceGetP2PProperties(devices[0], devices[1], &peerProperties);
    if ((peerProperties.flags & ZE_DEVICE_P2P_PROPERTY_FLAG_ATOMICS) == 0) {
      LOG_WARNING << "P2P atomic access unsupported, required for concurrent "
                     "access tests\n";
      std::exit(0);
    }
  }

  cl = lzt::create_command_list(device);
  cq = lzt::create_command_queue(device);

  signal(SIGINT, sigint_handler);

  return;
}

void run_server(int size, int concurrency_offset, bool bidirectional,
                pid_t pid) {
  ze_context_handle_t context;
  ze_device_handle_t device;
  ze_command_queue_handle_t cq;
  ze_command_list_handle_t cl;

  init_process(context, device, cq, cl, true, concurrency_offset);

  void *memory = lzt::allocate_device_memory(size, 1, 0, device, context);
  lzt::append_memory_fill(cl, memory, &initial_pattern, sizeof(initial_pattern),
                          size, nullptr);
  lzt::close_command_list(cl);
  lzt::execute_command_lists(cq, 1, &cl, nullptr);
  lzt::synchronize(cq, UINT64_MAX);
  lzt::reset_command_list(cl);

  ze_ipc_mem_handle_t ipc_handle;
  lzt::get_ipc_handle(context, &ipc_handle, memory);
  lzt::send_ipc_handle(ipc_handle);

  if (concurrency_offset > 0) {
    void *buffer = lzt::allocate_device_memory(size - concurrency_offset, 1, 0,
                                               device, context);
    lzt::append_memory_fill(cl, buffer, &second_pattern, sizeof(second_pattern),
                            size - concurrency_offset, nullptr);
    lzt::append_barrier(cl, nullptr, 0, nullptr);

    lzt::append_memory_copy(cl, ((char *)memory) + concurrency_offset, buffer,
                            size - concurrency_offset);
    lzt::close_command_list(cl);
    lzt::execute_command_lists(cq, 1, &cl, nullptr);
    lzt::synchronize(cq, UINT64_MAX);
    lzt::reset_command_list(cl);
    lzt::free_memory(buffer);
  }

  sigset_t wset;
  sigemptyset(&wset);
  sigaddset(&wset, SIGINT);
  int sig;
  sigwait(&wset, &sig);

  void *buffer;
  buffer = malloc(size * sizeof(char));
  memset(buffer, 0, size);

  lzt::append_memory_copy(cl, buffer, memory, size);
  lzt::close_command_list(cl);
  lzt::execute_command_lists(cq, 1, &cl, nullptr);
  lzt::synchronize(cq, UINT64_MAX);
  lzt::reset_command_list(cl);

  LOG_DEBUG << "[Server] Validating buffer received correctly";
  char *expected_buffer = new char[size];
  fill_expected_buffer(expected_buffer, size, concurrency_offset);

  bool matched_expected_buffer = false;
  if (0 == memcmp(expected_buffer, buffer, size)) {
    matched_expected_buffer = true;
  }

  if (bidirectional) {
    void *bi_memory = nullptr;
    ze_ipc_mem_handle_t bi_ipc_handle;

    int ipc_descriptor = lzt::receive_ipc_handle();
    memcpy(&bi_ipc_handle, static_cast<void *>(&ipc_descriptor),
           sizeof(ipc_descriptor));

    lzt::open_ipc_handle(context, device, bi_ipc_handle, &bi_memory);
    void *bi_buffer = lzt::allocate_device_memory(size - concurrency_offset, 1,
                                                  0, device, context);
    lzt::append_memory_fill(cl, bi_buffer, &first_pattern,
                            sizeof(first_pattern), size - concurrency_offset,
                            nullptr);
    lzt::append_barrier(cl, nullptr, 0, nullptr);

    lzt::append_memory_copy(cl, bi_memory, bi_buffer,
                            size - concurrency_offset);
    lzt::close_command_list(cl);
    lzt::execute_command_lists(cq, 1, &cl, nullptr);
    lzt::synchronize(cq, UINT64_MAX);

    EXPECT_EQ(ZE_RESULT_SUCCESS, zeMemCloseIpcHandle(context, bi_memory));

    lzt::free_memory(bi_buffer);
    kill(pid, SIGINT);
  }

  int child_status;
  pid_t client_pid = wait(&child_status);

  free(buffer);
  lzt::free_memory(memory);
  lzt::destroy_command_list(cl);
  lzt::destroy_command_queue(cq);
  lzt::destroy_context(context);

  if (client_pid <= 0) {
    LOG_ERROR << "Client terminated abruptly with error code "
              << strerror(errno) << "\n";
    std::terminate();
  }
  if (WIFEXITED(child_status) == false) {
    std::terminate();
  }

  EXPECT_EQ(true, matched_expected_buffer);
  if (matched_expected_buffer == false) {
    std::terminate();
  }

  std::exit(0);
}

void run_client(int size, int concurrency_offset, bool bidirectional,
                pid_t ppid) {
  ze_context_handle_t context;
  ze_device_handle_t device;
  ze_command_queue_handle_t cq;
  ze_command_list_handle_t cl;
  bool matched_expected_buffer = true;

  init_process(context, device, cq, cl, false, concurrency_offset);

  void *memory = nullptr;
  ze_ipc_mem_handle_t ipc_handle;

  int ipc_descriptor = lzt::receive_ipc_handle();
  memcpy(&ipc_handle, static_cast<void *>(&ipc_descriptor),
         sizeof(ipc_descriptor));

  lzt::open_ipc_handle(context, device, ipc_handle, &memory);
  void *buffer = lzt::allocate_device_memory(size - concurrency_offset, 1, 0,
                                             device, context);
  lzt::append_memory_fill(cl, buffer, &first_pattern, sizeof(first_pattern),
                          size - concurrency_offset, nullptr);
  lzt::append_barrier(cl, nullptr, 0, nullptr);

  lzt::append_memory_copy(cl, memory, buffer, size - concurrency_offset);
  lzt::close_command_list(cl);
  lzt::execute_command_lists(cq, 1, &cl, nullptr);
  lzt::synchronize(cq, UINT64_MAX);
  lzt::reset_command_list(cl);

  zeMemCloseIpcHandle(context, memory);

  kill(ppid, SIGINT);

  if (bidirectional) {
    void *bi_memory = lzt::allocate_device_memory(size, 1, 0, device, context);
    lzt::append_memory_fill(cl, bi_memory, &initial_pattern,
                            sizeof(initial_pattern), size, nullptr);

    lzt::close_command_list(cl);
    lzt::execute_command_lists(cq, 1, &cl, nullptr);
    lzt::synchronize(cq, UINT64_MAX);
    lzt::reset_command_list(cl);

    ze_ipc_mem_handle_t bi_ipc_handle;
    lzt::get_ipc_handle(context, &bi_ipc_handle, bi_memory);
    lzt::send_ipc_handle(bi_ipc_handle);

    sigset_t wset;
    sigemptyset(&wset);
    sigaddset(&wset, SIGINT);
    int sig;
    sigwait(&wset, &sig);

    void *bi_buffer;
    bi_buffer = malloc(size * sizeof(char));
    memset(bi_buffer, 0, size);

    lzt::append_memory_copy(cl, bi_buffer, bi_memory, size);
    lzt::close_command_list(cl);
    lzt::execute_command_lists(cq, 1, &cl, nullptr);
    lzt::synchronize(cq, UINT64_MAX);
    lzt::reset_command_list(cl);

    LOG_DEBUG << "[Client] Validating buffer received correctly";
    char *expected_buffer = new char[size];
    fill_expected_buffer(expected_buffer, size, concurrency_offset);

    if (0 == memcmp(expected_buffer, bi_buffer, size)) {
      matched_expected_buffer = true;
    } else {
      matched_expected_buffer = false;
    }

    free(bi_buffer);
  }

  lzt::destroy_command_list(cl);
  lzt::destroy_command_queue(cq);
  lzt::free_memory(buffer);
  lzt::destroy_context(context);

  EXPECT_EQ(true, matched_expected_buffer);
  if (matched_expected_buffer == false) {
    std::terminate();
  }
  std::exit(0);
}

class P2PIpcMemoryAccessTest : public ::testing::Test,
                               public ::testing::WithParamInterface<size_t> {};

TEST_P(
    P2PIpcMemoryAccessTest,
    GivenL0MemoryAllocatedInDevice0ExportedToDevice1WhenUsingL0IPCP2PThenExportedMemoryAllocationUpdatedByDevice1Correctly) {
  size_t size = pow(2, GetParam());
  LOG_INFO << "Buffer Size: " << size;

  pid_t pid = fork();
  if (pid < 0) {
    throw std::runtime_error("Failed to fork child process");
  } else if (pid > 0) {
    int child_status;
    pid_t client_pid = wait(&child_status);
    if (client_pid <= 0) {
      std::cerr << "Client terminated abruptly with error code "
                << strerror(errno) << "\n";
      std::terminate();
    }
    EXPECT_EQ(true, WIFEXITED(child_status));
  } else {
    pid_t ppid = getpid();
    pid_t pid = fork();
    if (pid < 0) {
      throw std::runtime_error("Failed to fork child process");
    } else if (pid > 0) {
      run_server(size, 0, false, pid);
    } else {
      run_client(size, 0, false, ppid);
    }
  }
}

TEST_P(
    P2PIpcMemoryAccessTest,
    GivenL0MemoryAllocatedInDevice0AndDevice1ExportedToEachOtherSequentiallyWhenUsingL0IPCP2PThenExportedMemoryAllocationUpdatedByDevice0AndDevice1Sequentially) {
  size_t size = pow(2, GetParam());
  LOG_INFO << "Buffer Size: " << size;

  pid_t pid = fork();
  if (pid < 0) {
    throw std::runtime_error("Failed to fork child process");
  } else if (pid > 0) {
    int child_status;
    pid_t client_pid = wait(&child_status);
    if (client_pid <= 0) {
      std::cerr << "Client terminated abruptly with error code "
                << strerror(errno) << "\n";
      std::terminate();
    }
    EXPECT_EQ(true, WIFEXITED(child_status));
  } else {
    pid_t ppid = getpid();
    pid_t pid = fork();
    if (pid < 0) {
      throw std::runtime_error("Failed to fork child process");
    } else if (pid > 0) {
      run_server(size, 0, true, pid);
    } else {
      run_client(size, 0, true, ppid);
    }
  }
}

INSTANTIATE_TEST_CASE_P(AllocationSize, P2PIpcMemoryAccessTest,
                        ::testing::Values(3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
                                          14, 15, 16, 17, 18, 19, 20, 21, 22,
                                          23, 24, 25, 26, 27, 28));

class P2PConcurrencyIpcMemoryAccessTest
    : public ::testing::Test,
      public ::testing::WithParamInterface<size_t> {};

TEST_P(
    P2PConcurrencyIpcMemoryAccessTest,
    GivenL0MemoryAllocatedInDevice0ExportedToDevice1WhenUsingL0IPCP2PThenExportedMemoryAllocationUpdatedByBothDevice0AndDevice1Concurrently) {
  size_t size = pow(2, GetParam());
  size_t concurrency_offset = pow(2, (GetParam() - 1));
  LOG_INFO << "Buffer Size: " << size
           << " Concurrency Offset: " << concurrency_offset;

  pid_t pid = fork();
  if (pid < 0) {
    throw std::runtime_error("Failed to fork child process");
  } else if (pid > 0) {
    int child_status;
    pid_t client_pid = wait(&child_status);
    if (client_pid <= 0) {
      std::cerr << "Client terminated abruptly with error code "
                << strerror(errno) << "\n";
      std::terminate();
    }
    EXPECT_EQ(true, WIFEXITED(child_status));
  } else {
    pid_t ppid = getpid();
    pid_t pid = fork();
    if (pid < 0) {
      throw std::runtime_error("Failed to fork child process");
    } else if (pid > 0) {
      run_server(size, concurrency_offset, false, pid);
    } else {
      run_client(size, concurrency_offset, false, ppid);
    }
  }
}

INSTANTIATE_TEST_CASE_P(AllocationSize, P2PConcurrencyIpcMemoryAccessTest,
                        ::testing::Values(3, 4, 5, 6, 7, 8, 9, 10, 11, 12));

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