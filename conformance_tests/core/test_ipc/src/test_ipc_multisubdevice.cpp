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

void multi_sub_device_sender(size_t size) {
  zeInit(0);

  auto driver = lzt::get_default_driver();
  auto context = lzt::create_context(driver);
  auto devices = lzt::get_devices(driver);

  // devices are not guaranteed to be reported in the same order, so
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

  auto command_list = lzt::create_command_list(context, device, 0);
  auto command_queue = lzt::create_command_queue(
      context, device, 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);

  ze_device_mem_alloc_flags_t flags = 0;
  auto memory = lzt::allocate_device_memory(size, 1, flags, device, context);

  void *buffer = lzt::allocate_host_memory(size, 1, context);
  lzt::write_data_pattern(buffer, size, 1);
  lzt::append_memory_copy(command_list, memory, buffer, size);

  lzt::close_command_list(command_list);
  lzt::execute_command_lists(command_queue, 1, &command_list, nullptr);

  ze_ipc_mem_handle_t ipc_handle = {};
  lzt::get_ipc_handle(context, &ipc_handle, memory);
  lzt::send_ipc_handle(ipc_handle);

  lzt::free_memory(context, buffer);
  lzt::destroy_command_list(command_list);
  lzt::destroy_command_queue(command_queue);
  lzt::destroy_context(context);
}

void multi_sub_device_receiver(size_t size) {
  zeInit(0);
  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);
  auto context = lzt::create_context(driver);

  ze_ipc_mem_handle_t ipc_handle;
  void *memory = nullptr;

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

  auto cl = lzt::create_command_list(context, device, 0);
  auto cq = lzt::create_command_queue(context, device, 0,
                                      ZE_COMMAND_QUEUE_MODE_DEFAULT,
                                      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0);
  auto ipc_descriptor = lzt::receive_ipc_handle<ze_ipc_mem_handle_t>();
  memcpy(&ipc_handle, static_cast<void *>(&ipc_descriptor),
         sizeof(ipc_descriptor));

  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeMemOpenIpcHandle(context, device, ipc_handle, 0, &memory));

  void *buffer = lzt::allocate_host_memory(size, 1, context);
  memset(buffer, 0, size);
  lzt::append_memory_copy(cl, buffer, memory, size);
  lzt::close_command_list(cl);
  lzt::execute_command_lists(cq, 1, &cl, nullptr);
  lzt::synchronize(cq, UINT64_MAX);

  lzt::validate_data_pattern(buffer, size, 1);

  EXPECT_EQ(ZE_RESULT_SUCCESS, zeMemCloseIpcHandle(context, memory));
  lzt::free_memory(context, buffer);
  lzt::destroy_command_list(cl);
  lzt::destroy_command_queue(cq);
  lzt::destroy_context(context);
}

TEST(
    IpcMemoryAccessTest,
    GivenL0MemoryAllocatedInChildProcessWhenUsingL0IPCMultiDeviceThenParentProcessReadsMemoryCorrectly) {
  size_t size = 4096;
  pid_t pid = fork();
  if (pid < 0) {
    throw std::runtime_error("Failed to fork child process");
  } else if (pid > 0) {
    multi_sub_device_receiver(size);
  } else {
    multi_sub_device_sender(size);
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