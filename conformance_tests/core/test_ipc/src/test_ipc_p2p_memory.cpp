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

uint8_t client_pattern = 1;
uint8_t server_pattern = 2;

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
                          int concurrency_offset, bool is_server) {
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
                         std::vector<ze_command_queue_handle_t> &queues,
                         std::vector<ze_command_list_handle_t> &lists,
                         bool is_server, uint32_t device_x, uint32_t device_y,
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
    ze_device_p2p_properties_t peerProperties;

    zeDeviceGetP2PProperties(devices[device_x], devices[device_y],
                             &peerProperties);
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
    cmdQueueDesc.ordinal = ordinal;
    cmdQueueDesc.mode = ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS;

    for (uint32_t i = 0; i < queue_properties[ordinal].numQueues; i++) {
      cmdQueueDesc.index = i;
      ze_command_queue_handle_t queue;
      zeCommandQueueCreate(context, device, &cmdQueueDesc, &queue);
      queues.push_back(queue);

      ze_command_list_desc_t listDesc = {};
      listDesc.commandQueueGroupOrdinal = ordinal;
      ze_command_list_handle_t list;
      zeCommandListCreate(context, device, &listDesc, &list);
      lists.push_back(list);
    }
  }

  signal(SIGINT, sigint_handler);

  return;
}

void run_server(int size, uint32_t device_x, uint32_t device_y,
                int concurrency_offset, bool bidirectional, pid_t pid) {
  ze_context_handle_t context;
  ze_device_handle_t device;
  std::vector<ze_command_queue_handle_t> queues;
  std::vector<ze_command_list_handle_t> lists;
  bool matched_expected_buffer = true;

  init_process(context, device, queues, lists, true, device_x, device_y,
               concurrency_offset);

  for (uint32_t i = 0; i < queues.size(); i++) {
    auto cq = queues[i];
    auto cl = lists[i];

    void *memory = lzt::allocate_device_memory(size, 1, 0, device, context);
    lzt::append_memory_fill(cl, memory, &server_pattern, sizeof(server_pattern),
                            size, nullptr);
    lzt::close_command_list(cl);
    lzt::execute_command_lists(cq, 1, &cl, nullptr);
    lzt::synchronize(cq, UINT64_MAX);
    lzt::reset_command_list(cl);

    ze_ipc_mem_handle_t ipc_handle;
    lzt::get_ipc_handle(context, &ipc_handle, memory);
    lzt::send_ipc_handle(ipc_handle);

    if (concurrency_offset > 0) {
      void *buffer = lzt::allocate_device_memory(size - concurrency_offset, 1,
                                                 0, device, context);
      lzt::append_memory_fill(cl, buffer, &server_pattern,
                              sizeof(server_pattern), size - concurrency_offset,
                              nullptr);
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

    LOG_INFO << "[Server] Validating buffer received correctly";
    char *expected_buffer = new char[size];
    fill_expected_buffer(expected_buffer, size, concurrency_offset,
                         true /* Server */);

    if (0 != memcmp(expected_buffer, buffer, size)) {
      matched_expected_buffer = false;
    }

    free(buffer);
    lzt::free_memory(memory);
  }

  for (auto queue : queues) {
    lzt::destroy_command_queue(queue);
  }
  for (auto list : lists) {
    lzt::destroy_command_list(list);
  }
  lzt::destroy_context(context);

  int child_status;
  pid_t client_pid = wait(&child_status);

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

void run_client(int size, uint32_t device_x, uint32_t device_y,
                int concurrency_offset, bool bidirectional, pid_t ppid) {
  ze_context_handle_t context;
  ze_device_handle_t device;
  std::vector<ze_command_queue_handle_t> queues;
  std::vector<ze_command_list_handle_t> lists;
  bool matched_expected_buffer = true;

  init_process(context, device, queues, lists, false, device_x, device_y,
               concurrency_offset);

  for (uint32_t i = 0; i < queues.size(); i++) {
    auto cq = queues[i];
    auto cl = lists[i];

    void *memory = nullptr;
    ze_ipc_mem_handle_t ipc_handle;

    int ipc_descriptor = lzt::receive_ipc_handle();
    memcpy(&ipc_handle, static_cast<void *>(&ipc_descriptor),
           sizeof(ipc_descriptor));
    lzt::open_ipc_handle(context, device, ipc_handle, &memory);

    void *buffer = lzt::allocate_device_memory(size - concurrency_offset, 1, 0,
                                               device, context);
    lzt::append_memory_fill(cl, buffer, &client_pattern, sizeof(client_pattern),
                            size - concurrency_offset, nullptr);

    void *bi_buffer = lzt::allocate_device_memory(size, 1, 0, device, context);
    lzt::append_memory_fill(cl, bi_buffer, &client_pattern,
                            sizeof(client_pattern), size, nullptr);
    lzt::append_barrier(cl, nullptr, 0, nullptr);

    if (bidirectional) {
      lzt::append_memory_copy(cl, bi_buffer, memory, size - concurrency_offset);
      lzt::append_barrier(cl, nullptr, 0, nullptr);
    }

    lzt::append_memory_copy(cl, memory, buffer, size - concurrency_offset);
    lzt::close_command_list(cl);
    lzt::execute_command_lists(cq, 1, &cl, nullptr);
    lzt::synchronize(cq, UINT64_MAX);
    lzt::reset_command_list(cl);

    zeMemCloseIpcHandle(context, memory);

    kill(ppid, SIGINT);

    if (bidirectional) {
      void *bi_host_buffer;
      bi_host_buffer = malloc(size * sizeof(char));
      memset(bi_host_buffer, 0, size);

      lzt::append_memory_copy(cl, bi_host_buffer, bi_buffer, size);
      lzt::close_command_list(cl);
      lzt::execute_command_lists(cq, 1, &cl, nullptr);
      lzt::synchronize(cq, UINT64_MAX);
      lzt::reset_command_list(cl);

      LOG_DEBUG << "[Client] Validating buffer received correctly";
      char *expected_buffer = new char[size];
      fill_expected_buffer(expected_buffer, size, concurrency_offset,
                           false /* Client */);

      if (0 != memcmp(expected_buffer, bi_host_buffer, size)) {
        matched_expected_buffer = false;
      }

      free(bi_host_buffer);
    }

    lzt::free_memory(bi_buffer);
    lzt::free_memory(buffer);
  }

  for (auto queue : queues) {
    lzt::destroy_command_queue(queue);
  }
  for (auto list : lists) {
    lzt::destroy_command_list(list);
  }
  lzt::destroy_context(context);

  EXPECT_EQ(true, matched_expected_buffer);
  if (matched_expected_buffer == false) {
    std::terminate();
  }
  std::exit(0);
}

class P2PIpcMemoryAccessTest : public ::testing::Test,
                               public ::testing::WithParamInterface<
                                   std::tuple<size_t, uint32_t, uint32_t>> {};

TEST_P(
    P2PIpcMemoryAccessTest,
    GivenL0MemoryAllocatedInDeviceXExportedToDeviceYWhenUsingL0IPCP2PThenExportedMemoryAllocationUpdatedByDeviceYCorrectly) {
  size_t size = pow(2, std::get<0>(GetParam()));
  uint32_t device_x = std::get<1>(GetParam());
  uint32_t device_y = std::get<2>(GetParam());
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
      run_server(size, device_x, device_y, 0, false, pid);
    } else {
      run_client(size, device_x, device_y, 0, false, ppid);
    }
  }
}

TEST_P(
    P2PIpcMemoryAccessTest,
    GivenL0MemoryAllocatedInDeviceXAndDeviceYExportedToEachOtherSequentiallyWhenUsingL0IPCP2PThenExportedMemoryAllocationUpdatedByDeviceXAndDeviceYSequentially) {
  size_t size = pow(2, std::get<0>(GetParam()));
  uint32_t device_x = std::get<1>(GetParam());
  uint32_t device_y = std::get<2>(GetParam());
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
      run_server(size, device_x, device_y, 0, true, pid);
    } else {
      run_client(size, device_x, device_y, 0, true, ppid);
    }
  }
}

TEST_P(
    P2PIpcMemoryAccessTest,
    GivenL0MemoryAllocatedInDeviceXExportedToDeviceYWhenUsingL0IPCP2PThenExportedMemoryAllocationUpdatedByBothDeviceXAndDeviceYConcurrently) {
  size_t size = pow(2, std::get<0>(GetParam()));
  size_t concurrency_offset = pow(2, std::get<0>(GetParam()) - 1);
  uint32_t device_x = std::get<1>(GetParam());
  uint32_t device_y = std::get<2>(GetParam());
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
      run_server(size, device_x, device_y, concurrency_offset, false, pid);
    } else {
      run_client(size, device_x, device_y, concurrency_offset, false, ppid);
    }
  }
}

INSTANTIATE_TEST_CASE_P(
    AllocationSize, P2PIpcMemoryAccessTest,
    ::testing::Combine(::testing::Values(3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
                                         14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
                                         24, 25, 26, 27, 28), // Buffer Size
                       ::testing::Values(0, 1), // Server device ID
                       ::testing::Values(0, 1)  // Client device ID
                       ));

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