/*
 *
 * Copyright (C) 2021-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"
#include "net/test_ipc_comm.hpp"

#include <cmath>
#include <signal.h>
#include <level_zero/ze_api.h>

namespace {
#ifdef __linux__

const uint8_t client_pattern = 1;
const uint8_t server_pattern = 2;
int signum_caught = -1;

bool check_p2p_capabilities(std::vector<ze_device_handle_t> devices) {
  ze_bool_t can_access_peer = false;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeDeviceCanAccessPeer(devices[0], devices[1], &can_access_peer));
  ze_device_p2p_properties_t p2p_properties{};
  p2p_properties.stype = ZE_STRUCTURE_TYPE_DEVICE_P2P_PROPERTIES;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeDeviceGetP2PProperties(devices[0], devices[1], &p2p_properties));

  if ((can_access_peer == false) || (p2p_properties.flags == 0)) {
    LOG_WARNING << "Two devices found but no P2P capabilities detected\n";
    return false;
  } else {
    LOG_DEBUG << "Two devices found and P2P capabilities detected\n";
    return true;
  }
}

void sigint_handler(int signal_number) { signum_caught = signal_number; }

void init_driver() {
  ze_result_t result = zeInit(0);
  if (result != ZE_RESULT_SUCCESS) {
    throw std::runtime_error("zeInit failed: " +
                             level_zero_tests::to_string(result));
  }
  LOG_DEBUG << "Driver initialized\n";
}

void fill_expected_buffer(char *expected_buffer, const int size,
                          const int concurrency_offset, const bool is_server) {
  if (concurrency_offset > 0) {
    for (size_t i = 0; i < size - concurrency_offset; ++i) {
      if (is_server) {
        expected_buffer[i] = client_pattern;
      } else {
        expected_buffer[i] = server_pattern;
      }
    }
    for (size_t i = concurrency_offset; i < size; ++i) {
      if (is_server) {
        expected_buffer[i] = server_pattern;
      } else {
        expected_buffer[i] = client_pattern;
      }
    }
  } else {
    for (size_t i = 0; i < size; ++i) {
      if (is_server) {
        expected_buffer[i] = client_pattern;
      } else {
        expected_buffer[i] = server_pattern;
      }
    }
  }
}

inline void init_process(ze_context_handle_t &context,
                         ze_device_handle_t &device,
                         std::vector<lzt::zeCommandBundle> &cmd_bundles,
                         bool is_server, uint32_t device_x, uint32_t device_y,
                         int concurrency_offset, bool is_immediate) {
  init_driver();

  auto driver = lzt::get_default_driver();
  context = lzt::create_context(driver);

  auto devices = lzt::get_devices(driver);

  if (devices.size() > 1) {
    if (check_p2p_capabilities(devices) == true) {
      if (is_server) {
        device = devices[device_x];
      } else {
        device = devices[device_y];
      }
    } else {
      std::exit(0);
    }
  } else {
    LOG_WARNING << "Only one device found, requires two for P2P test\n";
    std::exit(0);
  }

  if (concurrency_offset > 0) {
    ze_device_p2p_properties_t peerProperties = {};
    peerProperties.stype = ZE_STRUCTURE_TYPE_DEVICE_P2P_PROPERTIES;

    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeDeviceGetP2PProperties(devices[device_x], devices[device_y],
                                       &peerProperties));
    if ((peerProperties.flags & ZE_DEVICE_P2P_PROPERTY_FLAG_ATOMICS) == 0) {
      LOG_WARNING << "P2P atomic access unsupported, required for concurrent "
                     "access tests\n";
      std::exit(0);
    }
  }

  uint32_t num_queue_groups =
      lzt::get_command_queue_group_properties_count(device);
  std::vector<ze_command_queue_group_properties_t> queue_properties(
      num_queue_groups);
  queue_properties =
      lzt::get_command_queue_group_properties(device, num_queue_groups);

  std::vector<uint32_t> ordinals;
  for (uint32_t i = 0; i < num_queue_groups; i++) {
    if ((queue_properties[i].flags &
         ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE) == 0 ||
        (queue_properties[i].flags &
         ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COPY)) {
      ordinals.push_back(i);
    }
  }

  for (uint32_t ordinal : ordinals) {
    ze_command_queue_desc_t cmdQueueDesc = {};
    cmdQueueDesc.stype = ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC;
    cmdQueueDesc.ordinal = ordinal;
    cmdQueueDesc.mode = ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS;

    for (uint32_t i = 0; i < queue_properties[ordinal].numQueues; i++) {
      auto cmd_bundle = lzt::create_command_bundle(
          context, device, ZE_COMMAND_QUEUE_FLAG_EXPLICIT_ONLY,
          ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS, ZE_COMMAND_QUEUE_PRIORITY_NORMAL,
          ZE_COMMAND_LIST_FLAG_EXPLICIT_ONLY, ordinal, i, is_immediate);
      cmd_bundles.push_back(cmd_bundle);
    }
  }
}

void run_server(int size, uint32_t device_x, uint32_t device_y,
                int concurrency_offset, bool bidirectional, pid_t pid,
                bool is_immediate) {
  ze_context_handle_t context;
  ze_device_handle_t device;
  std::vector<lzt::zeCommandBundle> cmd_bundles;
  bool matched_expected_buffer = true;

  init_process(context, device, cmd_bundles, true, device_x, device_y,
               concurrency_offset, is_immediate);

  for (uint32_t i = 0; i < cmd_bundles.size(); i++) {
    auto cb = cmd_bundles[i];

    void *memory = lzt::allocate_device_memory(size, 1, 0, device, context);
    lzt::append_memory_fill(cb.list, memory, &server_pattern,
                            sizeof(server_pattern), size, nullptr);
    lzt::close_command_list(cb.list);
    lzt::execute_and_sync_command_bundle(cb, UINT64_MAX);
    lzt::reset_command_list(cb.list);

    struct sigaction sigint_action {};
    sigint_action.sa_handler = sigint_handler;
    sigint_action.sa_flags = 0;
    sigint_action.sa_restorer = nullptr;
    EXPECT_EQ(0, sigemptyset(&sigint_action.sa_mask));
    EXPECT_EQ(0, sigaction(SIGINT, &sigint_action, nullptr));

    ze_ipc_mem_handle_t ipc_handle;
    lzt::get_ipc_handle(context, &ipc_handle, memory);
    lzt::send_ipc_handle(ipc_handle);

    if (concurrency_offset > 0) {
      void *buffer = lzt::allocate_device_memory(size - concurrency_offset, 1,
                                                 0, device, context);
      lzt::append_memory_fill(cb.list, buffer, &server_pattern,
                              sizeof(server_pattern), size - concurrency_offset,
                              nullptr);
      lzt::append_barrier(cb.list, nullptr, 0, nullptr);

      lzt::append_memory_copy(cb.list, ((char *)memory) + concurrency_offset,
                              buffer, size - concurrency_offset);
      lzt::close_command_list(cb.list);
      lzt::execute_and_sync_command_bundle(cb, UINT64_MAX);
      lzt::reset_command_list(cb.list);
      lzt::free_memory(context, buffer);
    }

    sigset_t sigint_mask{};
    EXPECT_EQ(0, sigfillset(&sigint_mask));
    EXPECT_EQ(0, sigdelset(&sigint_mask, SIGINT));
    EXPECT_EQ(-1, sigsuspend(&sigint_mask));
    EXPECT_EQ(SIGINT, signum_caught);

    void *buffer;
    buffer = malloc(size * sizeof(char));
    memset(buffer, 0, size);

    lzt::append_memory_copy(cb.list, buffer, memory, size);
    lzt::close_command_list(cb.list);
    lzt::execute_and_sync_command_bundle(cb, UINT64_MAX);
    lzt::reset_command_list(cb.list);

    LOG_INFO << "[Server] Validating buffer received correctly";
    char *expected_buffer = new char[size];
    fill_expected_buffer(expected_buffer, size, concurrency_offset,
                         true /* Server */);

    if (0 != memcmp(expected_buffer, buffer, size)) {
      matched_expected_buffer = false;
    }

    delete[] expected_buffer;
    free(buffer);
    lzt::free_memory(context, memory);
  }

  for (auto bundle : cmd_bundles) {
    lzt::destroy_command_bundle(bundle);
  }
  lzt::destroy_context(context);

  int child_status;
  pid_t client_pid = wait(&child_status);

  if (client_pid <= 0) {
    FAIL() << "Client terminated abruptly with error code " << strerror(errno)
           << "\n";
  }
  if (WIFEXITED(child_status)) {
    if (WEXITSTATUS(child_status) != 0) {
      FAIL() << "Client process failed\n";
    }
  } else {
    FAIL() << "Client process exited abnormally\n";
  }
  EXPECT_TRUE(matched_expected_buffer);

  if (testing::Test::HasFailure()) {
    std::exit(1);
  }
  std::exit(0);
}

void run_client(int size, uint32_t device_x, uint32_t device_y,
                int concurrency_offset, bool bidirectional, pid_t ppid,
                bool is_immediate) {
  ze_context_handle_t context;
  ze_device_handle_t device;
  std::vector<lzt::zeCommandBundle> cmd_bundles;
  bool matched_expected_buffer = true;

  init_process(context, device, cmd_bundles, false, device_x, device_y,
               concurrency_offset, is_immediate);

  for (uint32_t i = 0; i < cmd_bundles.size(); i++) {
    auto cb = cmd_bundles[i];

    void *memory = nullptr;
    ze_ipc_mem_handle_t ipc_handle;

    int ipc_descriptor =
        lzt::receive_ipc_handle<ze_ipc_mem_handle_t>(ipc_handle.data);
    memcpy(&ipc_handle, static_cast<void *>(&ipc_descriptor),
           sizeof(ipc_descriptor));
    lzt::open_ipc_handle(context, device, ipc_handle, &memory);

    void *buffer = lzt::allocate_device_memory(size - concurrency_offset, 1, 0,
                                               device, context);
    lzt::append_memory_fill(cb.list, buffer, &client_pattern,
                            sizeof(client_pattern), size - concurrency_offset,
                            nullptr);

    void *bi_buffer = lzt::allocate_device_memory(size, 1, 0, device, context);
    lzt::append_memory_fill(cb.list, bi_buffer, &client_pattern,
                            sizeof(client_pattern), size, nullptr);
    lzt::append_barrier(cb.list, nullptr, 0, nullptr);

    if (bidirectional) {
      lzt::append_memory_copy(cb.list, bi_buffer, memory,
                              size - concurrency_offset);
      lzt::append_barrier(cb.list, nullptr, 0, nullptr);
    }

    lzt::append_memory_copy(cb.list, memory, buffer, size - concurrency_offset);
    lzt::close_command_list(cb.list);
    lzt::execute_and_sync_command_bundle(cb, UINT64_MAX);
    lzt::reset_command_list(cb.list);

    EXPECT_EQ(ZE_RESULT_SUCCESS, zeMemCloseIpcHandle(context, memory));

    usleep(5e5);
    EXPECT_EQ(0, kill(ppid, SIGINT));

    if (bidirectional) {
      void *bi_host_buffer;
      bi_host_buffer = malloc(size * sizeof(char));
      memset(bi_host_buffer, 0, size);

      lzt::append_memory_copy(cb.list, bi_host_buffer, bi_buffer, size);
      lzt::close_command_list(cb.list);
      lzt::execute_and_sync_command_bundle(cb, UINT64_MAX);
      lzt::reset_command_list(cb.list);

      LOG_DEBUG << "[Client] Validating buffer received correctly";
      char *expected_buffer = new char[size];
      fill_expected_buffer(expected_buffer, size, concurrency_offset,
                           false /* Client */);

      if (0 != memcmp(expected_buffer, bi_host_buffer, size)) {
        matched_expected_buffer = false;
      }
      delete[] expected_buffer;

      free(bi_host_buffer);
    }

    lzt::free_memory(context, bi_buffer);
    lzt::free_memory(context, buffer);
  }

  for (auto bundle : cmd_bundles) {
    lzt::destroy_command_bundle(bundle);
  }
  lzt::destroy_context(context);

  EXPECT_TRUE(matched_expected_buffer);
  if (testing::Test::HasFailure()) {
    std::exit(1);
  }
  std::exit(0);
}

class P2PIpcMemoryAccessTest
    : public ::testing::Test,
      public ::testing::WithParamInterface<
          std::tuple<size_t, uint32_t, uint32_t, bool>> {};

TEST_P(
    P2PIpcMemoryAccessTest,
    GivenTwoDevicesAndL0MemoryAllocatedInDeviceXExportedToDeviceYWhenUsingL0IPCP2PThenExportedMemoryAllocationUpdatedByDeviceYCorrectly) {
  size_t size = std::pow(2, std::get<0>(GetParam()));
  uint32_t device_x = std::get<1>(GetParam());
  uint32_t device_y = std::get<2>(GetParam());
  bool is_immediate = std::get<3>(GetParam());
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
    EXPECT_EQ(0, WEXITSTATUS(child_status));
  } else {
    pid_t ppid = getpid();
    pid_t pid = fork();
    if (pid < 0) {
      throw std::runtime_error("Failed to fork child process");
    } else if (pid > 0) {
      run_server(size, device_x, device_y, 0, false, pid, is_immediate);
    } else {
      run_client(size, device_x, device_y, 0, false, ppid, is_immediate);
    }
  }
}

TEST_P(
    P2PIpcMemoryAccessTest,
    GivenTwoDevicesAndL0MemoryAllocatedInDeviceXAndDeviceYExportedToEachOtherSequentiallyWhenUsingL0IPCP2PThenExportedMemoryAllocationUpdatedByDeviceXAndDeviceYSequentially) {
  size_t size = std::pow(2, std::get<0>(GetParam()));
  uint32_t device_x = std::get<1>(GetParam());
  uint32_t device_y = std::get<2>(GetParam());
  bool is_immediate = std::get<3>(GetParam());
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
    EXPECT_EQ(0, WEXITSTATUS(child_status));
  } else {
    pid_t ppid = getpid();
    pid_t pid = fork();
    if (pid < 0) {
      throw std::runtime_error("Failed to fork child process");
    } else if (pid > 0) {
      run_server(size, device_x, device_y, 0, true, pid, is_immediate);
    } else {
      run_client(size, device_x, device_y, 0, true, ppid, is_immediate);
    }
  }
}

TEST_P(
    P2PIpcMemoryAccessTest,
    GivenTwoDevicesAndL0MemoryAllocatedInDeviceXExportedToDeviceYWhenUsingL0IPCP2PThenExportedMemoryAllocationUpdatedByBothDeviceXAndDeviceYConcurrently) {
  size_t size = std::pow(2, std::get<0>(GetParam()));
  size_t concurrency_offset = std::pow(2, std::get<0>(GetParam()) - 1);
  uint32_t device_x = std::get<1>(GetParam());
  uint32_t device_y = std::get<2>(GetParam());
  bool is_immediate = std::get<3>(GetParam());
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
    EXPECT_EQ(0, WEXITSTATUS(child_status));
  } else {
    pid_t ppid = getpid();
    pid_t pid = fork();
    if (pid < 0) {
      throw std::runtime_error("Failed to fork child process");
    } else if (pid > 0) {
      run_server(size, device_x, device_y, concurrency_offset, false, pid,
                 is_immediate);
    } else {
      run_client(size, device_x, device_y, concurrency_offset, false, ppid,
                 is_immediate);
    }
  }
}

INSTANTIATE_TEST_SUITE_P(
    AllocationSize, P2PIpcMemoryAccessTest,
    ::testing::Combine(::testing::Values(3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
                                         14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
                                         24, 25, 26, 27, 28), // Buffer Size
                       ::testing::Values(0, 1), // Server device ID
                       ::testing::Values(0, 1), // Client device ID
                       ::testing::Bool()));

#endif

} // namespace

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
