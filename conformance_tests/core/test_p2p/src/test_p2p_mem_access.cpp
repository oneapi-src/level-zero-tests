/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"

namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>

namespace {
enum MemAccessTestType { ATOMIC, CONCURRENT, CONCURRENT_ATOMIC };

class zeP2PTests : public ::testing::Test,
                   public ::testing::WithParamInterface<ze_memory_type_t> {
protected:
  void SetUp() override {
    ze_memory_type_t memory_type = GetParam();
    ze_bool_t can_access;
    std::vector<ze_device_handle_t> devices;
    std::vector<ze_device_handle_t> sub_devices;
    auto drivers = lzt::get_all_driver_handles();
    ASSERT_GT(drivers.size(), 0);
    auto device_scan = lzt::get_ze_devices(drivers[0]);
    for (auto dev : device_scan) {
      if (lzt::get_ze_sub_device_count(dev) > 0) {
        sub_devices = lzt::get_ze_sub_devices(dev);
        devices.insert(devices.end(), sub_devices.begin(), sub_devices.end());
        sub_devices.clear();
      } else {
        devices.push_back(dev);
      }
    }
    device_scan.clear();

    if (devices.size() < 2) {
      LOG_INFO << "WARNING:  Exiting test due to lack of multiple devices";
      SUCCEED();
      skip = true;
      return;
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
    if (skip) {
      return;
    }
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
  if (skip) {
    return;
  }
  ze_bool_t n1_can_access_n2;
  ze_bool_t n2_can_access_n1;
  uint32_t copy_count = 0;

  for (uint32_t i = 1; i < dev_instance_.size(); i++) {

    n1_can_access_n2 =
        lzt::can_access_peer(dev_instance_[i - 1].dev, dev_instance_[i].dev);
    n2_can_access_n1 =
        lzt::can_access_peer(dev_instance_[i].dev, dev_instance_[i - 1].dev);

    if (n2_can_access_n1) {
      lzt::append_memory_copy(dev_instance_[i - 1].cmd_list,
                              dev_instance_[i - 1].dst_region,
                              dev_instance_[i].src_region, mem_size_, nullptr);
      EXPECT_EQ(ZE_RESULT_SUCCESS,
                zeCommandListClose(dev_instance_[i - 1].cmd_list));
      EXPECT_EQ(ZE_RESULT_SUCCESS,
                zeCommandQueueExecuteCommandLists(
                    dev_instance_[i - 1].cmd_q, 1,
                    &dev_instance_[i - 1].cmd_list, nullptr));
      EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandQueueSynchronize(
                                       dev_instance_[i - 1].cmd_q, UINT32_MAX));
      EXPECT_EQ(ZE_RESULT_SUCCESS,
                zeCommandListReset(dev_instance_[i - 1].cmd_list));
      copy_count++;
    }
    if (n1_can_access_n2) {
      lzt::append_memory_copy(
          dev_instance_[i].cmd_list, dev_instance_[i].dst_region,
          dev_instance_[i - 1].src_region, mem_size_, nullptr);
      EXPECT_EQ(ZE_RESULT_SUCCESS,
                zeCommandListClose(dev_instance_[i].cmd_list));
      EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandQueueExecuteCommandLists(
                                       dev_instance_[i].cmd_q, 1,
                                       &dev_instance_[i].cmd_list, nullptr));
      EXPECT_EQ(ZE_RESULT_SUCCESS,
                zeCommandQueueSynchronize(dev_instance_[i].cmd_q, UINT32_MAX));
      EXPECT_EQ(ZE_RESULT_SUCCESS,
                zeCommandListReset(dev_instance_[i].cmd_list));
      copy_count++;
    }
    if (copy_count == 0) {
      LOG_INFO << "WARNING: Exiting as no device peer access enabled";
      SUCCEED();
    }
  }
}

INSTANTIATE_TEST_CASE_P(
    GivenP2PDevicesWhenCopyingDeviceMemoryFromRemoteDeviceThenSuccessIsReturned_IP,
    zeP2PTests, testing::Values(ZE_MEMORY_TYPE_DEVICE, ZE_MEMORY_TYPE_SHARED));

TEST_P(
    zeP2PTests,
    GivenP2PDevicesWhenSettingAndCopyingMemoryToRemoteDeviceThenRemoteDeviceGetsCorrectMemory) {
  if (skip) {
    return;
  }
  uint32_t copy_count = 0;
  DevInstance *ptr_dev_src;
  DevInstance *ptr_dev_dst;
  for (uint32_t i = 1; i < dev_instance_.size(); i++) {
    if (lzt::can_access_peer(dev_instance_[i - 1].dev, dev_instance_[i].dev)) {
      ptr_dev_src = &dev_instance_[i - 1];
      ptr_dev_dst = &dev_instance_[i];
    } else if (lzt::can_access_peer(dev_instance_[i].dev,
                                    dev_instance_[i - 1].dev)) {
      ptr_dev_src = &dev_instance_[i];
      ptr_dev_dst = &dev_instance_[i - 1];
    } else {
      continue;
    }
    uint8_t *shr_mem = static_cast<uint8_t *>(lzt::allocate_shared_memory(
        mem_size_, 1, ZE_DEVICE_MEM_ALLOC_FLAG_DEFAULT,
        ZE_HOST_MEM_ALLOC_FLAG_DEFAULT, ptr_dev_dst->dev));
    uint8_t value = rand() & 0xff;

    // Set memory region on source device and copy to destination device
    lzt::append_memory_set(ptr_dev_src->cmd_list, ptr_dev_src->src_region,
                           &value, mem_size_);
    EXPECT_EQ(
        ZE_RESULT_SUCCESS,
        zeCommandListAppendBarrier(ptr_dev_src->cmd_list, nullptr, 0, nullptr));
    lzt::append_memory_copy(ptr_dev_src->cmd_list, ptr_dev_dst->dst_region,
                            ptr_dev_src->src_region, mem_size_, nullptr);
    EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListClose(ptr_dev_src->cmd_list));
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeCommandQueueExecuteCommandLists(
                  ptr_dev_src->cmd_q, 1, &ptr_dev_src->cmd_list, nullptr));

    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeCommandQueueSynchronize(ptr_dev_src->cmd_q, UINT32_MAX));
    EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListReset(ptr_dev_src->cmd_list));

    // Copy memory region from dest device to shared mem, and verify it is
    // correct
    lzt::append_memory_copy(ptr_dev_dst->cmd_list, shr_mem,
                            ptr_dev_dst->dst_region, mem_size_, nullptr);
    EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListClose(ptr_dev_dst->cmd_list));
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeCommandQueueExecuteCommandLists(
                  ptr_dev_dst->cmd_q, 1, &ptr_dev_dst->cmd_list, nullptr));

    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeCommandQueueSynchronize(ptr_dev_dst->cmd_q, UINT32_MAX));
    EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListReset(ptr_dev_dst->cmd_list));

    for (uint32_t j = 0; j < mem_size_; j++) {
      ASSERT_EQ(shr_mem[j], value)
          << "Memory Copied from Device did not match.";
    }
    lzt::free_memory(shr_mem);
    copy_count++;
  }
  if (copy_count == 0) {
    LOG_INFO << "WARNING: Exiting as no device peer access enabled";
    SUCCEED();
  }
}

INSTANTIATE_TEST_CASE_P(
    GivenP2PDevicesWhenSettingAndCopyingMemoryToRemoteDeviceThenRemoteDeviceGetsCorrectMemory_IP,
    zeP2PTests, testing::Values(ZE_MEMORY_TYPE_DEVICE, ZE_MEMORY_TYPE_SHARED));

TEST_P(zeP2PTests,
       GivenP2PDevicesWhenKernelReadsRemoteDeviceMemoryThenCorrectDataIsRead) {
  if (skip) {
    return;
  }
  std::string module_name = "p2p_test.spv";
  std::string func_name = "multi_device_function";

  uint32_t access_count = 0;
  DevInstance *ptr_dev_src;
  DevInstance *ptr_dev_dst;

  for (uint32_t i = 1; i < dev_instance_.size(); i++) {
    if (lzt::can_access_peer(dev_instance_[i - 1].dev, dev_instance_[i].dev)) {
      ptr_dev_src = &dev_instance_[i - 1];
      ptr_dev_dst = &dev_instance_[i];
    } else if (lzt::can_access_peer(dev_instance_[i].dev,
                                    dev_instance_[i - 1].dev)) {
      ptr_dev_src = &dev_instance_[i];
      ptr_dev_dst = &dev_instance_[i - 1];
    } else {
      continue;
    }
    ze_module_handle_t module =
        lzt::create_module(ptr_dev_src->dev, module_name);
    uint8_t *shr_mem = static_cast<uint8_t *>(lzt::allocate_shared_memory(
        mem_size_, 1, ZE_DEVICE_MEM_ALLOC_FLAG_DEFAULT,
        ZE_HOST_MEM_ALLOC_FLAG_DEFAULT, ptr_dev_dst->dev));

    // random memory region on destination device. Allow "space" for increment.
    uint8_t value = rand() & 0x7f;
    lzt::append_memory_set(ptr_dev_dst->cmd_list, ptr_dev_dst->src_region,
                           &value, mem_size_);
    EXPECT_EQ(
        ZE_RESULT_SUCCESS,
        zeCommandListAppendBarrier(ptr_dev_dst->cmd_list, nullptr, 0, nullptr));
    EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListClose(ptr_dev_dst->cmd_list));
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeCommandQueueExecuteCommandLists(
                  ptr_dev_dst->cmd_q, 1, &ptr_dev_dst->cmd_list, nullptr));
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeCommandQueueSynchronize(ptr_dev_dst->cmd_q, UINT32_MAX));

    EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListReset(ptr_dev_dst->cmd_list));

    // source device will modify memory allocated for destination device
    lzt::create_and_execute_function(ptr_dev_src->dev, module, func_name, 1,
                                     ptr_dev_dst->src_region);

    // on destination device copy memory to shared region and verify it is
    // correct
    lzt::append_memory_copy(ptr_dev_dst->cmd_list, shr_mem,
                            ptr_dev_dst->src_region, mem_size_, nullptr);
    EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListClose(ptr_dev_dst->cmd_list));
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeCommandQueueExecuteCommandLists(
                  ptr_dev_dst->cmd_q, 1, &ptr_dev_dst->cmd_list, nullptr));

    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeCommandQueueSynchronize(ptr_dev_dst->cmd_q, UINT32_MAX));
    EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListReset(ptr_dev_dst->cmd_list));
    ASSERT_EQ(shr_mem[0], value + 1)
        << "Memory Copied from Device did not match.";

    lzt::destroy_module(module);
    access_count++;
  }
  if (access_count == 0) {
    LOG_INFO << "WARNING: Exiting as no device peer access enabled";
    SUCCEED();
  }
}

INSTANTIATE_TEST_CASE_P(
    GivenP2PDevicesWhenKernelReadsRemoteDeviceMemoryThenCorrectDataIsRead_IP,
    zeP2PTests, testing::Values(ZE_MEMORY_TYPE_DEVICE, ZE_MEMORY_TYPE_SHARED));

class zeP2PKernelTests : public lzt::zeEventPoolTests,
                         public ::testing::WithParamInterface<
                             std::tuple<std::string, ze_memory_type_t>> {
protected:
  void SetUp() override {

    ze_bool_t can_access;

    auto drivers = lzt::get_all_driver_handles();
    ASSERT_GE(drivers.size(), 1);

    std::vector<ze_device_handle_t> devices;
    std::vector<ze_device_handle_t> sub_devices;
    std::vector<ze_device_handle_t> device_scan;
    ze_driver_handle_t driver;
    for (auto dg : drivers) {
      driver = dg;
      device_scan = lzt::get_devices(driver);
      for (auto dev : device_scan) {
        if (lzt::get_ze_sub_device_count(dev) > 0) {
          devices.push_back(dev);
          sub_devices = lzt::get_ze_sub_devices(dev);
          devices.insert(devices.end(), sub_devices.begin(), sub_devices.end());
          sub_devices.clear();
        } else {
          devices.push_back(dev);
        }
      }
      device_scan.clear();
      if (devices.size() >= 2)
        break;
    }

    if (devices.size() < 2) {
      LOG_INFO << "WARNING:  Exiting test due to lack of multiple devices";
      SUCCEED();
      skip = true;
      return;
    }
    get_devices_ = devices.size();
    LOG_INFO << "Detected " << get_devices_ << " devices";

    ze_device_compute_properties_t dev_compute_properties;

    for (auto device : devices) {
      DevAccess instance;
      instance.dev = device;
      instance.dev_grp = driver;
      instance.device_mem_local = nullptr;
      instance.device_mem_remote = nullptr;
      instance.shared_mem = nullptr;
      instance.cmd_list = nullptr;
      instance.cmd_q = nullptr;
      instance.module = nullptr;
      instance.function = nullptr;
      instance.init_val = 0;
      instance.kernel_add_val = 0;
      dev_compute_properties.version =
          ZE_DEVICE_COMPUTE_PROPERTIES_VERSION_CURRENT;
      EXPECT_EQ(ZE_RESULT_SUCCESS,
                zeDeviceGetComputeProperties(device, &dev_compute_properties));
      instance.group_size_x = std::min(
          static_cast<uint32_t>(128), dev_compute_properties.maxTotalGroupSize);
      instance.group_count_x = 1;
      instance.dev_mem_properties.version =
          ZE_DEVICE_MEMORY_PROPERTIES_VERSION_CURRENT;
      uint32_t num_mem_properties = 1;
      EXPECT_EQ(ZE_RESULT_SUCCESS,
                zeDeviceGetMemoryProperties(device, &num_mem_properties,
                                            &instance.dev_mem_properties));
      instance.dev_mem_access_properties.version =
          ZE_DEVICE_MEMORY_ACCESS_PROPERTIES_VERSION_CURRENT;

      EXPECT_EQ(ZE_RESULT_SUCCESS,
                zeDeviceGetMemoryAccessProperties(
                    device, &instance.dev_mem_access_properties));

      dev_access_scan_.push_back(instance);
    }
  }

  ze_kernel_handle_t create_function(const ze_module_handle_t module,
                                     const std::string name,
                                     uint32_t group_size_x, int *arg1,
                                     int arg2) {

    ze_kernel_desc_t function_description;
    function_description.version = ZE_KERNEL_DESC_VERSION_CURRENT;
    function_description.flags = ZE_KERNEL_FLAG_NONE;
    function_description.pKernelName = name.c_str();
    ze_kernel_handle_t function = nullptr;
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeKernelCreate(module, &function_description, &function));

    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeKernelSetGroupSize(function, group_size_x, 1, 1));

    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeKernelSetArgumentValue(function, 0, sizeof(arg1), &arg1));

    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeKernelSetArgumentValue(function, 1, sizeof(arg2), &arg2));

    return function;
  }

  void setup_cmd_list(uint32_t i, uint32_t num, bool sync,
                      ze_event_handle_t event_sync_start,
                      enum MemAccessTestType type,
                      std::vector<ze_event_handle_t> events_sync_end) {

    void *p_init_val = &dev_access_[i].init_val;

    dev_access_[i].cmd_list = lzt::create_command_list(dev_access_[i].dev);

    if ((num > 1) && (i == (num - 1))) {
      lzt::append_memory_copy(dev_access_[i].cmd_list,
                              dev_access_[i].device_mem_local, p_init_val,
                              sizeof(int), nullptr);
    } else if (type != CONCURRENT_ATOMIC) {
      lzt::append_memory_copy(dev_access_[i].cmd_list,
                              dev_access_[i].device_mem_remote, p_init_val,
                              sizeof(int), nullptr);
    }

    if (sync) {
      if (i == 0) {
        EXPECT_EQ(ZE_RESULT_SUCCESS,
                  zeCommandListAppendSignalEvent(dev_access_[i].cmd_list,
                                                 event_sync_start));
      } else {
        EXPECT_EQ(ZE_RESULT_SUCCESS,
                  zeCommandListAppendWaitOnEvents(dev_access_[i].cmd_list, 1,
                                                  &event_sync_start));
      }
    }

    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeCommandListAppendBarrier(dev_access_[i].cmd_list, nullptr, 0,
                                         nullptr));

    ze_group_count_t thread_group_dimensions;
    thread_group_dimensions.groupCountX = dev_access_[i].group_count_x;
    thread_group_dimensions.groupCountY = 1;
    thread_group_dimensions.groupCountZ = 1;

    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeCommandListAppendLaunchKernel(
                  dev_access_[i].cmd_list, dev_access_[i].function,
                  &thread_group_dimensions, nullptr, 0, nullptr));
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeCommandListAppendBarrier(dev_access_[i].cmd_list, nullptr, 0,
                                         nullptr));

    if (sync) {
      if (i == 0) {
        EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListAppendWaitOnEvents(
                                         dev_access_[i].cmd_list, num - 1,
                                         events_sync_end.data()));
      } else {
        EXPECT_EQ(ZE_RESULT_SUCCESS,
                  zeCommandListAppendSignalEvent(dev_access_[i].cmd_list,
                                                 events_sync_end[i - 1]));
      }
    }
    if (i == 0) {
      lzt::append_memory_copy(
          dev_access_[i].cmd_list, dev_access_[i].shared_mem,
          dev_access_[i].device_mem_remote, num * sizeof(int), nullptr);

      EXPECT_EQ(ZE_RESULT_SUCCESS,
                zeCommandListAppendBarrier(dev_access_[i].cmd_list, nullptr, 0,
                                           nullptr));
    }

    EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListClose(dev_access_[i].cmd_list));
  }

  void run_test(uint32_t num, bool sync, enum MemAccessTestType type) {
    const std::string module_path = kernel_name_ + ".spv";
    ze_event_handle_t event_sync_start = nullptr;
    std::vector<ze_event_handle_t> events_sync_end(num, nullptr);

    EXPECT_GT(num, 0);

    if (sync) {
      ep.create_event(event_sync_start, ZE_EVENT_SCOPE_FLAG_DEVICE,
                      ZE_EVENT_SCOPE_FLAG_DEVICE);
      ep.create_events(events_sync_end, num - 1, ZE_EVENT_SCOPE_FLAG_DEVICE,
                       ZE_EVENT_SCOPE_FLAG_DEVICE);
    }

    if (type == CONCURRENT_ATOMIC) {
      dev_access_[0].init_val = 0;
    } else {
      dev_access_[0].init_val = 1;
    }
    dev_access_[0].kernel_add_val = 10;
    if (memory_type_ == ZE_MEMORY_TYPE_DEVICE) {
      dev_access_[0].device_mem_remote = lzt::allocate_device_memory(
          (num * sizeof(int)), 1, ZE_DEVICE_MEM_ALLOC_FLAG_DEFAULT,
          dev_access_[std::max(static_cast<uint32_t>(1), (num - 1))].dev,
          dev_access_[std::max(static_cast<uint32_t>(1), (num - 1))].dev_grp);
    } else if (memory_type_ == ZE_MEMORY_TYPE_SHARED) {
      dev_access_[0].device_mem_remote = lzt::allocate_shared_memory(
          (num * sizeof(int)), 1, ZE_DEVICE_MEM_ALLOC_FLAG_DEFAULT,
          ZE_HOST_MEM_ALLOC_FLAG_DEFAULT,
          dev_access_[std::max(static_cast<uint32_t>(1), (num - 1))].dev);
    } else {
      FAIL() << "Unexpected memory type";
    }
    dev_access_[0].shared_mem = lzt::allocate_shared_memory(
        (num * sizeof(int)), 1, ZE_DEVICE_MEM_ALLOC_FLAG_DEFAULT,
        ZE_HOST_MEM_ALLOC_FLAG_DEFAULT, dev_access_[0].dev);
    memset(dev_access_[0].shared_mem, 0xff, num * sizeof(int));
    dev_access_[0].module = lzt::create_module(dev_access_[0].dev, module_path);
    if (type == CONCURRENT) {
      dev_access_[0].group_size_x = 1;
      dev_access_[0].group_count_x = 100;
    }
    dev_access_[0].function = create_function(
        dev_access_[0].module, kernel_name_, dev_access_[0].group_size_x,
        (int *)dev_access_[0].device_mem_remote, dev_access_[0].kernel_add_val);
    setup_cmd_list(0, num, sync, event_sync_start, type, events_sync_end);
    dev_access_[0].cmd_q = lzt::create_command_queue(
        dev_access_[0].dev, ZE_COMMAND_QUEUE_FLAG_NONE,
        ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS, ZE_COMMAND_QUEUE_PRIORITY_NORMAL,
        0);
    for (uint32_t i = 1; i < num; i++) {
      if (type == CONCURRENT_ATOMIC) {
        dev_access_[i].init_val = dev_access_[0].init_val;
        dev_access_[i].kernel_add_val = dev_access_[0].kernel_add_val;
      } else {
        dev_access_[i].init_val = (1 + i) * dev_access_[0].init_val;
        dev_access_[i].kernel_add_val = (1 + i) * dev_access_[0].kernel_add_val;
      }
      dev_access_[i].module =
          lzt::create_module(dev_access_[i].dev, module_path);
      uint8_t *char_src =
          static_cast<uint8_t *>(dev_access_[0].device_mem_remote);
      if (type == CONCURRENT) {
        char_src += (i * sizeof(int));
        dev_access_[i].group_size_x = 1;
        dev_access_[i].group_count_x = dev_access_[0].group_count_x;
      }
      if (i < num - 1) {
        dev_access_[i].device_mem_remote =
            static_cast<void *>(static_cast<uint8_t *>(char_src));
        dev_access_[i].function = create_function(
            dev_access_[i].module, kernel_name_, dev_access_[i].group_size_x,
            (int *)dev_access_[i].device_mem_remote,
            dev_access_[i].kernel_add_val);
      } else {
        dev_access_[i].device_mem_local =
            static_cast<void *>(static_cast<uint8_t *>(char_src));
        dev_access_[i].function = create_function(
            dev_access_[i].module, kernel_name_, dev_access_[i].group_size_x,
            (int *)dev_access_[i].device_mem_local,
            dev_access_[i].kernel_add_val);
      }

      setup_cmd_list(i, num, sync, event_sync_start, type, events_sync_end);
      dev_access_[i].cmd_q = lzt::create_command_queue(
          dev_access_[i].dev, ZE_COMMAND_QUEUE_FLAG_NONE,
          ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS, ZE_COMMAND_QUEUE_PRIORITY_NORMAL,
          0);
    }
    for (uint32_t i = 0; i < num; i++) {
      EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandQueueExecuteCommandLists(
                                       dev_access_[i].cmd_q, 1,
                                       &dev_access_[i].cmd_list, nullptr));
    }

    for (uint32_t i = 0; i < num; i++) {
      EXPECT_EQ(ZE_RESULT_SUCCESS,
                zeCommandQueueSynchronize(dev_access_[i].cmd_q, UINT32_MAX));
      EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListReset(dev_access_[i].cmd_list));
    }

    if (sync) {
      ep.destroy_event(event_sync_start);
      ep.destroy_events(events_sync_end);
    }
    lzt::free_memory(dev_access_[0].device_mem_remote);

    for (uint32_t i = 0; i < num; i++) {
      lzt::destroy_command_queue(dev_access_[i].cmd_q);
      EXPECT_EQ(ZE_RESULT_SUCCESS,
                zeCommandListDestroy(dev_access_[i].cmd_list));
      EXPECT_EQ(ZE_RESULT_SUCCESS, zeKernelDestroy(dev_access_[i].function));
      EXPECT_EQ(ZE_RESULT_SUCCESS, zeModuleDestroy(dev_access_[i].module));
    }
  }

  typedef struct DevAccess {
    ze_device_handle_t dev;
    ze_driver_handle_t dev_grp;
    void *device_mem_local;
    void *device_mem_remote;
    void *shared_mem;
    int init_val;
    int kernel_add_val;
    uint32_t group_size_x;
    uint32_t group_count_x;
    ze_command_list_handle_t cmd_list;
    ze_command_queue_handle_t cmd_q;
    ze_module_handle_t module;
    ze_kernel_handle_t function;
    ze_device_memory_properties_t dev_mem_properties;
    ze_device_memory_access_properties_t dev_mem_access_properties;
  } DevAccess_t;

  std::vector<DevAccess> dev_access_;
  std::vector<DevAccess> dev_access_scan_;
  std::string kernel_name_;
  uint32_t get_devices_;
  ze_memory_type_t memory_type_;
  bool skip = false;
};

class zeP2PKernelTestsAtomicAccess : public zeP2PKernelTests {};

TEST_P(zeP2PKernelTestsAtomicAccess,
       GivenP2PDevicesWhenAtomicAccessOfPeerMemoryThenSuccessIsReturned) {
  if (skip) {
    return;
  }
  ze_device_p2p_properties_t dev_p2p_properties;

  // Search for compatible access
  dev_access_.clear();
  for (uint32_t i = 0; i < dev_access_scan_.size(); i++) {
    for (uint32_t j = i + 1; j < dev_access_scan_.size(); j++) {
      dev_p2p_properties = lzt::get_p2p_properties(dev_access_scan_[i].dev,
                                                   dev_access_scan_[j].dev);
      if ((dev_p2p_properties.accessSupported == true) &&
          (dev_p2p_properties.atomicsSupported == true)) {
        dev_access_.push_back(dev_access_scan_[i]);
        dev_access_.push_back(dev_access_scan_[j]);
        break;
      }
      dev_p2p_properties = lzt::get_p2p_properties(dev_access_scan_[j].dev,
                                                   dev_access_scan_[i].dev);
      if ((dev_p2p_properties.accessSupported == true) &&
          (dev_p2p_properties.atomicsSupported == true)) {
        dev_access_.push_back(dev_access_scan_[j]);
        dev_access_.push_back(dev_access_scan_[i]);
        break;
      }
    }
    if (dev_access_.size() >= 2)
      break;
  }

  if (dev_access_.size() < 2) {
    LOG_INFO << "WARNING:  Exiting as device memory atomic access not enabled";
    dev_access_.clear();
    SUCCEED();
    return;
  }

  kernel_name_ = std::get<0>(GetParam());
  memory_type_ = std::get<1>(GetParam());

  LOG_INFO << "Testing atomic access";

  run_test(1, false, ATOMIC);

  int *dev0_int = (static_cast<int *>(dev_access_[0].shared_mem));
  LOG_INFO << "OUTPUT = " << dev0_int[0];
  EXPECT_EQ(dev_access_[0].init_val + dev_access_[0].group_count_x *
                                          dev_access_[0].group_size_x *
                                          dev_access_[0].kernel_add_val,
            dev0_int[0]);
  dev_access_.clear();
  lzt::free_memory(dev_access_[0].shared_mem);
}

INSTANTIATE_TEST_CASE_P(
    TestP2PAtomicAccess, zeP2PKernelTestsAtomicAccess,
    testing::Combine(testing::Values("atomic_access"),
                     testing::Values(ZE_MEMORY_TYPE_DEVICE,
                                     ZE_MEMORY_TYPE_SHARED)));

class zeP2PKernelTestsConcurrentAccess : public zeP2PKernelTests {};

TEST_P(zeP2PKernelTestsConcurrentAccess,
       GivenP2PDevicesWhenConcurrentAccessesOfPeerMemoryThenSuccessIsReturned) {
  if (skip) {
    return;
  }
  kernel_name_ = std::get<0>(GetParam());
  memory_type_ = std::get<1>(GetParam());
  uint32_t base_index = 0;
  uint32_t concurrent_index = 0;
  // Search for compatible access
  dev_access_.clear();
  for (uint32_t i = 0; i < dev_access_scan_.size(); i++) {
    for (uint32_t j = i + 1; j < dev_access_scan_.size(); j++) {
      if ((lzt::can_access_peer(dev_access_scan_[j].dev,
                                dev_access_scan_[i].dev) &&
           (dev_access_scan_[i]
                .dev_mem_access_properties.deviceAllocCapabilities &
            ZE_MEMORY_CONCURRENT_ACCESS == ZE_MEMORY_CONCURRENT_ACCESS))) {
        dev_access_.push_back(dev_access_scan_[j]);
        dev_access_.push_back(dev_access_scan_[i]);
        concurrent_index = i;
        base_index = j;
        break;
      }
    }
    if (dev_access_.size() >= 2) {
      break;
    }
  }
  if (dev_access_.size() < 2) {
    LOG_INFO
        << "WARNING:  Exiting as device memory concurrent access not enabled";
    SUCCEED();
    return;
  }

  // Find additional devices, if available
  for (uint32_t i = 0; i < dev_access_scan_.size(); i++) {
    if ((i == base_index) || (i == concurrent_index)) {
      continue;
    }
    if (lzt::can_access_peer(dev_access_scan_[i].dev,
                             dev_access_scan_[concurrent_index].dev)) {
      dev_access_.insert(dev_access_.begin(), dev_access_scan_[i]);
    }
  }

  for (uint32_t num_concurrent = 2; num_concurrent <= dev_access_.size();
       num_concurrent++) {
    LOG_INFO << "Testing " << num_concurrent << " concurrent device access";

    run_test(num_concurrent, true, CONCURRENT);
    int *dev0_int = (static_cast<int *>(dev_access_[0].shared_mem));
    for (uint32_t i = 0; i < num_concurrent; i++) {
      LOG_INFO << "OUTPUT[" << i << "] = " << dev0_int[i];
      EXPECT_EQ(dev_access_[i].init_val + dev_access_[i].group_count_x *
                                              dev_access_[i].kernel_add_val,
                dev0_int[i]);
    }
    lzt::free_memory(dev_access_[0].shared_mem);
  }
}

INSTANTIATE_TEST_CASE_P(
    TestP2PConcurrentAccess, zeP2PKernelTestsConcurrentAccess,
    testing::Combine(testing::Values("concurrent_access"),
                     testing::Values(ZE_MEMORY_TYPE_DEVICE,
                                     ZE_MEMORY_TYPE_SHARED)));

class zeP2PKernelTestsAtomicAndConcurrentAccess : public zeP2PKernelTests {};

TEST_P(
    zeP2PKernelTestsAtomicAndConcurrentAccess,
    GivenP2PDevicesWhenAtomicAndConcurrentAccessesOfPeerMemoryThenSuccessIsReturned) {
  if (skip) {
    return;
  }
  ze_device_p2p_properties_t dev_p2p_properties;

  kernel_name_ = std::get<0>(GetParam());
  memory_type_ = std::get<1>(GetParam());
  uint32_t base_index = 0;
  uint32_t concurrent_index = 0;
  // Search for compatible access
  dev_access_.clear();
  for (uint32_t i = 0; i < dev_access_scan_.size(); i++) {
    for (uint32_t j = i + 1; j < dev_access_scan_.size(); j++) {
      dev_p2p_properties = lzt::get_p2p_properties(dev_access_scan_[j].dev,
                                                   dev_access_scan_[i].dev);
      if ((dev_p2p_properties.accessSupported == true) &&
          (dev_p2p_properties.atomicsSupported == true) &&
          (dev_access_scan_[i]
               .dev_mem_access_properties.deviceAllocCapabilities &
           ZE_MEMORY_CONCURRENT_ATOMIC_ACCESS ==
               ZE_MEMORY_CONCURRENT_ATOMIC_ACCESS)) {
        dev_access_.push_back(dev_access_scan_[j]);
        dev_access_.push_back(dev_access_scan_[i]);
        concurrent_index = i;
        base_index = j;
        break;
      }
    }
    if (dev_access_.size() >= 2) {
      break;
    }
  }
  if (dev_access_.size() < 2) {
    LOG_INFO << "WARNING:  Exiting as device memory concurrent atomic access "
                "not enabled";
    SUCCEED();
    return;
  }

  // Find additional devices, if available
  for (uint32_t i = 0; i < dev_access_scan_.size(); i++) {
    if ((i == base_index) || (i == concurrent_index)) {
      continue;
    }
    dev_p2p_properties = lzt::get_p2p_properties(
        dev_access_scan_[i].dev, dev_access_scan_[concurrent_index].dev);
    if ((dev_p2p_properties.accessSupported == true) &&
        (dev_p2p_properties.atomicsSupported == true)) {
      dev_access_.insert(dev_access_.begin(), dev_access_scan_[i]);
    }
  }

  for (uint32_t num_concurrent = 2; num_concurrent <= dev_access_.size();
       num_concurrent++) {
    LOG_INFO << "Testing " << num_concurrent
             << " atomic concurrent device access";

    run_test(num_concurrent, true, CONCURRENT_ATOMIC);
    int *dev0_int = (static_cast<int *>(dev_access_[0].shared_mem));
    // Concurrent Atomic test has all devices writing to same dev mem integer

    LOG_INFO << "OUTPUT[0] = " << dev0_int[0];
    EXPECT_EQ(num_concurrent * dev_access_[0].group_count_x *
                  dev_access_[0].group_size_x * dev_access_[0].kernel_add_val,
              dev0_int[0]);

    lzt::free_memory(dev_access_[0].shared_mem);
  }
}

INSTANTIATE_TEST_CASE_P(
    TestP2PAtomicAndConcurrentAccess, zeP2PKernelTestsAtomicAndConcurrentAccess,
    testing::Combine(testing::Values("atomic_access"),
                     testing::Values(ZE_MEMORY_TYPE_DEVICE,
                                     ZE_MEMORY_TYPE_SHARED)));
} // namespace
