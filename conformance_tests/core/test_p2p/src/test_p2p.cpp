/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <array>

#include "gtest/gtest.h"

#include "utils/utils.hpp"
#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"

namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>

namespace {

class zeP2PTests : public ::testing::Test,
                   public ::testing::WithParamInterface<
                       std::tuple<ze_memory_type_t, size_t>> {
protected:
  void SetUp() override {
    ze_memory_type_t memory_type = std::get<0>(GetParam());
    offset_ = std::get<1>(GetParam());
    ze_bool_t can_access;
    auto driver = lzt::get_default_driver();
    auto context = lzt::get_default_context();
    auto devices = lzt::get_ze_devices(driver);

    for (auto device : devices) {
      DevInstance instance;

      instance.dev = device;
      instance.dev_grp = driver;
      if (memory_type == ZE_MEMORY_TYPE_DEVICE) {
        instance.src_region = lzt::allocate_device_memory(
            mem_size_ + offset_, 1, 0, device, context);
        instance.dst_region = lzt::allocate_device_memory(
            mem_size_ + offset_, 1, 0, device, context);
      } else if (memory_type == ZE_MEMORY_TYPE_SHARED) {
        instance.src_region =
            lzt::allocate_shared_memory(mem_size_ + offset_, 1, 0, 0, device);
        instance.dst_region =
            lzt::allocate_shared_memory(mem_size_ + offset_, 1, 0, 0, device);
      } else {
        FAIL() << "Unexpected memory type";
      }

      instance.cmd_list = lzt::create_command_list(device);
      instance.cmd_q = lzt::create_command_queue(device);

      for (auto sub_device : lzt::get_ze_sub_devices(device)) {
        DevInstance sub_device_instance;
        sub_device_instance.dev = sub_device;
        sub_device_instance.dev_grp = instance.dev_grp;
        if (memory_type == ZE_MEMORY_TYPE_DEVICE) {
          sub_device_instance.src_region = lzt::allocate_device_memory(
              mem_size_ + offset_, 1, 0, sub_device, context);
          sub_device_instance.dst_region = lzt::allocate_device_memory(
              mem_size_ + offset_, 1, 0, sub_device, context);

        } else if (memory_type == ZE_MEMORY_TYPE_SHARED) {
          sub_device_instance.src_region = lzt::allocate_shared_memory(
              mem_size_ + offset_, 1, 0, 0, sub_device);
          sub_device_instance.dst_region = lzt::allocate_shared_memory(
              mem_size_ + offset_, 1, 0, 0, sub_device);
        } else {
          FAIL() << "Unexpected memory type";
        }
        sub_device_instance.cmd_list = lzt::create_command_list(sub_device);
        sub_device_instance.cmd_q = lzt::create_command_queue(sub_device);
        instance.sub_devices.push_back(sub_device_instance);
      }

      dev_instance_.push_back(instance);
    }
  }

  void TearDown() override {

    for (auto instance : dev_instance_) {

      lzt::destroy_command_queue(instance.cmd_q);
      lzt::destroy_command_list(instance.cmd_list);

      lzt::free_memory(instance.src_region);
      lzt::free_memory(instance.dst_region);

      for (auto sub_device_instance : instance.sub_devices) {
        lzt::destroy_command_queue(sub_device_instance.cmd_q);
        lzt::destroy_command_list(sub_device_instance.cmd_list);

        lzt::free_memory(sub_device_instance.src_region);
        lzt::free_memory(sub_device_instance.dst_region);
      }
    }
  }

  struct DevInstance {
    ze_device_handle_t dev;
    ze_driver_handle_t dev_grp;
    void *src_region;
    void *dst_region;
    ze_command_list_handle_t cmd_list;
    ze_command_queue_handle_t cmd_q;
    std::vector<DevInstance> sub_devices;
  };
  const uint32_t columns = 8;
  const uint32_t rows = 8;
  const uint32_t slices = 8;
  size_t mem_size_ = columns * rows * slices;
  size_t offset_;
  std::vector<DevInstance> dev_instance_;
};

TEST_P(
    zeP2PTests,
    GivenP2PDevicesWhenCopyingDeviceMemoryToAndFromRemoteDeviceThenSuccessIsReturned) {

  void *initial_pattern_memory = lzt::allocate_host_memory(mem_size_ + offset_);
  void *verification_memory1 = lzt::allocate_host_memory(mem_size_ + offset_);
  void *verification_memory2 = lzt::allocate_host_memory(mem_size_ + offset_);

  uint8_t *src = static_cast<uint8_t *>(initial_pattern_memory);
  for (uint32_t i = 0; i < mem_size_ + offset_; i++) {
    src[i] = i & 0xff;
  }

  for (uint32_t i = 1; i < dev_instance_.size(); i++) {
    if (!lzt::can_access_peer(dev_instance_[i - 1].dev, dev_instance_[i].dev)) {
      continue;
    }
    if (!lzt::can_access_peer(dev_instance_[i].dev, dev_instance_[i - 1].dev)) {
      continue;
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
      lzt::append_memory_copy(dev_instance_[i].cmd_list,
                              dev_instance_[i].src_region,
                              initial_pattern_memory, mem_size_ + src_offset);
      lzt::append_barrier(dev_instance_[i].cmd_list, nullptr, 0, nullptr);
      lzt::append_memory_copy(dev_instance_[i - 1].cmd_list,
                              dev_instance_[i - 1].src_region,
                              initial_pattern_memory, mem_size_ + src_offset);
      lzt::append_barrier(dev_instance_[i - 1].cmd_list, nullptr, 0, nullptr);
      lzt::append_memory_copy(
          dev_instance_[i].cmd_list,
          static_cast<void *>(
              static_cast<uint8_t *>(dev_instance_[i - 1].dst_region) +
              dst_offset),
          static_cast<void *>(
              static_cast<uint8_t *>(dev_instance_[i].src_region) + src_offset),
          mem_size_, nullptr);
      lzt::append_barrier(dev_instance_[i].cmd_list, nullptr, 0, nullptr);
      lzt::append_memory_copy(
          dev_instance_[i - 1].cmd_list,
          static_cast<void *>(
              static_cast<uint8_t *>(dev_instance_[i].dst_region) + dst_offset),
          static_cast<void *>(
              static_cast<uint8_t *>(dev_instance_[i - 1].src_region) +
              src_offset),
          mem_size_, nullptr);

      lzt::append_barrier(dev_instance_[i - 1].cmd_list, nullptr, 0, nullptr);
      lzt::append_memory_copy(
          dev_instance_[i].cmd_list, verification_memory1,
          static_cast<void *>(
              static_cast<uint8_t *>(dev_instance_[i - 1].dst_region) +
              dst_offset),
          mem_size_, nullptr);
      lzt::append_barrier(dev_instance_[i].cmd_list, nullptr, 0, nullptr);
      lzt::append_memory_copy(
          dev_instance_[i - 1].cmd_list, verification_memory2,
          static_cast<void *>(
              static_cast<uint8_t *>(dev_instance_[i].dst_region) + dst_offset),
          mem_size_, nullptr);
      lzt::append_barrier(dev_instance_[i - 1].cmd_list, nullptr, 0, nullptr);
      lzt::close_command_list(dev_instance_[i - 1].cmd_list);
      lzt::close_command_list(dev_instance_[i].cmd_list);

      lzt::execute_command_lists(dev_instance_[i - 1].cmd_q, 1,
                                 &dev_instance_[i - 1].cmd_list, nullptr);
      lzt::execute_command_lists(dev_instance_[i].cmd_q, 1,
                                 &dev_instance_[i].cmd_list, nullptr);
      lzt::synchronize(dev_instance_[i - 1].cmd_q, UINT64_MAX);
      lzt::reset_command_list(dev_instance_[i - 1].cmd_list);
      lzt::synchronize(dev_instance_[i].cmd_q, UINT64_MAX);
      lzt::reset_command_list(dev_instance_[i].cmd_list);

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

TEST_P(
    zeP2PTests,
    GivenP2PSubDevicesWhenCopyingDeviceMemoryToAndFromRemoteSubDeviceThenSuccessIsReturned) {

  void *initial_pattern_memory = lzt::allocate_host_memory(mem_size_ + offset_);
  void *verification_memory1 = lzt::allocate_host_memory(mem_size_ + offset_);
  void *verification_memory2 = lzt::allocate_host_memory(mem_size_ + offset_);

  uint8_t *src = static_cast<uint8_t *>(initial_pattern_memory);
  for (uint32_t i = 0; i < mem_size_ + offset_; i++) {
    src[i] = i & 0xff;
  }

  for (uint32_t i = 0; i < dev_instance_.size(); i++) {
    for (int j = 1; j < dev_instance_[i].sub_devices.size(); j++) {
      if (!lzt::can_access_peer(dev_instance_[i].sub_devices[j - 1].dev,
                                dev_instance_[i].sub_devices[j].dev)) {
        continue;
      }
      if (!lzt::can_access_peer(dev_instance_[i].sub_devices[j].dev,
                                dev_instance_[i].sub_devices[j - 1].dev)) {
        continue;
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
        lzt::append_memory_copy(dev_instance_[i].sub_devices[j].cmd_list,
                                dev_instance_[i].sub_devices[j].src_region,
                                initial_pattern_memory, mem_size_ + src_offset);
        lzt::append_barrier(dev_instance_[i].sub_devices[j].cmd_list, nullptr,
                            0, nullptr);
        lzt::append_memory_copy(dev_instance_[i].sub_devices[j - 1].cmd_list,
                                dev_instance_[i].sub_devices[j - 1].src_region,
                                initial_pattern_memory, mem_size_ + src_offset);
        lzt::append_barrier(dev_instance_[i].sub_devices[j - 1].cmd_list,
                            nullptr, 0, nullptr);
        lzt::append_memory_copy(
            dev_instance_[i].sub_devices[j].cmd_list,
            static_cast<void *>(
                static_cast<uint8_t *>(
                    dev_instance_[i].sub_devices[j - 1].dst_region) +
                dst_offset),
            static_cast<void *>(
                static_cast<uint8_t *>(
                    dev_instance_[i].sub_devices[j].src_region) +
                src_offset),
            mem_size_, nullptr);
        lzt::append_barrier(dev_instance_[i].sub_devices[j].cmd_list, nullptr,
                            0, nullptr);
        lzt::append_memory_copy(
            dev_instance_[i].sub_devices[j - 1].cmd_list,
            static_cast<void *>(
                static_cast<uint8_t *>(
                    dev_instance_[i].sub_devices[j].dst_region) +
                dst_offset),
            static_cast<void *>(
                static_cast<uint8_t *>(
                    dev_instance_[i].sub_devices[j - 1].src_region) +
                src_offset),
            mem_size_, nullptr);
        lzt::append_barrier(dev_instance_[i].sub_devices[j - 1].cmd_list,
                            nullptr, 0, nullptr);

        lzt::append_memory_copy(
            dev_instance_[i].sub_devices[j].cmd_list, verification_memory1,
            static_cast<void *>(
                static_cast<uint8_t *>(
                    dev_instance_[i].sub_devices[j - 1].dst_region) +
                dst_offset),
            mem_size_, nullptr);
        lzt::append_barrier(dev_instance_[i].sub_devices[j].cmd_list, nullptr,
                            0, nullptr);

        lzt::append_memory_copy(
            dev_instance_[i].sub_devices[j - 1].cmd_list, verification_memory2,
            static_cast<void *>(
                static_cast<uint8_t *>(
                    dev_instance_[i].sub_devices[j].dst_region) +
                dst_offset),
            mem_size_, nullptr);
        lzt::append_barrier(dev_instance_[i].sub_devices[j - 1].cmd_list,
                            nullptr, 0, nullptr);
        lzt::close_command_list(dev_instance_[i].sub_devices[j - 1].cmd_list);
        lzt::close_command_list(dev_instance_[i].sub_devices[j].cmd_list);
        lzt::execute_command_lists(
            dev_instance_[i].sub_devices[j - 1].cmd_q, 1,
            &dev_instance_[i].sub_devices[j - 1].cmd_list, nullptr);
        lzt::execute_command_lists(dev_instance_[i].sub_devices[j].cmd_q, 1,
                                   &dev_instance_[i].sub_devices[j].cmd_list,
                                   nullptr);
        lzt::synchronize(dev_instance_[i].sub_devices[j - 1].cmd_q, UINT64_MAX);
        lzt::reset_command_list(dev_instance_[i].sub_devices[j - 1].cmd_list);
        lzt::synchronize(dev_instance_[i].sub_devices[j].cmd_q, UINT64_MAX);
        lzt::reset_command_list(dev_instance_[i].sub_devices[j].cmd_list);

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

TEST_P(
    zeP2PTests,
    GivenP2PDevicesWhenSettingAndCopyingMemoryToRemoteDeviceThenRemoteDeviceGetsCorrectMemory) {

  for (uint32_t i = 1; i < dev_instance_.size(); i++) {
    if (!lzt::can_access_peer(dev_instance_[i - 1].dev, dev_instance_[i].dev)) {
      continue;
    }
    uint8_t *shr_mem = static_cast<uint8_t *>(lzt::allocate_shared_memory(
        mem_size_ + offset_, 1, 0, 0, dev_instance_[i].dev));
    uint8_t value_before = rand() & 0xff;
    uint8_t value_after = rand() & 0xff;

    // Set memory region on device i - 1 and copy to device i

    lzt::append_memory_set(
        dev_instance_[i - 1].cmd_list,
        static_cast<void *>(
            static_cast<uint8_t *>(dev_instance_[i - 1].src_region) + offset_),
        &value_after, mem_size_);
    if (offset_ > 0) {
      lzt::append_memory_set(dev_instance_[i - 1].cmd_list,
                             dev_instance_[i - 1].src_region, &value_before,
                             offset_);
    }
    lzt::append_barrier(dev_instance_[i - 1].cmd_list, nullptr, 0, nullptr);
    lzt::append_memory_copy(
        dev_instance_[i - 1].cmd_list, dev_instance_[i].dst_region,
        static_cast<void *>(
            static_cast<uint8_t *>(dev_instance_[i - 1].src_region) + offset_),
        mem_size_, nullptr);
    lzt::close_command_list(dev_instance_[i - 1].cmd_list);
    lzt::execute_command_lists(dev_instance_[i - 1].cmd_q, 1,
                               &dev_instance_[i - 1].cmd_list, nullptr);
    lzt::synchronize(dev_instance_[i - 1].cmd_q, UINT64_MAX);
    lzt::reset_command_list(dev_instance_[i - 1].cmd_list);

    // Copy memory region from device i to shared mem, and verify it is correct
    lzt::append_memory_copy(dev_instance_[i].cmd_list, shr_mem,
                            dev_instance_[i].dst_region, mem_size_, nullptr);
    lzt::close_command_list(dev_instance_[i].cmd_list);
    lzt::execute_command_lists(dev_instance_[i].cmd_q, 1,
                               &dev_instance_[i].cmd_list, nullptr);
    lzt::synchronize(dev_instance_[i].cmd_q, UINT64_MAX);
    lzt::reset_command_list(dev_instance_[i].cmd_list);

    for (uint32_t j = 0; j < mem_size_; j++) {
      ASSERT_EQ(shr_mem[j], value_after)
          << "Memory Copied from Device did not match.";
    }
    lzt::free_memory(shr_mem);
  }
}

TEST_P(
    zeP2PTests,
    GivenP2PSubDevicesWhenSettingAndCopyingMemoryToRemoteSubDeviceThenRemoteSubDeviceGetsCorrectMemory) {

  for (uint32_t i = 0; i < dev_instance_.size(); i++) {
    for (int j = 1; j < dev_instance_[i].sub_devices.size(); j++) {
      if (!lzt::can_access_peer(dev_instance_[i].sub_devices[j - 1].dev,
                                dev_instance_[i].sub_devices[j].dev)) {
        continue;
      }
      uint8_t *shr_mem = static_cast<uint8_t *>(lzt::allocate_shared_memory(
          mem_size_ + offset_, 1, 0, 0, dev_instance_[i].sub_devices[j].dev));
      uint8_t value_before = rand() & 0xff;
      uint8_t value_after = rand() & 0xff;
      // Set memory region on device i - 1 and copy to device i

      lzt::append_memory_set(
          dev_instance_[i].sub_devices[j - 1].cmd_list,
          static_cast<void *>(
              static_cast<uint8_t *>(
                  dev_instance_[i].sub_devices[j - 1].src_region) +
              offset_),
          &value_after, mem_size_);
      if (offset_ > 0) {
        lzt::append_memory_set(dev_instance_[i].sub_devices[j - 1].cmd_list,
                               dev_instance_[i].sub_devices[j - 1].src_region,
                               &value_before, offset_);
      }
      lzt::append_barrier(dev_instance_[i].sub_devices[j - 1].cmd_list, nullptr,
                          0, nullptr);
      lzt::append_memory_copy(
          dev_instance_[i].sub_devices[j - 1].cmd_list,
          dev_instance_[i].sub_devices[j].dst_region,
          static_cast<void *>(
              static_cast<uint8_t *>(
                  dev_instance_[i].sub_devices[j - 1].src_region) +
              offset_),
          mem_size_, nullptr);
      lzt::close_command_list(dev_instance_[i].sub_devices[j - 1].cmd_list);
      lzt::execute_command_lists(dev_instance_[i].sub_devices[j - 1].cmd_q, 1,
                                 &dev_instance_[i].sub_devices[j - 1].cmd_list,
                                 nullptr);
      lzt::synchronize(dev_instance_[i].sub_devices[j - 1].cmd_q, UINT64_MAX);
      lzt::reset_command_list(dev_instance_[i].sub_devices[j - 1].cmd_list);

      // Copy memory region from device i to shared mem, and verify it is
      // correct
      lzt::append_memory_copy(dev_instance_[i].sub_devices[j].cmd_list, shr_mem,
                              dev_instance_[i].sub_devices[j].dst_region,
                              mem_size_, nullptr);
      lzt::close_command_list(dev_instance_[i].sub_devices[j].cmd_list);
      lzt::execute_command_lists(dev_instance_[i].sub_devices[j].cmd_q, 1,
                                 &dev_instance_[i].sub_devices[j].cmd_list,
                                 nullptr);
      lzt::synchronize(dev_instance_[i].sub_devices[j].cmd_q, UINT64_MAX);
      lzt::reset_command_list(dev_instance_[i].sub_devices[j].cmd_list);

      for (uint32_t k = 0; k < mem_size_; k++) {
        ASSERT_EQ(shr_mem[k], value_after)
            << "Memory Copied from SubDevice did not match.";
      }
      lzt::free_memory(shr_mem);
    }
  }
}

TEST_P(
    zeP2PTests,
    GivenP2PDevicesWhenCopyingMemoryRegionToRemoteDeviceThenRemoteDeviceGetsCorrectMemory) {

  int test_count = 0;
  const size_t num_regions = 3;

  void *initial_pattern_memory = lzt::allocate_host_memory(mem_size_ + offset_);
  void *verification_memory = lzt::allocate_host_memory(mem_size_ + offset_);

  lzt::write_data_pattern(initial_pattern_memory, mem_size_ + offset_, 1);

  std::array<size_t, num_regions> widths = {1, columns / 2, columns};
  std::array<size_t, num_regions> heights = {1, rows / 2, rows};
  std::array<size_t, num_regions> depths = {1, slices / 2, slices};

  for (int region = 0; region < num_regions; region++) {
    // Define region to be copied from/to
    auto width = widths[region];
    auto height = heights[region];
    auto depth = depths[region];

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
    for (int i = 1; i < dev_instance_.size(); i++) {

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
        lzt::append_memory_copy(ptr_dev_src->cmd_list, ptr_dev_src->src_region,
                                initial_pattern_memory, mem_size_ + src_offset);
        lzt::close_command_list(ptr_dev_src->cmd_list);
        lzt::execute_command_lists(ptr_dev_src->cmd_q, 1,
                                   &ptr_dev_src->cmd_list, nullptr);
        lzt::synchronize(ptr_dev_src->cmd_q, UINT64_MAX);
        lzt::reset_command_list(ptr_dev_src->cmd_list);

        lzt::append_memory_copy_region(
            ptr_dev_src->cmd_list,
            static_cast<uint8_t *>(ptr_dev_dst->dst_region) + dst_offset,
            &dest_region, columns, columns * rows,
            static_cast<uint8_t *>(ptr_dev_src->src_region) + src_offset,
            &src_region, columns, columns * rows, nullptr);

        lzt::close_command_list(ptr_dev_src->cmd_list);
        lzt::execute_command_lists(ptr_dev_src->cmd_q, 1,
                                   &ptr_dev_src->cmd_list, nullptr);
        lzt::synchronize(ptr_dev_src->cmd_q, UINT64_MAX);
        lzt::reset_command_list(ptr_dev_src->cmd_list);

        lzt::append_memory_copy(
            ptr_dev_dst->cmd_list, verification_memory,
            static_cast<uint8_t *>(ptr_dev_dst->dst_region) + dst_offset,
            mem_size_);
        lzt::close_command_list(ptr_dev_dst->cmd_list);
        lzt::execute_command_lists(ptr_dev_dst->cmd_q, 1,
                                   &ptr_dev_dst->cmd_list, nullptr);
        lzt::synchronize(ptr_dev_dst->cmd_q, UINT64_MAX);
        lzt::reset_command_list(ptr_dev_dst->cmd_list);

        for (int z = 0; z < depth; z++) {
          for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
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
    LOG_INFO << " No memory region copy device<-->device tests executed";
  }
  lzt::free_memory(verification_memory);
  lzt::free_memory(initial_pattern_memory);
}

TEST_P(
    zeP2PTests,
    GivenP2PSubDevicesWhenCopyingMemoryRegionToSubDeviceOnSameDeviceThenRemoteSubDeviceGetsCorrectMemory) {

  int test_count = 0;
  const size_t num_regions = 3;

  void *initial_pattern_memory = lzt::allocate_host_memory(mem_size_ + offset_);
  void *verification_memory = lzt::allocate_host_memory(mem_size_ + offset_);

  lzt::write_data_pattern(initial_pattern_memory, mem_size_ + offset_, 1);

  std::array<size_t, num_regions> widths = {1, columns / 2, columns};
  std::array<size_t, num_regions> heights = {1, rows / 2, rows};
  std::array<size_t, num_regions> depths = {1, slices / 2, slices};

  for (int region = 0; region < num_regions; region++) {
    // Define region to be copied from/to
    auto width = widths[region];
    auto height = heights[region];
    auto depth = depths[region];

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
    for (int i = 0; i < dev_instance_.size(); i++) {

      for (uint32_t d = 0; d < 2; d++) {
        size_t src_offset, dst_offset;

        if (d == 0) {
          src_offset = offset_;
          dst_offset = 0;
        } else {
          src_offset = 0;
          dst_offset = offset_;
        }

        for (int j = 1; j < dev_instance_[i].sub_devices.size(); j++) {

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
              ptr_dev_src->cmd_list, ptr_dev_src->src_region,
              initial_pattern_memory, mem_size_ + src_offset);
          lzt::close_command_list(ptr_dev_src->cmd_list);
          lzt::execute_command_lists(ptr_dev_src->cmd_q, 1,
                                     &ptr_dev_src->cmd_list, nullptr);
          lzt::synchronize(ptr_dev_src->cmd_q, UINT64_MAX);
          lzt::reset_command_list(ptr_dev_src->cmd_list);

          lzt::append_memory_copy_region(
              ptr_dev_src->cmd_list,
              static_cast<uint8_t *>(ptr_dev_dst->dst_region) + dst_offset,
              &dest_region, columns, columns * rows,
              static_cast<uint8_t *>(ptr_dev_src->src_region) + src_offset,
              &src_region, columns, columns * rows, nullptr);

          lzt::close_command_list(ptr_dev_src->cmd_list);
          lzt::execute_command_lists(ptr_dev_src->cmd_q, 1,
                                     &ptr_dev_src->cmd_list, nullptr);
          lzt::synchronize(ptr_dev_src->cmd_q, UINT64_MAX);
          lzt::reset_command_list(ptr_dev_src->cmd_list);

          lzt::append_memory_copy(
              ptr_dev_dst->cmd_list, verification_memory,
              static_cast<uint8_t *>(ptr_dev_dst->dst_region) + dst_offset,
              mem_size_);
          lzt::close_command_list(ptr_dev_dst->cmd_list);
          lzt::execute_command_lists(ptr_dev_dst->cmd_q, 1,
                                     &ptr_dev_dst->cmd_list, nullptr);
          lzt::synchronize(ptr_dev_dst->cmd_q, UINT64_MAX);
          lzt::reset_command_list(ptr_dev_dst->cmd_list);

          for (int z = 0; z < depth; z++) {
            for (int y = 0; y < height; y++) {
              for (int x = 0; x < width; x++) {
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
    LOG_INFO << "No memory copy region subdevice<-->subdevice tests executed "
                "on same device";
  }

  lzt::free_memory(verification_memory);
  lzt::free_memory(initial_pattern_memory);
}

TEST_P(
    zeP2PTests,
    GivenP2PSubDevicesWhenCopyingMemoryRegionToSubDeviceOnDifferentDeviceThenRemoteSubDeviceGetsCorrectMemory) {

  int test_count = 0;
  const size_t num_regions = 3;

  void *initial_pattern_memory = lzt::allocate_host_memory(mem_size_ + offset_);
  void *verification_memory = lzt::allocate_host_memory(mem_size_ + offset_);

  lzt::write_data_pattern(initial_pattern_memory, mem_size_ + offset_, 1);

  std::array<size_t, num_regions> widths = {1, columns / 2, columns};
  std::array<size_t, num_regions> heights = {1, rows / 2, rows};
  std::array<size_t, num_regions> depths = {1, slices / 2, slices};

  for (int region = 0; region < num_regions; region++) {
    // Define region to be copied from/to
    auto width = widths[region];
    auto height = heights[region];
    auto depth = depths[region];

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
    for (int i = 1; i < dev_instance_.size(); i++) {

      for (uint32_t d = 0; d < 2; d++) {
        size_t src_offset, dst_offset;

        if (d == 0) {
          src_offset = offset_;
          dst_offset = 0;
        } else {
          src_offset = 0;
          dst_offset = offset_;
        }

        for (int j = 0; j < dev_instance_[i].sub_devices.size(); j++) {
          for (int k = 0; k < dev_instance_[i - 1].sub_devices.size(); k++) {

            if (lzt::can_access_peer(dev_instance_[i].sub_devices[j].dev,
                                     dev_instance_[i - 1].sub_devices[k].dev)) {
              ptr_dev_src = &dev_instance_[i].sub_devices[j];
              ptr_dev_dst = &dev_instance_[i - 1].sub_devices[k];
            } else if (lzt::can_access_peer(
                           dev_instance_[i - 1].sub_devices[k].dev,
                           dev_instance_[i].sub_devices[j].dev)) {
              ptr_dev_src = &dev_instance_[i - 1].sub_devices[k];
              ptr_dev_dst = &dev_instance_[i].sub_devices[j];
            } else {
              continue;
            }

            test_count++;
            lzt::append_memory_copy(
                ptr_dev_src->cmd_list, ptr_dev_src->src_region,
                initial_pattern_memory, mem_size_ + src_offset);
            lzt::close_command_list(ptr_dev_src->cmd_list);
            lzt::execute_command_lists(ptr_dev_src->cmd_q, 1,
                                       &ptr_dev_src->cmd_list, nullptr);
            lzt::synchronize(ptr_dev_src->cmd_q, UINT64_MAX);
            lzt::reset_command_list(ptr_dev_src->cmd_list);

            lzt::append_memory_copy_region(
                ptr_dev_src->cmd_list,
                static_cast<uint8_t *>(ptr_dev_dst->dst_region) + dst_offset,
                &dest_region, columns, columns * rows,
                static_cast<uint8_t *>(ptr_dev_src->src_region) + src_offset,
                &src_region, columns, columns * rows, nullptr);

            lzt::close_command_list(ptr_dev_src->cmd_list);
            lzt::execute_command_lists(ptr_dev_src->cmd_q, 1,
                                       &ptr_dev_src->cmd_list, nullptr);
            lzt::synchronize(ptr_dev_src->cmd_q, UINT64_MAX);
            lzt::reset_command_list(ptr_dev_src->cmd_list);

            lzt::append_memory_copy(
                ptr_dev_dst->cmd_list, verification_memory,
                static_cast<uint8_t *>(ptr_dev_dst->dst_region) + dst_offset,
                mem_size_);
            lzt::close_command_list(ptr_dev_dst->cmd_list);
            lzt::execute_command_lists(ptr_dev_dst->cmd_q, 1,
                                       &ptr_dev_dst->cmd_list, nullptr);
            lzt::synchronize(ptr_dev_dst->cmd_q, UINT64_MAX);
            lzt::reset_command_list(ptr_dev_dst->cmd_list);

            for (int z = 0; z < depth; z++) {
              for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
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
    LOG_INFO << "No memory copy region subdevice<-->subdevice tests executed "
                "on different device";
  }
  lzt::free_memory(verification_memory);
  lzt::free_memory(initial_pattern_memory);
}

TEST_P(zeP2PTests,
       GivenP2PDevicesWhenKernelReadsRemoteDeviceMemoryThenCorrectDataIsRead) {

  std::string module_name = "p2p_test.spv";
  std::string func_name = "multi_device_function";

  uint32_t dev_instance_size = dev_instance_.size();

  for (uint32_t i = 1; i < dev_instance_.size(); i++) {
    if (!lzt::can_access_peer(dev_instance_[i - 1].dev, dev_instance_[i].dev)) {
      continue;
    }
    ze_module_handle_t module =
        lzt::create_module(dev_instance_[i - 1].dev, module_name);
    uint8_t *shr_mem = static_cast<uint8_t *>(lzt::allocate_shared_memory(
        mem_size_ + offset_, 1, 0, 0, dev_instance_[i].dev));

    // random memory region on device i. Allow "space" for increment.
    uint8_t value_before = rand() & 0x7f;
    uint8_t value_after = rand() & 0x7f;

    lzt::append_memory_set(
        dev_instance_[i].cmd_list,
        static_cast<void *>(
            static_cast<uint8_t *>(dev_instance_[i].src_region) + offset_),
        &value_after, mem_size_);
    if (offset_ > 0) {
      lzt::append_memory_set(dev_instance_[i].cmd_list,
                             dev_instance_[i].src_region, &value_before,
                             offset_);
    }
    lzt::append_barrier(dev_instance_[i].cmd_list, nullptr, 0, nullptr);
    lzt::close_command_list(dev_instance_[i].cmd_list);
    lzt::execute_command_lists(dev_instance_[i].cmd_q, 1,
                               &dev_instance_[i].cmd_list, nullptr);
    lzt::synchronize(dev_instance_[i].cmd_q, UINT64_MAX);
    lzt::reset_command_list(dev_instance_[i].cmd_list);

    // device (i - 1) will modify memory allocated for device i
    lzt::create_and_execute_function(
        dev_instance_[i - 1].dev, module, func_name, 1,
        static_cast<void *>(
            static_cast<uint8_t *>(dev_instance_[i].src_region) + offset_));

    // copy memory to shared region and verify it is correct
    lzt::append_memory_copy(
        dev_instance_[i].cmd_list, shr_mem,
        static_cast<void *>(
            static_cast<uint8_t *>(dev_instance_[i].src_region) + offset_),
        mem_size_, nullptr);
    lzt::close_command_list(dev_instance_[i].cmd_list);
    lzt::execute_command_lists(dev_instance_[i].cmd_q, 1,
                               &dev_instance_[i].cmd_list, nullptr);
    lzt::synchronize(dev_instance_[i].cmd_q, UINT64_MAX);
    lzt::reset_command_list(dev_instance_[i].cmd_list);
    ASSERT_EQ(shr_mem[0], value_after + 1)
        << "Memory Copied from Device did not match.";

    lzt::destroy_module(module);
  }
}

TEST_P(
    zeP2PTests,
    GivenP2PSubDevicesWhenKernelReadsRemoteSubDeviceMemoryThenCorrectDataIsRead) {

  std::string module_name = "p2p_test.spv";
  std::string func_name = "multi_device_function";

  uint32_t dev_instance_size = dev_instance_.size();

  for (uint32_t i = 0; i < dev_instance_.size(); i++) {
    for (int j = 1; j < dev_instance_[i].sub_devices.size(); j++) {
      if (!lzt::can_access_peer(dev_instance_[i].sub_devices[j - 1].dev,
                                dev_instance_[i].sub_devices[j].dev)) {
        continue;
      }

      ze_module_handle_t module = lzt::create_module(
          dev_instance_[i].sub_devices[j - 1].dev, module_name);
      uint8_t *shr_mem = static_cast<uint8_t *>(lzt::allocate_shared_memory(
          mem_size_ + offset_, 1, 0, 0, dev_instance_[i].sub_devices[j].dev));

      // random memory region on device i. Allow "space" for increment.
      uint8_t value_before = rand() & 0x7f;
      uint8_t value_after = rand() & 0x7f;

      lzt::append_memory_set(
          dev_instance_[i].sub_devices[j].cmd_list,
          static_cast<void *>(static_cast<uint8_t *>(
                                  dev_instance_[i].sub_devices[j].src_region) +
                              offset_),
          &value_after, mem_size_);
      if (offset_ > 0) {
        lzt::append_memory_set(dev_instance_[i].sub_devices[j].cmd_list,
                               dev_instance_[i].sub_devices[j].src_region,
                               &value_before, offset_);
      }
      lzt::append_barrier(dev_instance_[i].sub_devices[j].cmd_list, nullptr, 0,
                          nullptr);
      lzt::close_command_list(dev_instance_[i].sub_devices[j].cmd_list);
      lzt::execute_command_lists(dev_instance_[i].sub_devices[j].cmd_q, 1,
                                 &dev_instance_[i].sub_devices[j].cmd_list,
                                 nullptr);
      lzt::synchronize(dev_instance_[i].sub_devices[j].cmd_q, UINT64_MAX);
      lzt::reset_command_list(dev_instance_[i].sub_devices[j].cmd_list);

      // device (i - 1) will modify memory allocated for device i
      lzt::create_and_execute_function(
          dev_instance_[i].sub_devices[j - 1].dev, module, func_name, 1,
          static_cast<void *>(static_cast<uint8_t *>(
                                  dev_instance_[i].sub_devices[j].src_region) +
                              offset_));

      // copy memory to shared region and verify it is correct
      lzt::append_memory_copy(
          dev_instance_[i].sub_devices[j].cmd_list, shr_mem,
          static_cast<void *>(static_cast<uint8_t *>(
                                  dev_instance_[i].sub_devices[j].src_region) +
                              offset_),
          mem_size_, nullptr);
      lzt::append_barrier(dev_instance_[i].sub_devices[j].cmd_list, nullptr, 0,
                          nullptr);
      lzt::close_command_list(dev_instance_[i].sub_devices[j].cmd_list);
      lzt::execute_command_lists(dev_instance_[i].sub_devices[j].cmd_q, 1,
                                 &dev_instance_[i].sub_devices[j].cmd_list,
                                 nullptr);
      lzt::synchronize(dev_instance_[i].sub_devices[j].cmd_q, UINT64_MAX);
      lzt::reset_command_list(dev_instance_[i].sub_devices[j].cmd_list);
      ASSERT_EQ(shr_mem[0], value_after + 1)
          << "Memory Copied from SubDevice did not match.";
      lzt::destroy_module(module);
    }
  }
}

INSTANTIATE_TEST_CASE_P(
    GivenP2PDevicesWhenCopyingDeviceMemoryFromRemoteDeviceThenSuccessIsReturned_IP,
    zeP2PTests,
    testing::Combine(
        ::testing::Values(ZE_MEMORY_TYPE_DEVICE, ZE_MEMORY_TYPE_SHARED),
        ::testing::Values(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
                          16, 32, 64, 128, 255, 510, 1021, 2043)));

} // namespace
