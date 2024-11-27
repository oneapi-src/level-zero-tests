/*
 *
 * Copyright (C) 2019-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"
#include "../headers/test_p2p_common.hpp"

namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>

namespace {
enum MemAccessTestType { ATOMIC, CONCURRENT, CONCURRENT_ATOMIC };

class zeP2PMemAccessTests
    : public lzt::zeEventPoolTests,
      public ::testing::WithParamInterface<
          std::tuple<std::string, lzt_p2p_memory_type_tests_t, int>> {
protected:
  void SetUp() override {

    ze_bool_t can_access;

    auto drivers = lzt::get_all_driver_handles();
    ASSERT_GE(drivers.size(), 1);

    std::vector<ze_device_handle_t> devices;
    std::vector<ze_device_handle_t> sub_devices;
    std::vector<ze_device_handle_t> device_scan;
    ze_driver_handle_t driver = drivers[0];
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
      if (devices.size() >= 2) {
        // use this driver
        break;
      } else {
        // check next driver
        devices.clear();
      }
    }

    if (devices.size() < 2) {
      skip = true;
      GTEST_SKIP() << "WARNING:  Exiting test due to lack of multiple devices";
    }
    get_devices_ = devices.size();
    LOG_INFO << "Detected " << get_devices_ << " devices";

    ze_device_compute_properties_t dev_compute_properties = {
        ZE_STRUCTURE_TYPE_DEVICE_COMPUTE_PROPERTIES, nullptr};

    for (auto device : devices) {
      DevAccess instance = {};
      instance.dev = device;
      instance.dev_grp = driver;
      instance.device_mem_local = nullptr;
      instance.device_mem_remote = nullptr;
      instance.device_mem_validation = nullptr;
      instance.cmd_list = nullptr;
      instance.cmd_q = nullptr;
      instance.module = nullptr;
      instance.function = nullptr;
      instance.init_val = 0;
      instance.kernel_add_val = 0;
      dev_compute_properties.stype =
          ZE_STRUCTURE_TYPE_DEVICE_COMPUTE_PROPERTIES;
      EXPECT_EQ(ZE_RESULT_SUCCESS,
                zeDeviceGetComputeProperties(device, &dev_compute_properties));
      instance.group_size_x = std::min(
          static_cast<uint32_t>(128), dev_compute_properties.maxTotalGroupSize);
      instance.group_count_x = 1;
      instance.dev_mem_properties.pNext = nullptr;
      instance.dev_mem_properties.stype =
          ZE_STRUCTURE_TYPE_DEVICE_MEMORY_PROPERTIES;
      uint32_t num_mem_properties = 1;
      EXPECT_EQ(ZE_RESULT_SUCCESS,
                zeDeviceGetMemoryProperties(device, &num_mem_properties,
                                            &instance.dev_mem_properties));
      instance.dev_mem_access_properties.stype =
          ZE_STRUCTURE_TYPE_DEVICE_MEMORY_ACCESS_PROPERTIES;

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

    ze_kernel_desc_t function_description = {};
    function_description.stype = ZE_STRUCTURE_TYPE_KERNEL_DESC;

    function_description.pNext = nullptr;
    function_description.flags = 0;
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
                              (int *)dev_access_[i].device_mem_local,
                              p_init_val, sizeof(int), nullptr);
    } else if (type != CONCURRENT_ATOMIC) {
      lzt::append_memory_copy(dev_access_[i].cmd_list,
                              (int *)dev_access_[i].device_mem_remote,
                              p_init_val, sizeof(int), nullptr);
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
          dev_access_[i].cmd_list, dev_access_[i].device_mem_validation,
          dev_access_[i].device_mem_remote, num * sizeof(int), nullptr);

      EXPECT_EQ(ZE_RESULT_SUCCESS,
                zeCommandListAppendBarrier(dev_access_[i].cmd_list, nullptr, 0,
                                           nullptr));

      lzt::append_memory_copy(dev_access_[i].cmd_list, host_mem_validation,
                              dev_access_[i].device_mem_validation,
                              num * sizeof(int), nullptr);
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

    mem_size_ = (num * sizeof(int));

    if (p2p_memory_ == LZT_P2P_MEMORY_TYPE_DEVICE) {
      dev_access_[0].device_mem_remote = lzt::allocate_device_memory(
          mem_size_, 1, 0,
          dev_access_[std::max(static_cast<uint32_t>(1), (num - 1))].dev,
          lzt::get_default_context());
    } else if (p2p_memory_ == LZT_P2P_MEMORY_TYPE_SHARED) {
      dev_access_[0].device_mem_remote = lzt::allocate_shared_memory(
          mem_size_, 1, 0, 0,
          dev_access_[std::max(static_cast<uint32_t>(1), (num - 1))].dev);
    } else if (p2p_memory_ == LZT_P2P_MEMORY_TYPE_MEMORY_RESERVATION) {
      size_t pageSize = 0;
      lzt::query_page_size(
          lzt::get_default_context(),
          dev_access_[std::max(static_cast<uint32_t>(1), (num - 1))].dev,
          mem_size_, &pageSize);
      mem_size_ = lzt::create_page_aligned_size(mem_size_, pageSize);
      lzt::physical_device_memory_allocation(
          lzt::get_default_context(),
          dev_access_[std::max(static_cast<uint32_t>(1), (num - 1))].dev,
          mem_size_, &dev_access_[0].device_mem_remote_physical);
      lzt::virtual_memory_reservation(lzt::get_default_context(), nullptr,
                                      mem_size_,
                                      &dev_access_[0].device_mem_remote);
      EXPECT_NE(nullptr, dev_access_[0].device_mem_remote);

      ASSERT_EQ(zeVirtualMemMap(lzt::get_default_context(),
                                dev_access_[0].device_mem_remote, mem_size_,
                                dev_access_[0].device_mem_remote_physical, 0,
                                ZE_MEMORY_ACCESS_ATTRIBUTE_READWRITE),
                ZE_RESULT_SUCCESS);
    } else {
      FAIL() << "Unexpected memory type";
    }
    dev_access_[0].device_mem_validation = lzt::allocate_device_memory(
        mem_size_, 1, 0, dev_access_[0].dev, lzt::get_default_context());
    host_mem_validation = lzt::allocate_host_memory(mem_size_, 1);
    dev_access_[0].module = lzt::create_module(dev_access_[0].dev, module_path);
    if (type == CONCURRENT) {
      dev_access_[0].group_size_x = 1;
      dev_access_[0].group_count_x = 1;
    }
    dev_access_[0].function = create_function(
        dev_access_[0].module, kernel_name_, dev_access_[0].group_size_x,
        (int *)dev_access_[0].device_mem_remote, dev_access_[0].kernel_add_val);
    setup_cmd_list(0, num, sync, event_sync_start, type, events_sync_end);
    dev_access_[0].cmd_q = lzt::create_command_queue(
        dev_access_[0].dev, static_cast<ze_command_queue_flag_t>(0),
        ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS, ZE_COMMAND_QUEUE_PRIORITY_NORMAL,
        0);
    for (uint32_t i = 1; i < num; i++) {
      dev_access_[i].init_val = (1 + i) * dev_access_[0].init_val;
      dev_access_[i].kernel_add_val = (1 + i) * dev_access_[0].kernel_add_val;
      dev_access_[i].module =
          lzt::create_module(dev_access_[i].dev, module_path);
      uint8_t *char_src =
          static_cast<uint8_t *>(dev_access_[0].device_mem_remote);
      if ((type == CONCURRENT) || (type == CONCURRENT_ATOMIC)) {
        char_src += (i * sizeof(int));
        if (type == CONCURRENT) {
          dev_access_[i].group_size_x = 1;
          dev_access_[i].group_count_x = dev_access_[0].group_count_x;
        }
      }
      if (i < num - 1) {
        dev_access_[i].device_mem_remote =
            static_cast<void *>(static_cast<uint8_t *>(char_src));
        dev_access_[i].function = create_function(
            dev_access_[i].module, kernel_name_, dev_access_[i].group_size_x,
            static_cast<int *>(dev_access_[i].device_mem_remote),
            dev_access_[i].kernel_add_val);
      } else {
        dev_access_[i].device_mem_local =
            static_cast<void *>(static_cast<uint8_t *>(char_src));
        dev_access_[i].function = create_function(
            dev_access_[i].module, kernel_name_, dev_access_[i].group_size_x,
            static_cast<int *>(dev_access_[i].device_mem_local),
            dev_access_[i].kernel_add_val);
      }

      setup_cmd_list(i, num, sync, event_sync_start, type, events_sync_end);
      dev_access_[i].cmd_q = lzt::create_command_queue(
          dev_access_[i].dev, static_cast<ze_command_queue_flag_t>(0),
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
                zeCommandQueueSynchronize(dev_access_[i].cmd_q, UINT64_MAX));
      EXPECT_EQ(ZE_RESULT_SUCCESS, zeCommandListReset(dev_access_[i].cmd_list));
    }

    if (sync) {
      ep.destroy_event(event_sync_start);
      ep.destroy_events(events_sync_end);
    }
    if (p2p_memory_ == LZT_P2P_MEMORY_TYPE_MEMORY_RESERVATION) {
      lzt::virtual_memory_unmap(lzt::get_default_context(),
                                dev_access_[0].device_mem_remote, mem_size_);
      lzt::physical_memory_destroy(lzt::get_default_context(),
                                   dev_access_[0].device_mem_remote_physical);
      lzt::virtual_memory_free(lzt::get_default_context(),
                               dev_access_[0].device_mem_remote, mem_size_);
    } else {
      lzt::free_memory(dev_access_[0].device_mem_remote);
    }

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
    ze_physical_mem_handle_t device_mem_remote_physical;
    void *device_mem_validation;
    int init_val;
    int kernel_add_val;
    uint32_t group_size_x;
    uint32_t group_count_x;
    ze_command_list_handle_t cmd_list;
    ze_command_queue_handle_t cmd_q;
    ze_module_handle_t module;
    ze_kernel_handle_t function;
    ze_device_memory_properties_t dev_mem_properties = {
        ZE_STRUCTURE_TYPE_DEVICE_MEMORY_PROPERTIES, nullptr};
    ze_device_memory_access_properties_t dev_mem_access_properties = {
        ZE_STRUCTURE_TYPE_DEVICE_MEMORY_ACCESS_PROPERTIES, nullptr};
  } DevAccess_t;

  std::vector<DevAccess> dev_access_;
  std::vector<DevAccess> dev_access_scan_;
  std::string kernel_name_;
  void *host_mem_validation;
  uint32_t get_devices_;
  size_t mem_size_;
  lzt_p2p_memory_type_tests_t p2p_memory_;
  bool skip = false;
};

class zeP2PMemAccessTestsAtomicAccess : public zeP2PMemAccessTests {};

TEST_P(
    zeP2PMemAccessTestsAtomicAccess,
    GivenMultipleDevicesWithP2PWhenAtomicAccessOfPeerMemoryThenSuccessIsReturned) {
  if (skip) {
    return;
  }
  ze_device_p2p_properties_t dev_p2p_properties = {
      ZE_STRUCTURE_TYPE_DEVICE_P2P_PROPERTIES, nullptr};

  // Search for compatible access
  dev_access_.clear();
  for (uint32_t i = 0; i < dev_access_scan_.size(); i++) {
    for (uint32_t j = i + 1; j < dev_access_scan_.size(); j++) {
      dev_p2p_properties = lzt::get_p2p_properties(dev_access_scan_[i].dev,
                                                   dev_access_scan_[j].dev);
      if ((dev_p2p_properties.flags & ZE_DEVICE_P2P_PROPERTY_FLAG_ACCESS) &&
          (dev_p2p_properties.flags & ZE_DEVICE_P2P_PROPERTY_FLAG_ATOMICS)) {
        if (lzt::can_access_peer(dev_access_scan_[i].dev,
                                 dev_access_scan_[j].dev)) {
          dev_access_.push_back(dev_access_scan_[i]);
          dev_access_.push_back(dev_access_scan_[j]);
          break;
        }
      }
      dev_p2p_properties = lzt::get_p2p_properties(dev_access_scan_[j].dev,
                                                   dev_access_scan_[i].dev);
      if ((dev_p2p_properties.flags & ZE_DEVICE_P2P_PROPERTY_FLAG_ACCESS) &&
          (dev_p2p_properties.flags & ZE_DEVICE_P2P_PROPERTY_FLAG_ATOMICS)) {
        if (lzt::can_access_peer(dev_access_scan_[j].dev,
                                 dev_access_scan_[i].dev)) {
          dev_access_.push_back(dev_access_scan_[j]);
          dev_access_.push_back(dev_access_scan_[i]);
          break;
        }
      }
    }
    if (dev_access_.size() >= 2)
      break;
  }

  if (dev_access_.size() < 2) {
    LOG_INFO << "WARNING:  Exiting as no peer-to-peer atomic access capability";
    dev_access_.clear();
    GTEST_SKIP();
  }

  kernel_name_ = std::get<0>(GetParam());
  p2p_memory_ = std::get<1>(GetParam());

  LOG_INFO << "Testing atomic access";

  run_test(1, false, ATOMIC);

  int *dev0_int = (static_cast<int *>(host_mem_validation));
  LOG_INFO << "OUTPUT = " << dev0_int[0];
  EXPECT_EQ(dev_access_[0].init_val + dev_access_[0].group_count_x *
                                          dev_access_[0].group_size_x *
                                          dev_access_[0].kernel_add_val,
            dev0_int[0]);
  lzt::free_memory(host_mem_validation);
  lzt::free_memory(dev_access_[0].device_mem_validation);
  dev_access_.clear();
}

INSTANTIATE_TEST_CASE_P(
    TestP2PAtomicAccess, zeP2PMemAccessTestsAtomicAccess,
    testing::Combine(testing::Values("atomic_access"),
                     testing::Values(LZT_P2P_MEMORY_TYPE_DEVICE,
                                     LZT_P2P_MEMORY_TYPE_SHARED,
                                     LZT_P2P_MEMORY_TYPE_MEMORY_RESERVATION),
                     testing::Range(2, 3)));

class zeP2PMemAccessTestsConcurrentAccess : public zeP2PMemAccessTests {};

TEST_P(
    zeP2PMemAccessTestsConcurrentAccess,
    GivenMultipleDevicesWithP2PWhenConcurrentAccessesOfPeerMemoryThenSuccessIsReturned) {
  if (skip) {
    return;
  }
  kernel_name_ = std::get<0>(GetParam());
  p2p_memory_ = std::get<1>(GetParam());
  int num_concurrent_devices = std::get<2>(GetParam());

  uint32_t base_index = 0;
  uint32_t concurrent_index = 0;
  // Search for compatible access
  dev_access_.clear();
  for (uint32_t i = 0; i < dev_access_scan_.size(); i++) {
    for (uint32_t j = i + 1; j < dev_access_scan_.size(); j++) {
      if ((lzt::can_access_peer(dev_access_scan_[j].dev,
                                dev_access_scan_[i].dev))) {
        if (dev_access_scan_[i]
                    .dev_mem_access_properties
                    .sharedCrossDeviceAllocCapabilities &
                ZE_MEMORY_ACCESS_CAP_FLAG_CONCURRENT ||
            p2p_memory_ == LZT_P2P_MEMORY_TYPE_DEVICE) {
          dev_access_.push_back(dev_access_scan_[j]);
          dev_access_.push_back(dev_access_scan_[i]);
          concurrent_index = i;
          base_index = j;
          break;
        }
      }
    }
    if (dev_access_.size() >= 2) {
      break;
    }
  }
  if (dev_access_.size() < 2) {
    GTEST_SKIP()
        << "WARNING:  Exiting as no peer-to-peer concurrent access capability";
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

  if (dev_access_.size() < num_concurrent_devices) {
    GTEST_SKIP() << "WARNING:  Exiting as requested " << num_concurrent_devices
                 << "number of concurrent devices/subdevices not available";
  }

  LOG_INFO << "Testing " << num_concurrent_devices
           << " concurrent device access";

  run_test(num_concurrent_devices, true, CONCURRENT);
  int *dev0_int = (static_cast<int *>(host_mem_validation));
  for (uint32_t i = 0; i < num_concurrent_devices; i++) {
    LOG_INFO << "OUTPUT[" << i << "] = " << dev0_int[i];
    EXPECT_EQ(dev_access_[i].init_val +
                  dev_access_[i].group_count_x * dev_access_[i].kernel_add_val,
              dev0_int[i]);
  }

  lzt::free_memory(host_mem_validation);
  lzt::free_memory(dev_access_[0].device_mem_validation);
}

INSTANTIATE_TEST_CASE_P(
    TestP2PConcurrentAccess, zeP2PMemAccessTestsConcurrentAccess,
    testing::Combine(testing::Values("concurrent_access"),
                     testing::Values(LZT_P2P_MEMORY_TYPE_DEVICE,
                                     LZT_P2P_MEMORY_TYPE_SHARED,
                                     LZT_P2P_MEMORY_TYPE_MEMORY_RESERVATION),
                     testing::Range(2, 20)));

class zeP2PMemAccessTestsAtomicAndConcurrentAccess
    : public zeP2PMemAccessTests {};

TEST_P(
    zeP2PMemAccessTestsAtomicAndConcurrentAccess,
    GivenMultipleDevicesWithP2PWhenAtomicAndConcurrentAccessesOfPeerMemoryThenSuccessIsReturned) {
  if (skip) {
    return;
  }
  ze_device_p2p_properties_t dev_p2p_properties = {
      ZE_STRUCTURE_TYPE_DEVICE_P2P_PROPERTIES, nullptr};

  kernel_name_ = std::get<0>(GetParam());
  p2p_memory_ = std::get<1>(GetParam());
  int num_concurrent_devices = std::get<2>(GetParam());

  uint32_t base_index = 0;
  uint32_t concurrent_index = 0;
  // Search for compatible access
  dev_access_.clear();
  for (uint32_t i = 0; i < dev_access_scan_.size(); i++) {
    for (uint32_t j = i + 1; j < dev_access_scan_.size(); j++) {
      dev_p2p_properties = lzt::get_p2p_properties(dev_access_scan_[j].dev,
                                                   dev_access_scan_[i].dev);
      if ((lzt::can_access_peer(dev_access_scan_[j].dev,
                                dev_access_scan_[i].dev)) &&
          (dev_p2p_properties.flags & ZE_DEVICE_P2P_PROPERTY_FLAG_ATOMICS) &&
          ((dev_access_scan_[i]
                .dev_mem_access_properties.sharedCrossDeviceAllocCapabilities &
            ZE_MEMORY_ACCESS_CAP_FLAG_CONCURRENT_ATOMIC) ||
           p2p_memory_ == LZT_P2P_MEMORY_TYPE_DEVICE)) {
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
    GTEST_SKIP()
        << "WARNING:  Exiting as no peer-to-peer concurrent-atomic access "
           "capability";
  }

  // Find additional devices, if available
  for (uint32_t i = 0; i < dev_access_scan_.size(); i++) {
    if ((i == base_index) || (i == concurrent_index)) {
      continue;
    }
    dev_p2p_properties = lzt::get_p2p_properties(
        dev_access_scan_[i].dev, dev_access_scan_[concurrent_index].dev);
    if ((lzt::can_access_peer(dev_access_scan_[i].dev,
                              dev_access_scan_[concurrent_index].dev)) &&
        (dev_p2p_properties.flags & ZE_DEVICE_P2P_PROPERTY_FLAG_ATOMICS)) {
      dev_access_.insert(dev_access_.begin(), dev_access_scan_[i]);
    }
  }

  if (dev_access_.size() < num_concurrent_devices) {
    LOG_INFO << "WARNING:  Exiting as requested " << num_concurrent_devices
             << "number of concurrent-atomic devices/subdevices not available";
    GTEST_SKIP();
  }

  LOG_INFO << "Testing " << num_concurrent_devices
           << " atomic concurrent device access";

  run_test(num_concurrent_devices, true, CONCURRENT_ATOMIC);
  int *dev0_int = (static_cast<int *>(host_mem_validation));
  for (uint32_t i = 0; i < num_concurrent_devices; i++) {
    LOG_INFO << "OUTPUT[" << i << "] = " << dev0_int[i];
    EXPECT_EQ(dev_access_[i].init_val + dev_access_[i].group_count_x *
                                            dev_access_[i].group_size_x *
                                            dev_access_[i].kernel_add_val,
              dev0_int[i]);
  }

  lzt::free_memory(host_mem_validation);
  lzt::free_memory(dev_access_[0].device_mem_validation);
}

INSTANTIATE_TEST_CASE_P(
    TestP2PAtomicAndConcurrentAccess,
    zeP2PMemAccessTestsAtomicAndConcurrentAccess,
    testing::Combine(testing::Values("atomic_access"),
                     testing::Values(LZT_P2P_MEMORY_TYPE_DEVICE,
                                     LZT_P2P_MEMORY_TYPE_SHARED,
                                     LZT_P2P_MEMORY_TYPE_MEMORY_RESERVATION),
                     testing::Range(2, 20)));
} // namespace
