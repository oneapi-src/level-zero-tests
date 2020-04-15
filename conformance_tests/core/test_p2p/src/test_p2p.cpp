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
enum MemAccessTestType { ATOMIC, CONCURRENT, CONCURRENT_ATOMIC };

class zeP2PTests : public ::testing::Test,
                   public ::testing::WithParamInterface<ze_memory_type_t> {
protected:
  void SetUp() override {
    ze_memory_type_t memory_type = GetParam();
    ze_bool_t can_access;
    auto drivers = lzt::get_all_driver_handles();
    ASSERT_GT(drivers.size(), 0);
    auto devices = lzt::get_ze_devices(drivers[0]);
    ASSERT_GE(devices.size(), 2);

    for (auto dev1 : devices) {
      for (auto dev2 : devices) {
        if (dev1 == dev2)
          continue;
        EXPECT_EQ(ZE_RESULT_SUCCESS,
                  zeDeviceCanAccessPeer(dev1, dev2, &can_access));
        if (can_access == false) {
          LOG_INFO << "WARNING:  Exiting as device peer access not enabled";
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
};

TEST_P(
    zeP2PTests,
    GivenP2PDevicesWhenCopyingDeviceMemoryToAndFromRemoteDeviceThenSuccessIsReturned) {

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

class zeP2PKernelTests : public lzt::zeEventPoolTests,
                         public ::testing::WithParamInterface<
                             std::tuple<std::string, ze_memory_type_t>> {
protected:
  void SetUp() override {

    ze_bool_t can_access;

    auto drivers = lzt::get_all_driver_handles();
    ASSERT_GE(drivers.size(), 1);

    std::vector<ze_device_handle_t> devices;
    ze_driver_handle_t driver;
    for (auto dg : drivers) {
      driver = dg;
      devices = lzt::get_devices(driver);

      if (devices.size() >= 2)
        break;
    }

    if (devices.size() < 2) {
      LOG_INFO << "WARNING:  Exiting test due to lack of multiple devices";
      return;
    }
    get_devices_ = devices.size();
    LOG_INFO << "Detected " << get_devices_ << " devices";

    for (auto dev1 : devices) {
      for (auto dev2 : devices) {
        if (dev1 == dev2)
          continue;
        EXPECT_EQ(ZE_RESULT_SUCCESS,
                  zeDeviceCanAccessPeer(dev1, dev2, &can_access));
        if (can_access == false) {
          LOG_INFO << "WARNING:  Exiting as device peer access not enabled";
          return;
        }
      }
    }

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

      dev_access_.push_back(instance);
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
    thread_group_dimensions.groupCountX = 1;
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
    memset(dev_access_[0].shared_mem, 0, num * sizeof(int));
    dev_access_[0].module = lzt::create_module(dev_access_[0].dev, module_path);
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
    }

    if (sync) {
      ep.destroy_event(event_sync_start);
      ep.destroy_events(events_sync_end);
    }
    lzt::free_memory(dev_access_[0].device_mem_remote);
    lzt::free_memory(dev_access_[0].shared_mem);

    for (uint32_t i = 0; i < num; i++) {
      lzt::destroy_command_queue(dev_access_[i].cmd_q);
      EXPECT_EQ(ZE_RESULT_SUCCESS,
                zeCommandListDestroy(dev_access_[i].cmd_list));
      EXPECT_EQ(ZE_RESULT_SUCCESS, zeModuleDestroy(dev_access_[i].module));
      EXPECT_EQ(ZE_RESULT_SUCCESS, zeKernelDestroy(dev_access_[i].function));
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
    ze_command_list_handle_t cmd_list;
    ze_command_queue_handle_t cmd_q;
    ze_module_handle_t module;
    ze_kernel_handle_t function;
    ze_device_memory_properties_t dev_mem_properties;
    ze_device_memory_access_properties_t dev_mem_access_properties;
  } DevAccess_t;

  std::vector<DevAccess> dev_access_;
  std::string kernel_name_;
  uint32_t get_devices_;
  ze_memory_type_t memory_type_;
};

class zeP2PKernelTestsAtomicAccess : public zeP2PKernelTests {};

TEST_P(zeP2PKernelTestsAtomicAccess,
       GivenP2PDevicesWhenAtomicAccessOfPeerMemoryThenSuccessIsReturned) {

  ze_device_p2p_properties_t dev_p2p_properties;
  dev_p2p_properties.version = ZE_DEVICE_P2P_PROPERTIES_VERSION_CURRENT;

  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeDeviceGetP2PProperties(dev_access_[0].dev, dev_access_[1].dev,
                                     &dev_p2p_properties));

  if (dev_p2p_properties.atomicsSupported == false) {
    LOG_INFO << "WARNING:  Exiting as atomics not supported";
    return;
  }

  if (dev_access_[1].dev_mem_access_properties.deviceAllocCapabilities &
      ZE_MEMORY_ATOMIC_ACCESS != ZE_MEMORY_ATOMIC_ACCESS) {
    LOG_INFO << "WARNING:  Exiting as device memory atomic access not enabled";
    return;
  }

  kernel_name_ = std::get<0>(GetParam());
  memory_type_ = std::get<1>(GetParam());
  run_test(1, false, ATOMIC);

  int *dev0_int = (static_cast<int *>(dev_access_[0].shared_mem));
  LOG_INFO << "OUTPUT = " << dev0_int[0];
  EXPECT_EQ(dev_access_[0].init_val +
                dev_access_[0].group_size_x * dev_access_[0].kernel_add_val,
            dev0_int[0]);
}

INSTANTIATE_TEST_CASE_P(
    TestP2PAtomicAccess, zeP2PKernelTestsAtomicAccess,
    testing::Combine(testing::Values("atomic_access"),
                     testing::Values(ZE_MEMORY_TYPE_DEVICE,
                                     ZE_MEMORY_TYPE_SHARED)));

class zeP2PKernelTestsConcurrentAccess : public zeP2PKernelTests {};

TEST_P(zeP2PKernelTestsConcurrentAccess,
       GivenP2PDevicesWhenConcurrentAccessesOfPeerMemoryThenSuccessIsReturned) {

  kernel_name_ = std::get<0>(GetParam());
  memory_type_ = std::get<1>(GetParam());
  for (uint32_t num_concurrent = 2; num_concurrent <= get_devices_;
       num_concurrent++) {
    LOG_INFO << "Testing " << num_concurrent << " concurrent device access";
    if (dev_access_[num_concurrent - 1]
            .dev_mem_access_properties.deviceAllocCapabilities &
        ZE_MEMORY_CONCURRENT_ACCESS != ZE_MEMORY_CONCURRENT_ACCESS) {
      LOG_INFO
          << "WARNING:  Exiting as device memory concurrent access not enabled";
      return;
    }
    run_test(num_concurrent, true, CONCURRENT);
    int *dev0_int = (static_cast<int *>(dev_access_[0].shared_mem));
    for (uint32_t i = 0; i < num_concurrent; i++) {
      LOG_INFO << "OUTPUT[" << i << "] = " << dev0_int[i];
      EXPECT_EQ(dev_access_[i].init_val + dev_access_[i].kernel_add_val,
                dev0_int[i]);
    }
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

  ze_device_p2p_properties_t dev_p2p_properties;
  dev_p2p_properties.version = ZE_DEVICE_P2P_PROPERTIES_VERSION_CURRENT;

  kernel_name_ = std::get<0>(GetParam());
  memory_type_ = std::get<1>(GetParam());
  for (uint32_t num_concurrent = 2; num_concurrent <= get_devices_;
       num_concurrent++) {
    LOG_INFO << "Testing " << num_concurrent
             << " atomic concurrent device access";
    for (uint32_t i = 0; i < num_concurrent - 1; i++) {
      EXPECT_EQ(ZE_RESULT_SUCCESS,
                zeDeviceGetP2PProperties(dev_access_[i].dev,
                                         dev_access_[num_concurrent - 1].dev,
                                         &dev_p2p_properties));

      if (dev_p2p_properties.atomicsSupported == false) {
        LOG_INFO << "WARNING:  Exiting as atomics not supported";
        return;
      }
    }

    if (dev_access_[num_concurrent - 1]
            .dev_mem_access_properties.deviceAllocCapabilities &
        ZE_MEMORY_CONCURRENT_ATOMIC_ACCESS !=
            ZE_MEMORY_CONCURRENT_ATOMIC_ACCESS) {
      LOG_INFO << "WARNING: Exiting as device memory concurrent atomic access "
                  "not enabled";
      return;
    }
    run_test(num_concurrent, true, CONCURRENT_ATOMIC);
    int *dev0_int = (static_cast<int *>(dev_access_[0].shared_mem));
    // Concurrent Atomic test has all devices writing to same dev mem integer
    // All values read back to shared mem should be identical
    for (uint32_t i = 0; i < num_concurrent; i++) {
      LOG_INFO << "OUTPUT[" << i << "] = " << dev0_int[i];
      EXPECT_EQ(num_concurrent * dev_access_[i].group_size_x *
                    dev_access_[i].kernel_add_val,
                dev0_int[i]);
    }
  }
}

INSTANTIATE_TEST_CASE_P(
    TestP2PAtomicAndConcurrentAccess, zeP2PKernelTestsAtomicAndConcurrentAccess,
    testing::Combine(testing::Values("atomic_access"),
                     testing::Values(ZE_MEMORY_TYPE_DEVICE,
                                     ZE_MEMORY_TYPE_SHARED)));

} // namespace
