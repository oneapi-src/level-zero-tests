/*
 *
 * Copyright (C) 2019-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <array>

#include "gtest/gtest.h"
#include <cstdint>

#include "utils/utils.hpp"
#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "random/random.hpp"
#include "logging/logging.hpp"
#include "../headers/test_p2p_common.hpp"

namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>

namespace {

class zeP2PTests : public ::testing::Test,
                   public ::testing::WithParamInterface<
                       std::tuple<lzt_p2p_memory_type_tests_t, size_t, bool>> {
protected:
  void SetUp() override {
    p2p_memory_ = std::get<0>(GetParam());
    offset_ = std::get<1>(GetParam());
    is_immediate_ = std::get<2>(GetParam());
    ze_bool_t can_access;
    auto driver = lzt::get_default_driver();
    auto context = lzt::get_default_context();
    auto devices = lzt::get_ze_devices(driver);

    for (auto device : devices) {
      DevInstance instance;

      instance.dev = device;
      instance.dev_grp = driver;
      if (p2p_memory_ == LZT_P2P_MEMORY_TYPE_DEVICE) {
        instance.src_region = lzt::allocate_device_memory(
            mem_size_ + offset_, 1, 0, device, context);
        instance.dst_region = lzt::allocate_device_memory(
            mem_size_ + offset_, 1, 0, device, context);
      } else if (p2p_memory_ == LZT_P2P_MEMORY_TYPE_SHARED) {
        instance.src_region =
            lzt::allocate_shared_memory(mem_size_ + offset_, 1, 0, 0, device);
        instance.dst_region =
            lzt::allocate_shared_memory(mem_size_ + offset_, 1, 0, 0, device);
      } else if (p2p_memory_ == LZT_P2P_MEMORY_TYPE_MEMORY_RESERVATION) {
        size_t pageSize = 0;
        lzt::query_page_size(context, device, mem_size_ + offset_, &pageSize);
        mem_size_ =
            lzt::create_page_aligned_size(mem_size_ + offset_, pageSize);
        offset_ = 0;
        lzt::physical_device_memory_allocation(context, device, mem_size_,
                                               &instance.src_physical_region);
        lzt::physical_device_memory_allocation(context, device, mem_size_,
                                               &instance.dst_physical_region);
        lzt::virtual_memory_reservation(context, nullptr, mem_size_,
                                        &instance.src_region);
        EXPECT_NE(nullptr, instance.src_region);
        lzt::virtual_memory_reservation(context, nullptr, mem_size_,
                                        &instance.dst_region);
        EXPECT_NE(nullptr, instance.dst_region);

        ASSERT_ZE_RESULT_SUCCESS(
            zeVirtualMemMap(context, instance.src_region, mem_size_,
                            instance.src_physical_region, 0,
                            ZE_MEMORY_ACCESS_ATTRIBUTE_READWRITE));
        ASSERT_ZE_RESULT_SUCCESS(
            zeVirtualMemMap(context, instance.dst_region, mem_size_,
                            instance.dst_physical_region, 0,
                            ZE_MEMORY_ACCESS_ATTRIBUTE_READWRITE));
      } else {
        FAIL() << "Unexpected memory type";
      }

      instance.cmd_bundle = lzt::create_command_bundle(device, is_immediate_);

      for (auto sub_device : lzt::get_ze_sub_devices(device)) {
        DevInstance sub_device_instance;
        sub_device_instance.dev = sub_device;
        sub_device_instance.dev_grp = instance.dev_grp;
        if (p2p_memory_ == LZT_P2P_MEMORY_TYPE_DEVICE) {
          sub_device_instance.src_region = lzt::allocate_device_memory(
              mem_size_ + offset_, 1, 0, sub_device, context);
          sub_device_instance.dst_region = lzt::allocate_device_memory(
              mem_size_ + offset_, 1, 0, sub_device, context);

        } else if (p2p_memory_ == LZT_P2P_MEMORY_TYPE_SHARED) {
          sub_device_instance.src_region = lzt::allocate_shared_memory(
              mem_size_ + offset_, 1, 0, 0, sub_device);
          sub_device_instance.dst_region = lzt::allocate_shared_memory(
              mem_size_ + offset_, 1, 0, 0, sub_device);
        } else if (p2p_memory_ == LZT_P2P_MEMORY_TYPE_MEMORY_RESERVATION) {
          size_t pageSize = 0;
          lzt::query_page_size(context, sub_device, mem_size_ + offset_,
                               &pageSize);
          mem_size_ =
              lzt::create_page_aligned_size(mem_size_ + offset_, pageSize);
          offset_ = 0;
          lzt::physical_device_memory_allocation(
              context, sub_device, mem_size_,
              &sub_device_instance.src_physical_region);
          lzt::physical_device_memory_allocation(
              context, sub_device, mem_size_,
              &sub_device_instance.dst_physical_region);
          lzt::virtual_memory_reservation(context, nullptr, mem_size_,
                                          &sub_device_instance.src_region);
          EXPECT_NE(nullptr, sub_device_instance.src_region);
          lzt::virtual_memory_reservation(context, nullptr, mem_size_,
                                          &sub_device_instance.dst_region);
          EXPECT_NE(nullptr, sub_device_instance.dst_region);

          ASSERT_ZE_RESULT_SUCCESS(zeVirtualMemMap(
              context, sub_device_instance.src_region, mem_size_,
              sub_device_instance.src_physical_region, 0,
              ZE_MEMORY_ACCESS_ATTRIBUTE_READWRITE));
          ASSERT_ZE_RESULT_SUCCESS(zeVirtualMemMap(
              context, sub_device_instance.dst_region, mem_size_,
              sub_device_instance.dst_physical_region, 0,
              ZE_MEMORY_ACCESS_ATTRIBUTE_READWRITE));
        } else {
          FAIL() << "Unexpected memory type";
        }
        sub_device_instance.cmd_bundle =
            lzt::create_command_bundle(sub_device, is_immediate_);
        instance.sub_devices.push_back(sub_device_instance);
      }

      dev_instance_.push_back(instance);
    }
  }

  void TearDown() override {

    for (auto instance : dev_instance_) {
      auto context = lzt::get_default_context();
      lzt::destroy_command_bundle(instance.cmd_bundle);

      if (p2p_memory_ == LZT_P2P_MEMORY_TYPE_MEMORY_RESERVATION) {
        lzt::virtual_memory_unmap(context, instance.src_region, mem_size_);
        lzt::virtual_memory_unmap(context, instance.dst_region, mem_size_);
        lzt::physical_memory_destroy(context, instance.src_physical_region);
        lzt::physical_memory_destroy(context, instance.dst_physical_region);
        lzt::virtual_memory_free(context, instance.src_region, mem_size_);
        lzt::virtual_memory_free(context, instance.dst_region, mem_size_);
      } else {
        lzt::free_memory(instance.src_region);
        lzt::free_memory(instance.dst_region);
      }

      for (auto sub_device_instance : instance.sub_devices) {
        lzt::destroy_command_bundle(sub_device_instance.cmd_bundle);

        if (p2p_memory_ == LZT_P2P_MEMORY_TYPE_MEMORY_RESERVATION) {
          lzt::virtual_memory_unmap(context, sub_device_instance.src_region,
                                    mem_size_);
          lzt::virtual_memory_unmap(context, sub_device_instance.dst_region,
                                    mem_size_);
          lzt::physical_memory_destroy(context,
                                       sub_device_instance.src_physical_region);
          lzt::physical_memory_destroy(context,
                                       sub_device_instance.dst_physical_region);
          lzt::virtual_memory_free(context, sub_device_instance.src_region,
                                   mem_size_);
          lzt::virtual_memory_free(context, sub_device_instance.dst_region,
                                   mem_size_);
        } else {
          lzt::free_memory(sub_device_instance.src_region);
          lzt::free_memory(sub_device_instance.dst_region);
        }
      }
    }
  }

  struct DevInstance {
    ze_device_handle_t dev;
    ze_driver_handle_t dev_grp;
    void *src_region;
    void *dst_region;
    ze_physical_mem_handle_t src_physical_region;
    ze_physical_mem_handle_t dst_physical_region;
    lzt::zeCommandBundle cmd_bundle;
    std::vector<DevInstance> sub_devices;
  };
  const uint32_t columns = 8;
  const uint32_t rows = 8;
  const uint32_t slices = 8;
  size_t mem_size_ = columns * rows * slices;
  size_t offset_;
  std::vector<DevInstance> dev_instance_;
  lzt_p2p_memory_type_tests_t p2p_memory_;
  bool is_immediate_ = false;
};

LZT_TEST_P(
    zeP2PTests,
    GivenMultipleDevicesWithP2PWhenCopyingDeviceMemoryToAndFromRemoteDeviceThenSuccessIsReturned) {

  void *initial_pattern_memory = lzt::allocate_host_memory(mem_size_ + offset_);
  void *verification_memory1 = lzt::allocate_host_memory(mem_size_ + offset_);
  void *verification_memory2 = lzt::allocate_host_memory(mem_size_ + offset_);

  uint8_t *src = static_cast<uint8_t *>(initial_pattern_memory);
  for (uint32_t i = 0; i < mem_size_ + offset_; i++) {
    src[i] = i & 0xff;
  }

  if (dev_instance_.size() < 2) {
    LOG_INFO << "Test cannot be run with less than 2 Devices";
    GTEST_SKIP();
  }

  for (uint32_t i = 1; i < dev_instance_.size(); i++) {
    if (!lzt::can_access_peer(dev_instance_[i - 1].dev, dev_instance_[i].dev)) {
      LOG_INFO << "FAILURE:  Device-to-Device access disabled";
      FAIL();
    }
    if (!lzt::can_access_peer(dev_instance_[i].dev, dev_instance_[i - 1].dev)) {
      LOG_INFO << "FAILURE:  Device-to-Device access disabled";
      FAIL();
    }

    for (uint32_t d = 0; d < 2; d++) {
      size_t src_offset, dst_offset;
      if (d == 0) {
        src_offset = offset_;
        dst_offset = 0;
      } else {
        src_offset = 0;
        dst_offset = offset_;
      }
      lzt::append_memory_copy(dev_instance_[i].cmd_bundle.list,
                              dev_instance_[i].src_region,
                              initial_pattern_memory, mem_size_ + src_offset);
      lzt::append_barrier(dev_instance_[i].cmd_bundle.list, nullptr, 0,
                          nullptr);
      lzt::append_memory_copy(dev_instance_[i - 1].cmd_bundle.list,
                              dev_instance_[i - 1].src_region,
                              initial_pattern_memory, mem_size_ + src_offset);
      lzt::append_barrier(dev_instance_[i - 1].cmd_bundle.list, nullptr, 0,
                          nullptr);
      lzt::append_memory_copy(
          dev_instance_[i].cmd_bundle.list,
          static_cast<void *>(
              static_cast<uint8_t *>(dev_instance_[i - 1].dst_region) +
              dst_offset),
          static_cast<void *>(
              static_cast<uint8_t *>(dev_instance_[i].src_region) + src_offset),
          mem_size_, nullptr);
      lzt::append_barrier(dev_instance_[i].cmd_bundle.list, nullptr, 0,
                          nullptr);
      lzt::append_memory_copy(
          dev_instance_[i - 1].cmd_bundle.list,
          static_cast<void *>(
              static_cast<uint8_t *>(dev_instance_[i].dst_region) + dst_offset),
          static_cast<void *>(
              static_cast<uint8_t *>(dev_instance_[i - 1].src_region) +
              src_offset),
          mem_size_, nullptr);

      lzt::append_barrier(dev_instance_[i - 1].cmd_bundle.list, nullptr, 0,
                          nullptr);
      lzt::append_memory_copy(
          dev_instance_[i].cmd_bundle.list, verification_memory1,
          static_cast<void *>(
              static_cast<uint8_t *>(dev_instance_[i - 1].dst_region) +
              dst_offset),
          mem_size_, nullptr);
      lzt::append_barrier(dev_instance_[i].cmd_bundle.list, nullptr, 0,
                          nullptr);
      lzt::append_memory_copy(
          dev_instance_[i - 1].cmd_bundle.list, verification_memory2,
          static_cast<void *>(
              static_cast<uint8_t *>(dev_instance_[i].dst_region) + dst_offset),
          mem_size_, nullptr);
      lzt::append_barrier(dev_instance_[i - 1].cmd_bundle.list, nullptr, 0,
                          nullptr);

      lzt::close_command_list(dev_instance_[i - 1].cmd_bundle.list);
      lzt::close_command_list(dev_instance_[i].cmd_bundle.list);
      if (is_immediate_) {
        lzt::synchronize_command_list_host(dev_instance_[i - 1].cmd_bundle.list,
                                           UINT64_MAX);
        lzt::synchronize_command_list_host(dev_instance_[i].cmd_bundle.list,
                                           UINT64_MAX);
      } else {
        lzt::execute_command_lists(dev_instance_[i - 1].cmd_bundle.queue, 1,
                                   &dev_instance_[i - 1].cmd_bundle.list,
                                   nullptr);
        lzt::execute_command_lists(dev_instance_[i].cmd_bundle.queue, 1,
                                   &dev_instance_[i].cmd_bundle.list, nullptr);
        lzt::synchronize(dev_instance_[i - 1].cmd_bundle.queue, UINT64_MAX);
        lzt::synchronize(dev_instance_[i].cmd_bundle.queue, UINT64_MAX);
      }
      lzt::reset_command_list(dev_instance_[i - 1].cmd_bundle.list);
      lzt::reset_command_list(dev_instance_[i].cmd_bundle.list);

      uint8_t *src =
          static_cast<uint8_t *>(initial_pattern_memory) + src_offset;
      uint8_t *dst1 = static_cast<uint8_t *>(verification_memory1);
      uint8_t *dst2 = static_cast<uint8_t *>(verification_memory2);
      for (uint32_t i = 0; i < mem_size_; i++) {
        ASSERT_EQ(src[i], dst1[i])
            << "Memory Copied from Device did not match.";
        ASSERT_EQ(src[i], dst2[i])
            << "Memory Copied from Device did not match.";
      }
    }
  }
  lzt::free_memory(verification_memory1);
  lzt::free_memory(verification_memory2);
  lzt::free_memory(initial_pattern_memory);
}

LZT_TEST_P(
    zeP2PTests,
    GivenP2PSubDevicesWhenCopyingDeviceMemoryToAndFromRemoteSubDeviceThenSuccessIsReturned) {

  void *initial_pattern_memory = lzt::allocate_host_memory(mem_size_ + offset_);
  void *verification_memory1 = lzt::allocate_host_memory(mem_size_ + offset_);
  void *verification_memory2 = lzt::allocate_host_memory(mem_size_ + offset_);

  uint8_t *src = static_cast<uint8_t *>(initial_pattern_memory);
  for (size_t i = 0U; i < mem_size_ + offset_; i++) {
    src[i] = i & 0xff;
  }

  for (size_t i = 0U; i < dev_instance_.size(); i++) {
    if (dev_instance_[i].sub_devices.size() < 2) {
      LOG_INFO << "Test cannot be run with less than 2 SubDevices";
      GTEST_SKIP();
    }

    for (size_t j = 1; j < dev_instance_[i].sub_devices.size(); j++) {
      if (!lzt::can_access_peer(dev_instance_[i].sub_devices[j - 1].dev,
                                dev_instance_[i].sub_devices[j].dev)) {
        LOG_INFO
            << "FAILURE:  IntraDevice SubDevice-to-SubDevice access disabled";
        FAIL();
      }
      if (!lzt::can_access_peer(dev_instance_[i].sub_devices[j].dev,
                                dev_instance_[i].sub_devices[j - 1].dev)) {
        LOG_INFO
            << "FAILURE:  IntraDevice SubDevice-to-SubDevice access disabled";
        FAIL();
      }

      for (uint32_t d = 0; d < 2; d++) {
        size_t src_offset, dst_offset;
        if (d == 0) {
          src_offset = offset_;
          dst_offset = 0;
        } else {
          src_offset = 0;
          dst_offset = offset_;
        }
        lzt::append_memory_copy(dev_instance_[i].sub_devices[j].cmd_bundle.list,
                                dev_instance_[i].sub_devices[j].src_region,
                                initial_pattern_memory, mem_size_ + src_offset);
        lzt::append_barrier(dev_instance_[i].sub_devices[j].cmd_bundle.list,
                            nullptr, 0, nullptr);
        lzt::append_memory_copy(
            dev_instance_[i].sub_devices[j - 1].cmd_bundle.list,
            dev_instance_[i].sub_devices[j - 1].src_region,
            initial_pattern_memory, mem_size_ + src_offset);
        lzt::append_barrier(dev_instance_[i].sub_devices[j - 1].cmd_bundle.list,
                            nullptr, 0, nullptr);
        lzt::append_memory_copy(
            dev_instance_[i].sub_devices[j].cmd_bundle.list,
            static_cast<void *>(
                static_cast<uint8_t *>(
                    dev_instance_[i].sub_devices[j - 1].dst_region) +
                dst_offset),
            static_cast<void *>(
                static_cast<uint8_t *>(
                    dev_instance_[i].sub_devices[j].src_region) +
                src_offset),
            mem_size_, nullptr);
        lzt::append_barrier(dev_instance_[i].sub_devices[j].cmd_bundle.list,
                            nullptr, 0, nullptr);
        lzt::append_memory_copy(
            dev_instance_[i].sub_devices[j - 1].cmd_bundle.list,
            static_cast<void *>(
                static_cast<uint8_t *>(
                    dev_instance_[i].sub_devices[j].dst_region) +
                dst_offset),
            static_cast<void *>(
                static_cast<uint8_t *>(
                    dev_instance_[i].sub_devices[j - 1].src_region) +
                src_offset),
            mem_size_, nullptr);
        lzt::append_barrier(dev_instance_[i].sub_devices[j - 1].cmd_bundle.list,
                            nullptr, 0, nullptr);

        lzt::append_memory_copy(
            dev_instance_[i].sub_devices[j].cmd_bundle.list,
            verification_memory1,
            static_cast<void *>(
                static_cast<uint8_t *>(
                    dev_instance_[i].sub_devices[j - 1].dst_region) +
                dst_offset),
            mem_size_, nullptr);
        lzt::append_barrier(dev_instance_[i].sub_devices[j].cmd_bundle.list,
                            nullptr, 0, nullptr);

        lzt::append_memory_copy(
            dev_instance_[i].sub_devices[j - 1].cmd_bundle.list,
            verification_memory2,
            static_cast<void *>(
                static_cast<uint8_t *>(
                    dev_instance_[i].sub_devices[j].dst_region) +
                dst_offset),
            mem_size_, nullptr);
        lzt::append_barrier(dev_instance_[i].sub_devices[j - 1].cmd_bundle.list,
                            nullptr, 0, nullptr);
        lzt::close_command_list(
            dev_instance_[i].sub_devices[j - 1].cmd_bundle.list);
        lzt::close_command_list(
            dev_instance_[i].sub_devices[j].cmd_bundle.list);
        if (is_immediate_) {
          lzt::synchronize_command_list_host(
              dev_instance_[i].sub_devices[j - 1].cmd_bundle.list, UINT64_MAX);
          lzt::synchronize_command_list_host(
              dev_instance_[i].sub_devices[j].cmd_bundle.list, UINT64_MAX);
        } else {
          lzt::execute_command_lists(
              dev_instance_[i].sub_devices[j - 1].cmd_bundle.queue, 1,
              &dev_instance_[i].sub_devices[j - 1].cmd_bundle.list, nullptr);
          lzt::execute_command_lists(
              dev_instance_[i].sub_devices[j].cmd_bundle.queue, 1,
              &dev_instance_[i].sub_devices[j].cmd_bundle.list, nullptr);
          lzt::synchronize(dev_instance_[i].sub_devices[j - 1].cmd_bundle.queue,
                           UINT64_MAX);
          lzt::synchronize(dev_instance_[i].sub_devices[j].cmd_bundle.queue,
                           UINT64_MAX);
        }
        lzt::reset_command_list(
            dev_instance_[i].sub_devices[j - 1].cmd_bundle.list);
        lzt::reset_command_list(
            dev_instance_[i].sub_devices[j].cmd_bundle.list);

        uint8_t *src =
            static_cast<uint8_t *>(initial_pattern_memory) + src_offset;
        uint8_t *dst1 = static_cast<uint8_t *>(verification_memory1);
        uint8_t *dst2 = static_cast<uint8_t *>(verification_memory2);
        for (uint32_t i = 0; i < mem_size_; i++) {
          ASSERT_EQ(src[i], dst1[i])
              << "Memory Copied from SubDevice did not match.";
          ASSERT_EQ(src[i], dst2[i])
              << "Memory Copied from SubDevice did not match.";
        }
      }
    }
  }

  lzt::free_memory(verification_memory1);
  lzt::free_memory(verification_memory2);
  lzt::free_memory(initial_pattern_memory);
}

LZT_TEST_P(
    zeP2PTests,
    GivenMultipleDevicesWithP2PWhenSettingAndCopyingMemoryToRemoteDeviceThenRemoteDeviceGetsCorrectMemory) {

  if (dev_instance_.size() < 2) {
    LOG_INFO << "Test cannot be run with less than 2 devices";
    GTEST_SKIP();
  }

  for (size_t i = 1; i < dev_instance_.size(); i++) {
    if (!lzt::can_access_peer(dev_instance_[i - 1].dev, dev_instance_[i].dev)) {
      LOG_INFO << "FAILURE:  Device-to-Device access disabled";
      FAIL();
    }
    uint8_t *shr_mem = static_cast<uint8_t *>(lzt::allocate_shared_memory(
        mem_size_ + offset_, 1, 0, 0, dev_instance_[i].dev));
    uint8_t value_before = lzt::generate_value<uint8_t>();
    uint8_t value_after = lzt::generate_value<uint8_t>();

    // Set memory region on device i - 1 and copy to device i

    lzt::append_memory_set(
        dev_instance_[i - 1].cmd_bundle.list,
        static_cast<void *>(
            static_cast<uint8_t *>(dev_instance_[i - 1].src_region) + offset_),
        &value_after, mem_size_);
    if (offset_ > 0) {
      lzt::append_memory_set(dev_instance_[i - 1].cmd_bundle.list,
                             dev_instance_[i - 1].src_region, &value_before,
                             offset_);
    }
    lzt::append_barrier(dev_instance_[i - 1].cmd_bundle.list, nullptr, 0,
                        nullptr);
    lzt::append_memory_copy(
        dev_instance_[i - 1].cmd_bundle.list, dev_instance_[i].dst_region,
        static_cast<void *>(
            static_cast<uint8_t *>(dev_instance_[i - 1].src_region) + offset_),
        mem_size_, nullptr);
    lzt::close_command_list(dev_instance_[i - 1].cmd_bundle.list);
    lzt::execute_and_sync_command_bundle(dev_instance_[i - 1].cmd_bundle,
                                         UINT64_MAX);
    lzt::reset_command_list(dev_instance_[i - 1].cmd_bundle.list);

    // Copy memory region from device i to shared mem, and verify it is
    // correct
    lzt::append_memory_copy(dev_instance_[i].cmd_bundle.list, shr_mem,
                            dev_instance_[i].dst_region, mem_size_, nullptr);
    lzt::close_command_list(dev_instance_[i].cmd_bundle.list);
    lzt::execute_and_sync_command_bundle(dev_instance_[i].cmd_bundle,
                                         UINT64_MAX);
    lzt::reset_command_list(dev_instance_[i].cmd_bundle.list);

    for (uint32_t j = 0; j < mem_size_; j++) {
      ASSERT_EQ(shr_mem[j], value_after)
          << "Memory Copied from Device did not match.";
    }
    lzt::free_memory(shr_mem);
  }
}

LZT_TEST_P(
    zeP2PTests,
    GivenP2PSubDevicesWhenSettingAndCopyingMemoryToRemoteSubDeviceThenRemoteSubDeviceGetsCorrectMemory) {

  for (size_t i = 0U; i < dev_instance_.size(); i++) {
    if (dev_instance_[i].sub_devices.size() < 2) {
      LOG_INFO << "Test cannot be run with less than 2 SubDevices";
      GTEST_SKIP();
    }
    for (size_t j = 1; j < dev_instance_[i].sub_devices.size(); j++) {
      if (!lzt::can_access_peer(dev_instance_[i].sub_devices[j - 1].dev,
                                dev_instance_[i].sub_devices[j].dev)) {
        LOG_INFO
            << "FAILURE:  IntraDevice SubDevice-to-SubDevice access disabled";
        FAIL();
      }
      uint8_t *shr_mem = static_cast<uint8_t *>(lzt::allocate_shared_memory(
          mem_size_ + offset_, 1, 0, 0, dev_instance_[i].sub_devices[j].dev));
      uint8_t value_before = lzt::generate_value<uint8_t>();
      uint8_t value_after = lzt::generate_value<uint8_t>();
      // Set memory region on device i - 1 and copy to device i

      lzt::append_memory_set(
          dev_instance_[i].sub_devices[j - 1].cmd_bundle.list,
          static_cast<void *>(
              static_cast<uint8_t *>(
                  dev_instance_[i].sub_devices[j - 1].src_region) +
              offset_),
          &value_after, mem_size_);
      if (offset_ > 0) {
        lzt::append_memory_set(
            dev_instance_[i].sub_devices[j - 1].cmd_bundle.list,
            dev_instance_[i].sub_devices[j - 1].src_region, &value_before,
            offset_);
      }
      lzt::append_barrier(dev_instance_[i].sub_devices[j - 1].cmd_bundle.list,
                          nullptr, 0, nullptr);
      lzt::append_memory_copy(
          dev_instance_[i].sub_devices[j - 1].cmd_bundle.list,
          dev_instance_[i].sub_devices[j].dst_region,
          static_cast<void *>(
              static_cast<uint8_t *>(
                  dev_instance_[i].sub_devices[j - 1].src_region) +
              offset_),
          mem_size_, nullptr);
      lzt::close_command_list(
          dev_instance_[i].sub_devices[j - 1].cmd_bundle.list);
      lzt::execute_and_sync_command_bundle(
          dev_instance_[i].sub_devices[j - 1].cmd_bundle, UINT64_MAX);
      lzt::reset_command_list(
          dev_instance_[i].sub_devices[j - 1].cmd_bundle.list);

      // Copy memory region from device i to shared mem, and verify it is
      // correct
      lzt::append_memory_copy(
          dev_instance_[i].sub_devices[j].cmd_bundle.list, shr_mem,
          dev_instance_[i].sub_devices[j].dst_region, mem_size_, nullptr);
      lzt::close_command_list(dev_instance_[i].sub_devices[j].cmd_bundle.list);
      lzt::execute_and_sync_command_bundle(
          dev_instance_[i].sub_devices[j].cmd_bundle, UINT64_MAX);
      lzt::reset_command_list(dev_instance_[i].sub_devices[j].cmd_bundle.list);

      for (uint32_t k = 0; k < mem_size_; k++) {
        ASSERT_EQ(shr_mem[k], value_after)
            << "Memory Copied from SubDevice did not match.";
      }
      lzt::free_memory(shr_mem);
    }
  }
}

LZT_TEST_P(
    zeP2PTests,
    GivenMultipleDevicesWithP2PWhenCopyingMemoryRegionToRemoteDeviceThenRemoteDeviceGetsCorrectMemory) {

  int test_count = 0;
  const size_t num_regions = 3;

  void *initial_pattern_memory = lzt::allocate_host_memory(mem_size_ + offset_);
  void *verification_memory = lzt::allocate_host_memory(mem_size_ + offset_);

  lzt::write_data_pattern(initial_pattern_memory, mem_size_ + offset_, 1);

  std::array<uint32_t, num_regions> widths = {1, columns / 2, columns};
  std::array<uint32_t, num_regions> heights = {1, rows / 2, rows};
  std::array<uint32_t, num_regions> depths = {1, slices / 2, slices};

  if (dev_instance_.size() < 2) {
    LOG_INFO << "Test cannot be run with less than 2 devices";
    GTEST_SKIP();
  }

  for (size_t region = 0U; region < num_regions; region++) {
    // Define region to be copied from/to
    uint32_t width = widths[region];
    uint32_t height = heights[region];
    uint32_t depth = depths[region];

    ze_copy_region_t src_region;
    src_region.originX = 0;
    src_region.originY = 0;
    src_region.originZ = 0;
    src_region.width = width;
    src_region.height = height;
    src_region.depth = depth;

    ze_copy_region_t dest_region;
    dest_region.originX = 0;
    dest_region.originY = 0;
    dest_region.originZ = 0;
    dest_region.width = width;
    dest_region.height = height;
    dest_region.depth = depth;

    DevInstance *ptr_dev_src;
    DevInstance *ptr_dev_dst;

    for (size_t i = 1; i < dev_instance_.size(); i++) {

      for (uint32_t d = 0; d < 2; d++) {
        size_t src_offset, dst_offset;

        if (d == 0) {
          src_offset = offset_;
          dst_offset = 0;
        } else {
          src_offset = 0;
          dst_offset = offset_;
        }

        if (lzt::can_access_peer(dev_instance_[i - 1].dev,
                                 dev_instance_[i].dev)) {
          ptr_dev_src = &dev_instance_[i - 1];
          ptr_dev_dst = &dev_instance_[i];
        } else if (lzt::can_access_peer(dev_instance_[i].dev,
                                        dev_instance_[i - 1].dev)) {
          ptr_dev_src = &dev_instance_[i];
          ptr_dev_dst = &dev_instance_[i - 1];
        } else {
          continue;
        }
        test_count++;
        lzt::append_memory_copy(ptr_dev_src->cmd_bundle.list,
                                ptr_dev_src->src_region, initial_pattern_memory,
                                mem_size_ + src_offset);
        lzt::close_command_list(ptr_dev_src->cmd_bundle.list);
        lzt::execute_and_sync_command_bundle(ptr_dev_src->cmd_bundle,
                                             UINT64_MAX);
        lzt::reset_command_list(ptr_dev_src->cmd_bundle.list);

        lzt::append_memory_copy_region(
            ptr_dev_src->cmd_bundle.list,
            static_cast<uint8_t *>(ptr_dev_dst->dst_region) + dst_offset,
            &dest_region, columns, columns * rows,
            static_cast<uint8_t *>(ptr_dev_src->src_region) + src_offset,
            &src_region, columns, columns * rows, nullptr);

        lzt::close_command_list(ptr_dev_src->cmd_bundle.list);
        lzt::execute_and_sync_command_bundle(ptr_dev_src->cmd_bundle,
                                             UINT64_MAX);
        lzt::reset_command_list(ptr_dev_src->cmd_bundle.list);

        lzt::append_memory_copy(
            ptr_dev_dst->cmd_bundle.list, verification_memory,
            static_cast<uint8_t *>(ptr_dev_dst->dst_region) + dst_offset,
            mem_size_);
        lzt::close_command_list(ptr_dev_dst->cmd_bundle.list);
        lzt::execute_and_sync_command_bundle(ptr_dev_dst->cmd_bundle,
                                             UINT64_MAX);
        lzt::reset_command_list(ptr_dev_dst->cmd_bundle.list);

        for (uint32_t z = 0U; z < depth; z++) {
          for (uint32_t y = 0U; y < height; y++) {
            for (uint32_t x = 0U; x < width; x++) {
              // index calculated based on memory sized by rows * columns *
              // slices
              size_t index = z * columns * rows + y * columns + x;
              uint8_t dest_val =
                  static_cast<uint8_t *>(verification_memory)[index];
              uint8_t src_val = static_cast<uint8_t *>(
                  initial_pattern_memory)[index + src_offset];

              ASSERT_EQ(dest_val, src_val)
                  << "Copy failed with region(w,h,d)=(" << width << ", "
                  << height << ", " << depth << ")";
            }
          }
        }
      }
    }
  }

  if (!test_count) {
    LOG_INFO << "FAILURE:  Device-to-Device access disabled";
    FAIL();
  }
  lzt::free_memory(verification_memory);
  lzt::free_memory(initial_pattern_memory);
}

LZT_TEST_P(
    zeP2PTests,
    GivenP2PSubDevicesWhenCopyingMemoryRegionToSubDeviceOnSameDeviceThenRemoteSubDeviceGetsCorrectMemory) {

  int test_count = 0;
  const uint32_t num_regions = 3;

  void *initial_pattern_memory = lzt::allocate_host_memory(mem_size_ + offset_);
  void *verification_memory = lzt::allocate_host_memory(mem_size_ + offset_);

  lzt::write_data_pattern(initial_pattern_memory, mem_size_ + offset_, 1);

  std::array<uint32_t, num_regions> widths = {1, columns / 2, columns};
  std::array<uint32_t, num_regions> heights = {1, rows / 2, rows};
  std::array<uint32_t, num_regions> depths = {1, slices / 2, slices};

  for (uint32_t region = 0U; region < num_regions; region++) {
    // Define region to be copied from/to
    uint32_t width = widths[region];
    uint32_t height = heights[region];
    uint32_t depth = depths[region];

    ze_copy_region_t src_region;
    src_region.originX = 0;
    src_region.originY = 0;
    src_region.originZ = 0;
    src_region.width = width;
    src_region.height = height;
    src_region.depth = depth;

    ze_copy_region_t dest_region;
    dest_region.originX = 0;
    dest_region.originY = 0;
    dest_region.originZ = 0;
    dest_region.width = width;
    dest_region.height = height;
    dest_region.depth = depth;

    DevInstance *ptr_dev_src;
    DevInstance *ptr_dev_dst;
    for (size_t i = 0U; i < dev_instance_.size(); i++) {

      for (uint32_t d = 0; d < 2; d++) {
        size_t src_offset, dst_offset;

        if (d == 0) {
          src_offset = offset_;
          dst_offset = 0;
        } else {
          src_offset = 0;
          dst_offset = offset_;
        }

        if (dev_instance_[i].sub_devices.size() < 2) {
          LOG_INFO << "Test cannot be run with less than 2 SubDevices";
          GTEST_SKIP();
        }

        for (size_t j = 1; j < dev_instance_[i].sub_devices.size(); j++) {

          if (lzt::can_access_peer(dev_instance_[i].sub_devices[j - 1].dev,
                                   dev_instance_[i].sub_devices[j].dev)) {
            ptr_dev_src = &dev_instance_[i].sub_devices[j - 1];
            ptr_dev_dst = &dev_instance_[i].sub_devices[j];
          } else if (lzt::can_access_peer(
                         dev_instance_[i].sub_devices[j].dev,
                         dev_instance_[i].sub_devices[j - 1].dev)) {
            ptr_dev_src = &dev_instance_[i].sub_devices[j];
            ptr_dev_dst = &dev_instance_[i].sub_devices[j - 1];
          } else {
            continue;
          }

          test_count++;
          lzt::append_memory_copy(
              ptr_dev_src->cmd_bundle.list, ptr_dev_src->src_region,
              initial_pattern_memory, mem_size_ + src_offset);
          lzt::close_command_list(ptr_dev_src->cmd_bundle.list);
          lzt::execute_and_sync_command_bundle(ptr_dev_src->cmd_bundle,
                                               UINT64_MAX);
          lzt::reset_command_list(ptr_dev_src->cmd_bundle.list);

          lzt::append_memory_copy_region(
              ptr_dev_src->cmd_bundle.list,
              static_cast<uint8_t *>(ptr_dev_dst->dst_region) + dst_offset,
              &dest_region, columns, columns * rows,
              static_cast<uint8_t *>(ptr_dev_src->src_region) + src_offset,
              &src_region, columns, columns * rows, nullptr);

          lzt::close_command_list(ptr_dev_src->cmd_bundle.list);
          lzt::execute_and_sync_command_bundle(ptr_dev_src->cmd_bundle,
                                               UINT64_MAX);
          lzt::reset_command_list(ptr_dev_src->cmd_bundle.list);

          lzt::append_memory_copy(
              ptr_dev_dst->cmd_bundle.list, verification_memory,
              static_cast<uint8_t *>(ptr_dev_dst->dst_region) + dst_offset,
              mem_size_);
          lzt::close_command_list(ptr_dev_dst->cmd_bundle.list);
          lzt::execute_and_sync_command_bundle(ptr_dev_dst->cmd_bundle,
                                               UINT64_MAX);
          lzt::reset_command_list(ptr_dev_dst->cmd_bundle.list);

          for (uint32_t z = 0U; z < depth; z++) {
            for (uint32_t y = 0U; y < height; y++) {
              for (uint32_t x = 0U; x < width; x++) {
                // index calculated based on memory sized by rows * columns *
                // slices
                size_t index = z * columns * rows + y * columns + x;
                uint8_t dest_val =
                    static_cast<uint8_t *>(verification_memory)[index];
                uint8_t src_val = static_cast<uint8_t *>(
                    initial_pattern_memory)[index + src_offset];

                ASSERT_EQ(dest_val, src_val)
                    << "Copy failed with region(w,h,d)=(" << width << ", "
                    << height << ", " << depth << ")";
              }
            }
          }
        }
      }
    }
  }

  if (!test_count) {
    LOG_INFO << "FAILURE:  IntraDevice SubDevice-to-SubDevice access disabled";
  }

  lzt::free_memory(verification_memory);
  lzt::free_memory(initial_pattern_memory);
}

LZT_TEST_P(
    zeP2PTests,
    GivenP2PSubDevicesWhenCopyingMemoryRegionToSubDeviceOnDifferentDeviceThenRemoteSubDeviceGetsCorrectMemory) {

  int test_count = 0;
  const uint32_t num_regions = 3;

  void *initial_pattern_memory = lzt::allocate_host_memory(mem_size_ + offset_);
  void *verification_memory = lzt::allocate_host_memory(mem_size_ + offset_);

  lzt::write_data_pattern(initial_pattern_memory, mem_size_ + offset_, 1);

  std::array<uint32_t, num_regions> widths = {1, columns / 2, columns};
  std::array<uint32_t, num_regions> heights = {1, rows / 2, rows};
  std::array<uint32_t, num_regions> depths = {1, slices / 2, slices};

  if (dev_instance_.size() < 2) {
    LOG_INFO << "Test cannot be run with less than 2 Devices";
    GTEST_SKIP();
  }

  for (uint32_t region = 0U; region < num_regions; region++) {
    // Define region to be copied from/to
    uint32_t width = widths[region];
    uint32_t height = heights[region];
    uint32_t depth = depths[region];

    ze_copy_region_t src_region;
    src_region.originX = 0;
    src_region.originY = 0;
    src_region.originZ = 0;
    src_region.width = width;
    src_region.height = height;
    src_region.depth = depth;

    ze_copy_region_t dest_region;
    dest_region.originX = 0;
    dest_region.originY = 0;
    dest_region.originZ = 0;
    dest_region.width = width;
    dest_region.height = height;
    dest_region.depth = depth;

    DevInstance *ptr_dev_src;
    DevInstance *ptr_dev_dst;
    for (size_t i = 1; i < dev_instance_.size(); i++) {

      for (uint32_t d = 0; d < 2; d++) {
        size_t src_offset, dst_offset;

        if (d == 0) {
          src_offset = offset_;
          dst_offset = 0;
        } else {
          src_offset = 0;
          dst_offset = offset_;
        }

        for (size_t j = 0U; j < dev_instance_[i].sub_devices.size(); j++) {
          for (size_t k = 0U; k < dev_instance_[i-1].sub_devices.size(); k++) {

            if (lzt::can_access_peer(dev_instance_[i].sub_devices[j].dev,
                                     dev_instance_[i-1].sub_devices[k].dev)) {
              ptr_dev_src = &dev_instance_[i].sub_devices[j];
              ptr_dev_dst = &dev_instance_[i-1].sub_devices[k];
            } else if (lzt::can_access_peer(
                           dev_instance_[i-1].sub_devices[k].dev,
                           dev_instance_[i].sub_devices[j].dev)) {
              ptr_dev_src = &dev_instance_[i-1].sub_devices[k];
              ptr_dev_dst = &dev_instance_[i].sub_devices[j];
            } else {
              continue;
            }

            test_count++;
            lzt::append_memory_copy(
                ptr_dev_src->cmd_bundle.list, ptr_dev_src->src_region,
                initial_pattern_memory, mem_size_ + src_offset);
            lzt::close_command_list(ptr_dev_src->cmd_bundle.list);
            lzt::execute_and_sync_command_bundle(ptr_dev_src->cmd_bundle,
                                                 UINT64_MAX);
            lzt::reset_command_list(ptr_dev_src->cmd_bundle.list);

            lzt::append_memory_copy_region(
                ptr_dev_src->cmd_bundle.list,
                static_cast<uint8_t *>(ptr_dev_dst->dst_region) + dst_offset,
                &dest_region, columns, columns * rows,
                static_cast<uint8_t *>(ptr_dev_src->src_region) + src_offset,
                &src_region, columns, columns * rows, nullptr);

            lzt::close_command_list(ptr_dev_src->cmd_bundle.list);
            lzt::execute_and_sync_command_bundle(ptr_dev_src->cmd_bundle,
                                                 UINT64_MAX);
            lzt::reset_command_list(ptr_dev_src->cmd_bundle.list);

            lzt::append_memory_copy(
                ptr_dev_dst->cmd_bundle.list, verification_memory,
                static_cast<uint8_t *>(ptr_dev_dst->dst_region) + dst_offset,
                mem_size_);
            lzt::close_command_list(ptr_dev_dst->cmd_bundle.list);
            lzt::execute_and_sync_command_bundle(ptr_dev_dst->cmd_bundle,
                                                 UINT64_MAX);
            lzt::reset_command_list(ptr_dev_dst->cmd_bundle.list);

            for (uint32_t z = 0U; z < depth; z++) {
              for (uint32_t y = 0U; y < height; y++) {
                for (uint32_t x = 0U; x < width; x++) {
                  // index calculated based on memory sized by rows * columns *
                  // slices
                  size_t index = z * columns * rows + y * columns + x;
                  uint8_t dest_val =
                      static_cast<uint8_t *>(verification_memory)[index];
                  uint8_t src_val = static_cast<uint8_t *>(
                      initial_pattern_memory)[index + src_offset];

                  ASSERT_EQ(dest_val, src_val)
                      << "Copy failed with region(w,h,d)=(" << width << ", "
                      << height << ", " << depth << ")";
                }
              }
            }
          }
        }
      }
    }
  }

  if (!test_count) {
    LOG_INFO << "FAILURE:  InterDevice SubDevice-to-SubDevice access disabled";
  }

  lzt::free_memory(verification_memory);
  lzt::free_memory(initial_pattern_memory);
}

LZT_TEST_P(
    zeP2PTests,
    GivenMultipleDevicesWithP2PWhenKernelReadsRemoteDeviceMemoryWithDevicePointerOffsetThenCorrectDataIsRead) {

  std::string module_name = "p2p_test_offset_pointer.spv";
  std::string func_name = "multi_device_function";

  uint32_t dev_instance_size = static_cast<uint32_t>(dev_instance_.size());

  if (dev_instance_size < 2) {
    LOG_INFO << "Test cannot be run with less than 2 Devices";
    GTEST_SKIP();
  }

  for (uint32_t i = 1; i < dev_instance_size; i++) {
    if (!lzt::can_access_peer(dev_instance_[i - 1].dev, dev_instance_[i].dev)) {
      LOG_INFO << "FAILURE:  Device-to-Device access disabled";
      FAIL();
    }
    ze_module_handle_t module =
        lzt::create_module(dev_instance_[i - 1].dev, module_name);
    uint8_t *shr_mem = static_cast<uint8_t *>(
        lzt::allocate_shared_memory(mem_size_, 1, 0, 0, dev_instance_[i].dev));

    // random memory region on device i. Allow "space" for increment.
    uint8_t value_before = lzt::generate_value<uint8_t>() & 0x7f;
    uint8_t value_after = lzt::generate_value<uint8_t>() & 0x7f;

    lzt::append_memory_set(
        dev_instance_[i].cmd_bundle.list,
        static_cast<void *>(
            static_cast<uint8_t *>(dev_instance_[i].src_region) + offset_),
        &value_after, mem_size_);
    if (offset_ > 0) {
      lzt::append_memory_set(dev_instance_[i].cmd_bundle.list,
                             dev_instance_[i].src_region, &value_before,
                             offset_);
    }
    lzt::append_barrier(dev_instance_[i].cmd_bundle.list, nullptr, 0, nullptr);
    lzt::close_command_list(dev_instance_[i].cmd_bundle.list);
    lzt::execute_and_sync_command_bundle(dev_instance_[i].cmd_bundle,
                                         UINT64_MAX);
    lzt::reset_command_list(dev_instance_[i].cmd_bundle.list);

    // device (i - 1) will modify memory allocated for device i
    lzt::create_and_execute_function(
        dev_instance_[i - 1].dev, module, func_name, 1,
        static_cast<void *>(
            static_cast<uint8_t *>(dev_instance_[i].src_region) + offset_),
        is_immediate_);

    // copy memory to shared region and verify it is correct
    lzt::append_memory_copy(
        dev_instance_[i].cmd_bundle.list, shr_mem,
        static_cast<void *>(
            static_cast<uint8_t *>(dev_instance_[i].src_region) + offset_),
        mem_size_, nullptr);
    lzt::close_command_list(dev_instance_[i].cmd_bundle.list);
    lzt::execute_and_sync_command_bundle(dev_instance_[i].cmd_bundle,
                                         UINT64_MAX);
    lzt::reset_command_list(dev_instance_[i].cmd_bundle.list);
    ASSERT_EQ(shr_mem[0], value_after + 1)
        << "Memory Copied from Device did not match.";

    lzt::destroy_module(module);
  }
}

LZT_TEST_P(
    zeP2PTests,
    GivenP2PSubDevicesWhenKernelReadsRemoteSubDeviceMemoryWithSubDevicePointerOffsetThenCorrectDataIsRead) {

  std::string module_name = "p2p_test_offset_pointer.spv";
  std::string func_name = "multi_device_function";

  uint32_t dev_instance_size = static_cast<uint32_t>(dev_instance_.size());

  for (uint32_t i = 0U; i < dev_instance_size; i++) {

    if (dev_instance_[i].sub_devices.size() < 2) {
      LOG_INFO << "Test cannot be run with less than 2 SubDevices";
      GTEST_SKIP();
    }

    for (size_t j = 1; j < dev_instance_[i].sub_devices.size(); j++) {
      if (!lzt::can_access_peer(dev_instance_[i].sub_devices[j - 1].dev,
                                dev_instance_[i].sub_devices[j].dev)) {
        LOG_INFO
            << "FAILURE:  IntraDevice SubDevice-to-SubDevice access disabled";
        FAIL();
      }

      ze_module_handle_t module = lzt::create_module(
          dev_instance_[i].sub_devices[j - 1].dev, module_name);
      uint8_t *shr_mem = static_cast<uint8_t *>(lzt::allocate_shared_memory(
          mem_size_, 1, 0, 0, dev_instance_[i].sub_devices[j].dev));

      // random memory region on device i. Allow "space" for increment.
      uint8_t value_before = lzt::generate_value<uint8_t>() & 0x7f;
      uint8_t value_after = lzt::generate_value<uint8_t>() & 0x7f;

      lzt::append_memory_set(
          dev_instance_[i].sub_devices[j].cmd_bundle.list,
          static_cast<void *>(static_cast<uint8_t *>(
                                  dev_instance_[i].sub_devices[j].src_region) +
                              offset_),
          &value_after, mem_size_);
      if (offset_ > 0) {
        lzt::append_memory_set(dev_instance_[i].sub_devices[j].cmd_bundle.list,
                               dev_instance_[i].sub_devices[j].src_region,
                               &value_before, offset_);
      }
      lzt::append_barrier(dev_instance_[i].sub_devices[j].cmd_bundle.list,
                          nullptr, 0, nullptr);
      lzt::close_command_list(dev_instance_[i].sub_devices[j].cmd_bundle.list);
      lzt::execute_and_sync_command_bundle(
          dev_instance_[i].sub_devices[j].cmd_bundle, UINT64_MAX);
      lzt::reset_command_list(dev_instance_[i].sub_devices[j].cmd_bundle.list);

      // device (i - 1) will modify memory allocated for device i
      lzt::create_and_execute_function(
          dev_instance_[i].sub_devices[j - 1].dev, module, func_name, 1,
          static_cast<void *>(static_cast<uint8_t *>(
                                  dev_instance_[i].sub_devices[j].src_region) +
                              offset_),
          is_immediate_);

      // copy memory to shared region and verify it is correct
      lzt::append_memory_copy(
          dev_instance_[i].sub_devices[j].cmd_bundle.list, shr_mem,
          static_cast<void *>(static_cast<uint8_t *>(
                                  dev_instance_[i].sub_devices[j].src_region) +
                              offset_),
          mem_size_, nullptr);
      lzt::append_barrier(dev_instance_[i].sub_devices[j].cmd_bundle.list,
                          nullptr, 0, nullptr);
      lzt::close_command_list(dev_instance_[i].sub_devices[j].cmd_bundle.list);
      lzt::execute_and_sync_command_bundle(
          dev_instance_[i].sub_devices[j].cmd_bundle, UINT64_MAX);
      lzt::reset_command_list(dev_instance_[i].sub_devices[j].cmd_bundle.list);
      ASSERT_EQ(shr_mem[0], value_after + 1)
          << "Memory Copied from SubDevice did not match.";
      lzt::destroy_module(module);
    }
  }
}

LZT_TEST_P(
    zeP2PTests,
    GivenMultipleDevicesWithP2PWhenKernelReadsRemoteDeviceMemoryThenCorrectDataIsRead) {

  std::string module_name = "p2p_test.spv";
  std::string func_name = "multi_device_function";

  uint32_t dev_instance_size = static_cast<uint32_t>(dev_instance_.size());

  if (dev_instance_size < 2) {
    LOG_INFO << "Test cannot be run with less than 2 Devices";
    GTEST_SKIP();
  }

  for (uint32_t i = 1; i < dev_instance_size; i++) {
    if (!lzt::can_access_peer(dev_instance_[i - 1].dev, dev_instance_[i].dev)) {
      LOG_INFO << "FAILURE:  Device-to-Device access disabled";
      FAIL();
    }
    ze_module_handle_t module =
        lzt::create_module(dev_instance_[i - 1].dev, module_name);
    uint8_t *shr_mem = static_cast<uint8_t *>(lzt::allocate_shared_memory(
        mem_size_ + offset_, 1, 0, 0, dev_instance_[i].dev));

    // random memory region on device i. Allow "space" for increment.
    uint8_t value_before = lzt::generate_value<uint8_t>() & 0x7f;
    uint8_t value_after = lzt::generate_value<uint8_t>() & 0x7f;

    lzt::append_memory_set(
        dev_instance_[i].cmd_bundle.list,
        static_cast<void *>(
            static_cast<uint8_t *>(dev_instance_[i].src_region) + offset_),
        &value_after, mem_size_);
    if (offset_ > 0) {
      lzt::append_memory_set(dev_instance_[i].cmd_bundle.list,
                             dev_instance_[i].src_region, &value_before,
                             offset_);
    }
    lzt::append_barrier(dev_instance_[i].cmd_bundle.list, nullptr, 0, nullptr);
    lzt::close_command_list(dev_instance_[i].cmd_bundle.list);
    lzt::execute_and_sync_command_bundle(dev_instance_[i].cmd_bundle,
                                         UINT64_MAX);
    lzt::reset_command_list(dev_instance_[i].cmd_bundle.list);

    lzt::FunctionArg arg;
    std::vector<lzt::FunctionArg> args;

    arg.arg_size = sizeof(uint8_t *);
    arg.arg_value = &dev_instance_[i].src_region;
    args.push_back(arg);
    arg.arg_size = sizeof(int);
    int offset = static_cast<int>(offset_);
    arg.arg_value = &offset;
    args.push_back(arg);

    // device (i - 1) will modify memory allocated for device i
    lzt::create_and_execute_function(dev_instance_[i - 1].dev, module,
                                     func_name, 1, args, is_immediate_);

    // copy memory to shared region and verify it is correct
    lzt::append_memory_copy(
        dev_instance_[i].cmd_bundle.list, shr_mem,
        static_cast<void *>(
            static_cast<uint8_t *>(dev_instance_[i].src_region) + offset_),
        mem_size_, nullptr);
    lzt::close_command_list(dev_instance_[i].cmd_bundle.list);
    lzt::execute_and_sync_command_bundle(dev_instance_[i].cmd_bundle,
                                         UINT64_MAX);
    lzt::reset_command_list(dev_instance_[i].cmd_bundle.list);
    ASSERT_EQ(shr_mem[0], value_after + 1)
        << "Memory Copied from Device did not match.";

    lzt::destroy_module(module);
  }
}

LZT_TEST_P(
    zeP2PTests,
    GivenP2PSubDevicesWhenKernelReadsRemoteSubDeviceMemoryThenCorrectDataIsRead) {

  std::string module_name = "p2p_test.spv";
  std::string func_name = "multi_device_function";

  uint32_t dev_instance_size = static_cast<uint32_t>(dev_instance_.size());

  for (uint32_t i = 0; i < dev_instance_size; i++) {

    if (dev_instance_[i].sub_devices.size() < 2) {
      LOG_INFO << "Test cannot be run with less than 2 SubDevices";
      GTEST_SKIP();
    }

    for (size_t j = 1; j < dev_instance_[i].sub_devices.size(); j++) {
      if (!lzt::can_access_peer(dev_instance_[i].sub_devices[j - 1].dev,
                                dev_instance_[i].sub_devices[j].dev)) {
        LOG_INFO
            << "FAILURE:  IntraDevice SubDevice-to-SubDevice access disabled";
        FAIL();
      }

      ze_module_handle_t module = lzt::create_module(
          dev_instance_[i].sub_devices[j - 1].dev, module_name);
      uint8_t *shr_mem = static_cast<uint8_t *>(lzt::allocate_shared_memory(
          mem_size_ + offset_, 1, 0, 0, dev_instance_[i].sub_devices[j].dev));

      // random memory region on device i. Allow "space" for increment.
      uint8_t value_before = lzt::generate_value<uint8_t>() & 0x7f;
      uint8_t value_after = lzt::generate_value<uint8_t>() & 0x7f;

      lzt::append_memory_set(
          dev_instance_[i].sub_devices[j].cmd_bundle.list,
          static_cast<void *>(static_cast<uint8_t *>(
                                  dev_instance_[i].sub_devices[j].src_region) +
                              offset_),
          &value_after, mem_size_);
      if (offset_ > 0) {
        lzt::append_memory_set(dev_instance_[i].sub_devices[j].cmd_bundle.list,
                               dev_instance_[i].sub_devices[j].src_region,
                               &value_before, offset_);
      }
      lzt::append_barrier(dev_instance_[i].sub_devices[j].cmd_bundle.list,
                          nullptr, 0, nullptr);
      lzt::close_command_list(dev_instance_[i].sub_devices[j].cmd_bundle.list);
      lzt::execute_and_sync_command_bundle(
          dev_instance_[i].sub_devices[j].cmd_bundle, UINT64_MAX);
      lzt::reset_command_list(dev_instance_[i].sub_devices[j].cmd_bundle.list);

      lzt::FunctionArg arg;
      std::vector<lzt::FunctionArg> args;

      arg.arg_size = sizeof(uint8_t *);
      arg.arg_value = &dev_instance_[i].sub_devices[j].src_region;
      args.push_back(arg);
      arg.arg_size = sizeof(int);
      int offset = static_cast<int>(offset_);
      arg.arg_value = &offset;
      args.push_back(arg);

      // device (i - 1) will modify memory allocated for device i
      lzt::create_and_execute_function(dev_instance_[i].sub_devices[j - 1].dev,
                                       module, func_name, 1, args,
                                       is_immediate_);

      // copy memory to shared region and verify it is correct
      lzt::append_memory_copy(
          dev_instance_[i].sub_devices[j].cmd_bundle.list, shr_mem,
          static_cast<void *>(static_cast<uint8_t *>(
                                  dev_instance_[i].sub_devices[j].src_region) +
                              offset_),
          mem_size_, nullptr);
      lzt::append_barrier(dev_instance_[i].sub_devices[j].cmd_bundle.list,
                          nullptr, 0, nullptr);
      lzt::close_command_list(dev_instance_[i].sub_devices[j].cmd_bundle.list);
      lzt::execute_and_sync_command_bundle(
          dev_instance_[i].sub_devices[j].cmd_bundle, UINT64_MAX);
      lzt::reset_command_list(dev_instance_[i].sub_devices[j].cmd_bundle.list);
      ASSERT_EQ(shr_mem[0], value_after + 1)
          << "Memory Copied from SubDevice did not match.";
      lzt::destroy_module(module);
    }
  }
}

INSTANTIATE_TEST_SUITE_P(
    GivenMultipleDevicesWithP2PWhenCopyingDeviceMemoryFromRemoteDeviceThenSuccessIsReturned_IP,
    zeP2PTests,
    testing::Combine(::testing::Values(LZT_P2P_MEMORY_TYPE_DEVICE,
                                       LZT_P2P_MEMORY_TYPE_SHARED,
                                       LZT_P2P_MEMORY_TYPE_MEMORY_RESERVATION),
                     ::testing::Values(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
                                       13, 14, 15, 16, 32, 64, 128, 255, 510,
                                       1021, 2043),
                     ::testing::Bool()));

} // namespace
