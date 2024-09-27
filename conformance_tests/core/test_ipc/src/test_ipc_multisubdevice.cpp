/*
 *
 * Copyright (C) 2020-2023 Intel Corporation
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

#include <level_zero/ze_api.h>

namespace {
#ifdef __linux__

void multi_sub_device_sender(size_t size, bool reserved, bool is_immediate) {
  ze_result_t result = zeInit(0);
  if (result != ZE_RESULT_SUCCESS) {
    throw std::runtime_error("Sender zeInit failed: " +
                             level_zero_tests::to_string(result));
  }

  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);

  // Devices are not guaranteed to be reported in the same order, so
  // we need to check that we use subdevices on the same device
  ze_device_handle_t test_device = nullptr;
  ze_device_properties_t test_device_properties = {};
  for (auto dev : devices) {
    auto temp_devices = lzt::get_ze_sub_devices(dev);

    if (temp_devices.size() > 1) {
      if (!test_device) {
        test_device = dev;
        test_device_properties = lzt::get_device_properties(dev);
      } else {
        // compare the uuid of this device to the current test_device
        auto dev_properties = lzt::get_device_properties(dev);
        auto select = memcmp(&dev_properties.uuid, &test_device_properties.uuid,
                             ZE_MAX_DEVICE_UUID_SIZE);

        if (select < 0) {
          test_device = dev;
          test_device_properties = dev_properties;
        }
      }
    }
  }

  if (!test_device) {
    LOG_WARNING << "Less than 2 sub-devices, skipping test\n";
    exit(0);
  }
  devices = lzt::get_ze_sub_devices(test_device);

  auto device_0 = devices[0];
  auto device_1 = devices[1];
  auto dev_properties_0 = lzt::get_device_properties(device_0);
  auto dev_properties_1 = lzt::get_device_properties(device_1);

  ze_device_handle_t device = device_0;
  if (dev_properties_0.subdeviceId < dev_properties_1.subdeviceId) {
    device = device_1;
    LOG_DEBUG << "Sender Selected subdevice 1" << std::endl;
  } else if (dev_properties_0.subdeviceId > dev_properties_1.subdeviceId) {
    device = device_0;
    LOG_DEBUG << "Sender Selected subdevice 0" << std::endl;
  } else {
    LOG_WARNING << "Devices have the same subdeviceId" << std::endl;
  }

  auto context = lzt::create_context(driver);
  auto cmd_bundle = lzt::create_command_bundle(
      context, device, 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0, 0, 0, is_immediate);

  ze_device_mem_alloc_flags_t flags = 0;
  size_t allocSize = size;
  ze_physical_mem_handle_t reservedPhysicalMemory = {};
  void *memory = nullptr;
  if (reserved) {
    memory = lzt::reserve_allocate_and_map_memory(context, device, allocSize,
                                                  &reservedPhysicalMemory);
  } else {
    memory = lzt::allocate_device_memory(size, 1, flags, device, context);
  }

  void *buffer = lzt::allocate_host_memory(size, 1, context);
  lzt::write_data_pattern(buffer, size, 1);
  lzt::append_memory_copy(cmd_bundle.list, memory, buffer, size);

  lzt::close_command_list(cmd_bundle.list);
  lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

  ze_ipc_mem_handle_t ipc_handle{};
  std::fill_n(ipc_handle.data, ZE_MAX_IPC_HANDLE_SIZE, 0);
  lzt::get_ipc_handle(context, &ipc_handle, memory);
  lzt::send_ipc_handle(ipc_handle);

  // Free device memory once receiver is done
  int child_status;
  pid_t client_pid = wait(&child_status);
  if (client_pid <= 0) {
    FAIL() << "Error waiting for receiver process " << strerror(errno) << "\n";
  }

  if (WIFEXITED(child_status)) {
    if (WEXITSTATUS(child_status) != 0) {
      FAIL() << "Receiver process failed memory verification\n";
    }
  } else {
    FAIL() << "Receiver process exited abnormally\n";
  }

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

void multi_sub_device_receiver(size_t size, bool is_immediate) {
  ze_result_t result = zeInit(0);
  if (result != ZE_RESULT_SUCCESS) {
    throw std::runtime_error("Receiver zeInit failed: " +
                             level_zero_tests::to_string(result));
  }

  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);

  ze_device_handle_t test_device = nullptr;
  ze_device_properties_t test_device_properties = {};
  for (auto dev : devices) {
    auto temp_devices = lzt::get_ze_sub_devices(dev);

    if (temp_devices.size() > 1) {
      if (!test_device) {
        test_device = dev;
        test_device_properties = lzt::get_device_properties(dev);
      } else {
        // Compare the uuid of this device to the current test_device
        auto dev_properties = lzt::get_device_properties(dev);
        auto select = memcmp(&dev_properties.uuid, &test_device_properties.uuid,
                             ZE_MAX_DEVICE_UUID_SIZE);

        if (select < 0) {
          test_device = dev;
          test_device_properties = dev_properties;
        }
      }
    }
  }

  if (!test_device) {
    exit(0);
  }
  devices = lzt::get_ze_sub_devices(test_device);

  auto device_0 = devices[0];
  auto device_1 = devices[1];
  auto dev_properties_0 = lzt::get_device_properties(device_0);
  auto dev_properties_1 = lzt::get_device_properties(device_1);

  auto select = memcmp(&dev_properties_0.uuid, &dev_properties_1.uuid,
                       ZE_MAX_DEVICE_UUID_SIZE);

  ze_device_handle_t device = device_0;
  if (dev_properties_0.subdeviceId < dev_properties_1.subdeviceId) {
    device = device_0;
    LOG_DEBUG << "Receiver Selected subdevice 0" << std::endl;
  } else if (dev_properties_0.subdeviceId > dev_properties_1.subdeviceId) {
    device = device_1;
    LOG_DEBUG << "Receiver Selected subdevice 1" << std::endl;
  } else {
    LOG_WARNING << "Devices have the same subdeviceId" << std::endl;
  }

  auto context = lzt::create_context(driver);
  auto cmd_bundle = lzt::create_command_bundle(
      context, device, 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0, 0, 0, is_immediate);
  ze_ipc_mem_handle_t ipc_handle{};
  std::fill_n(ipc_handle.data, ZE_MAX_IPC_HANDLE_SIZE, 0);
  auto ipc_descriptor =
      lzt::receive_ipc_handle<ze_ipc_mem_handle_t>(ipc_handle.data);
  memcpy(&ipc_handle, static_cast<void *>(&ipc_descriptor),
         sizeof(ipc_descriptor));

  void *memory = nullptr;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeMemOpenIpcHandle(context, device, ipc_handle, 0, &memory));

  void *buffer = lzt::allocate_host_memory(size, 1, context);
  memset(buffer, 0, size);
  lzt::append_memory_copy(cmd_bundle.list, buffer, memory, size);
  lzt::close_command_list(cmd_bundle.list);
  lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

  lzt::validate_data_pattern(buffer, size, 1);

  EXPECT_EQ(ZE_RESULT_SUCCESS, zeMemCloseIpcHandle(context, memory));
  lzt::free_memory(context, buffer);
  lzt::destroy_command_bundle(cmd_bundle);
  lzt::destroy_context(context);
}

void RunGivenL0MemoryAllocatedInParentProcessWhenUsingMultipleDevicesWithIPCTest(
    bool reserved, bool is_immediate) {
  size_t size = 4096;

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
    pid_t pid = fork();
    if (pid < 0) {
      throw std::runtime_error("Failed to fork child process");
    } else if (pid > 0) {
      multi_sub_device_sender(size, reserved, is_immediate);

      if (testing::Test::HasFailure()) {
        LOG_DEBUG << "IPC Sender Failed GTEST Check";
        exit(1);
      }
      exit(0);
    } else {
      multi_sub_device_receiver(size, is_immediate);

      if (testing::Test::HasFailure()) {
        LOG_DEBUG << "IPC Receiver Failed GTEST Check";
        exit(1);
      }
      exit(0);
    }
  }
}

TEST(
    IpcMemoryAccessTest,
    GivenL0MemoryAllocatedInParentProcessWhenUsingMultipleDevicesWithIPCThenChildProcessReadsMemoryCorrectly) {
  RunGivenL0MemoryAllocatedInParentProcessWhenUsingMultipleDevicesWithIPCTest(
      false, false);
}

TEST(
    IpcMemoryAccessTest,
    GivenL0MemoryAllocatedInParentProcessWhenUsingMultipleDevicesWithIPCOnImmediateCmdListThenChildProcessReadsMemoryCorrectly) {
  RunGivenL0MemoryAllocatedInParentProcessWhenUsingMultipleDevicesWithIPCTest(
      false, true);
}

TEST(
    IpcMemoryAccessTest,
    GivenL0PhysicalMemoryAllocatedAndReservedInParentProcessWhenUsingMultipleDevicesWithIPCThenChildProcessReadsMemoryCorrectly) {
  RunGivenL0MemoryAllocatedInParentProcessWhenUsingMultipleDevicesWithIPCTest(
      true, false);
}

TEST(
    IpcMemoryAccessTest,
    GivenL0PhysicalMemoryAllocatedAndReservedInParentProcessWhenUsingMultipleDevicesWithIPCOnImmediateCmdListThenChildProcessReadsMemoryCorrectly) {
  RunGivenL0MemoryAllocatedInParentProcessWhenUsingMultipleDevicesWithIPCTest(
      true, true);
}
#endif

} // namespace

// We put the main here because L0 doesn't currently specify how
// zeInit should be handled with fork(), so each process must call
// zeInit
int main(int argc, char **argv) {
  try {
    ::testing::InitGoogleMock(&argc, argv);
  } catch (std::exception &e) {
    std::cerr << "Failed to initialize GoogleMock: " << e.what() << std::endl;
    return 1;
  }
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
