/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "utils/utils.hpp"
#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"

namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>

namespace {

class zeP2PTests : public ::testing::Test,
                   public ::testing::WithParamInterface<ze_memory_type_t> {
protected:
  void SetUp() override {
    ze_memory_type_t memory_type = GetParam();
    ze_bool_t can_access;
    auto drivers = lzt::get_all_driver_handles();
    ASSERT_GT(drivers.size(), 0);
    auto devices = lzt::get_ze_devices(drivers[0]);
    if (devices.size() < 2) {
      SUCCEED();
      LOG_INFO << "WARNING:  Exiting as multiple devices do not exist";
      skip = true;
      return;
    }

    for (auto dev1 : devices) {
      for (auto dev2 : devices) {
        if (dev1 == dev2)
          continue;
        EXPECT_EQ(ZE_RESULT_SUCCESS,
                  zeDeviceCanAccessPeer(dev1, dev2, &can_access));
        if (can_access == false) {
          LOG_INFO << "WARNING:  Exiting as device peer access not enabled";
          SUCCEED();
          skip = true;
          return;
        }
      }
    }

    for (auto device : devices) {
      DevInstance instance;
      instance.dev = device;
      instance.dev_grp = drivers[0];
      if (memory_type == ZE_MEMORY_TYPE_DEVICE) {
        instance.src_region = lzt::allocate_device_memory(
            mem_size_, 1, ZE_DEVICE_MEM_ALLOC_FLAG_DEFAULT, device, drivers[0]);
        instance.dst_region = lzt::allocate_device_memory(
            mem_size_, 1, ZE_DEVICE_MEM_ALLOC_FLAG_DEFAULT, device, drivers[0]);
      } else if (memory_type == ZE_MEMORY_TYPE_SHARED) {
        instance.src_region = lzt::allocate_shared_memory(
            mem_size_, 1, ZE_DEVICE_MEM_ALLOC_FLAG_DEFAULT,
            ZE_HOST_MEM_ALLOC_FLAG_DEFAULT, device);
        instance.dst_region = lzt::allocate_shared_memory(
            mem_size_, 1, ZE_DEVICE_MEM_ALLOC_FLAG_DEFAULT,
            ZE_HOST_MEM_ALLOC_FLAG_DEFAULT, device);
      } else {
        FAIL() << "Unexpected memory type";
      }

      instance.cmd_list = lzt::create_command_list(device);
      instance.cmd_q = lzt::create_command_queue(device);

      dev_instance_.push_back(instance);
    }
  }

  void TearDown() override {
    if (skip)
      return;
    for (auto instance : dev_instance_) {

      lzt::destroy_command_queue(instance.cmd_q);
      lzt::destroy_command_list(instance.cmd_list);

      lzt::free_memory(instance.src_region);
      lzt::free_memory(instance.dst_region);
    }
  }

  struct DevInstance {
    ze_device_handle_t dev;
    ze_driver_handle_t dev_grp;
    void *src_region;
    void *dst_region;
    ze_command_list_handle_t cmd_list;
    ze_command_queue_handle_t cmd_q;
  };

  size_t mem_size_ = 16;
  std::vector<DevInstance> dev_instance_;
  bool skip = false;
};

TEST_P(
    zeP2PTests,
    GivenP2PDevicesWhenCopyingDeviceMemoryToAndFromRemoteDeviceThenSuccessIsReturned) {
  if (skip)
    return;
  uint32_t dev_instance_size = dev_instance_.size();

  for (uint32_t i = 1; i < dev_instance_.size(); i++) {

    lzt::append_memory_copy(dev_instance_[i - 1].cmd_list,
                            dev_instance_[i - 1].dst_region,
                            dev_instance_[i].src_region, mem_size_, nullptr);
    lzt::append_memory_copy(
        dev_instance_[i].cmd_list, dev_instance_[i].dst_region,
        dev_instance_[i - 1].src_region, mem_size_, nullptr);

    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeCommandListClose(dev_instance_[i - 1].cmd_list));
    EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListClose(dev_instance_[i].cmd_list));

    EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandQueueExecuteCommandLists(
                                     dev_instance_[i - 1].cmd_q, 1,
                                     &dev_instance_[i - 1].cmd_list, nullptr));
    EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandQueueExecuteCommandLists(
                                     dev_instance_[i].cmd_q, 1,
                                     &dev_instance_[i].cmd_list, nullptr));

    EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandQueueSynchronize(
                                     dev_instance_[i - 1].cmd_q, UINT32_MAX));
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeCommandListReset(dev_instance_[i - 1].cmd_list));
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeCommandQueueSynchronize(dev_instance_[i].cmd_q, UINT32_MAX));
    EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListReset(dev_instance_[i].cmd_list));
  }
}

INSTANTIATE_TEST_CASE_P(
    GivenP2PDevicesWhenCopyingDeviceMemoryFromRemoteDeviceThenSuccessIsReturned_IP,
    zeP2PTests, testing::Values(ZE_MEMORY_TYPE_DEVICE, ZE_MEMORY_TYPE_SHARED));

TEST_P(
    zeP2PTests,
    GivenP2PDevicesWhenSettingAndCopyingMemoryToRemoteDeviceThenRemoteDeviceGetsCorrectMemory) {
  if (skip)
    return;
  uint32_t dev_instance_size = dev_instance_.size();

  for (uint32_t i = 1; i < dev_instance_.size(); i++) {
    uint8_t *shr_mem = static_cast<uint8_t *>(lzt::allocate_shared_memory(
        mem_size_, 1, ZE_DEVICE_MEM_ALLOC_FLAG_DEFAULT,
        ZE_HOST_MEM_ALLOC_FLAG_DEFAULT, dev_instance_[i].dev));
    uint8_t value = rand() & 0xff;

    // Set memory region on device i - 1 and copy to device i
    lzt::append_memory_set(dev_instance_[i - 1].cmd_list,
                           dev_instance_[i - 1].src_region, &value, mem_size_);
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeCommandListAppendBarrier(dev_instance_[i - 1].cmd_list, nullptr,
                                         0, nullptr));
    lzt::append_memory_copy(
        dev_instance_[i - 1].cmd_list, dev_instance_[i].dst_region,
        dev_instance_[i - 1].src_region, mem_size_, nullptr);
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeCommandListClose(dev_instance_[i - 1].cmd_list));
    EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandQueueExecuteCommandLists(
                                     dev_instance_[i - 1].cmd_q, 1,
                                     &dev_instance_[i - 1].cmd_list, nullptr));

    EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandQueueSynchronize(
                                     dev_instance_[i - 1].cmd_q, UINT32_MAX));
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeCommandListReset(dev_instance_[i - 1].cmd_list));

    // Copy memory region from device i to shared mem, and verify it is correct
    lzt::append_memory_copy(dev_instance_[i].cmd_list, shr_mem,
                            dev_instance_[i].dst_region, mem_size_, nullptr);
    EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListClose(dev_instance_[i].cmd_list));
    EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandQueueExecuteCommandLists(
                                     dev_instance_[i].cmd_q, 1,
                                     &dev_instance_[i].cmd_list, nullptr));

    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeCommandQueueSynchronize(dev_instance_[i].cmd_q, UINT32_MAX));
    EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListReset(dev_instance_[i].cmd_list));

    for (uint32_t j = 0; j < mem_size_; j++) {
      ASSERT_EQ(shr_mem[j], value)
          << "Memory Copied from Device did not match.";
    }
    lzt::free_memory(shr_mem);
  }
}

INSTANTIATE_TEST_CASE_P(
    GivenP2PDevicesWhenSettingAndCopyingMemoryToRemoteDeviceThenRemoteDeviceGetsCorrectMemory_IP,
    zeP2PTests, testing::Values(ZE_MEMORY_TYPE_DEVICE, ZE_MEMORY_TYPE_SHARED));

TEST_P(zeP2PTests,
       GivenP2PDevicesWhenKernelReadsRemoteDeviceMemoryThenCorrectDataIsRead) {
  if (skip)
    return;
  std::string module_name = "p2p_test.spv";
  std::string func_name = "multi_device_function";

  uint32_t dev_instance_size = dev_instance_.size();

  for (uint32_t i = 1; i < dev_instance_.size(); i++) {

    ze_module_handle_t module =
        lzt::create_module(dev_instance_[i - 1].dev, module_name);
    uint8_t *shr_mem = static_cast<uint8_t *>(lzt::allocate_shared_memory(
        mem_size_, 1, ZE_DEVICE_MEM_ALLOC_FLAG_DEFAULT,
        ZE_HOST_MEM_ALLOC_FLAG_DEFAULT, dev_instance_[i].dev));

    // random memory region on device i. Allow "space" for increment.
    uint8_t value = rand() & 0x7f;
    lzt::append_memory_set(dev_instance_[i].cmd_list,
                           dev_instance_[i].src_region, &value, mem_size_);
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeCommandListAppendBarrier(dev_instance_[i].cmd_list, nullptr, 0,
                                         nullptr));
    EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListClose(dev_instance_[i].cmd_list));
    EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandQueueExecuteCommandLists(
                                     dev_instance_[i].cmd_q, 1,
                                     &dev_instance_[i].cmd_list, nullptr));
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeCommandQueueSynchronize(dev_instance_[i].cmd_q, UINT32_MAX));

    EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListReset(dev_instance_[i].cmd_list));

    // device (i - 1) will modify memory allocated for device i
    lzt::create_and_execute_function(dev_instance_[i - 1].dev, module,
                                     func_name, 1, dev_instance_[i].src_region);

    // copy memory to shared region and verify it is correct
    lzt::append_memory_copy(dev_instance_[i].cmd_list, shr_mem,
                            dev_instance_[i].src_region, mem_size_, nullptr);
    EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListClose(dev_instance_[i].cmd_list));
    EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandQueueExecuteCommandLists(
                                     dev_instance_[i].cmd_q, 1,
                                     &dev_instance_[i].cmd_list, nullptr));

    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeCommandQueueSynchronize(dev_instance_[i].cmd_q, UINT32_MAX));
    EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListReset(dev_instance_[i].cmd_list));
    ASSERT_EQ(shr_mem[0], value + 1)
        << "Memory Copied from Device did not match.";

    lzt::destroy_module(module);
  }
}

INSTANTIATE_TEST_CASE_P(
    GivenP2PDevicesWhenKernelReadsRemoteDeviceMemoryThenCorrectDataIsRead_IP,
    zeP2PTests, testing::Values(ZE_MEMORY_TYPE_DEVICE, ZE_MEMORY_TYPE_SHARED));

} // namespace
