/*
 *
 * Copyright (C) 2025 Intel Corporation
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

struct SharedSystemRemoteDeviceTests : public ::testing::TestWithParam<bool> {
  void SetUp() override {
    auto driver = lzt::get_default_driver();
    auto context = lzt::get_default_context();
    auto devices = lzt::get_ze_devices(driver);

    devices.erase(
        std::remove_if(
            begin(devices), end(devices),
            [](auto dev) { return !lzt::supports_shared_system_alloc(dev); }),
        end(devices));

    LOG_INFO << "num_devices: " << devices.size();
    if (devices.size() < 2) {
      GTEST_SKIP() << "Test requires at least two devices that support shared "
                      "system allocations.";
    }

    device = devices[0];
    remote_device = devices[1];

    ASSERT_TRUE(lzt::can_access_peer(device, remote_device));

    bool is_immediate = GetParam();
    cmd_bundle = lzt::create_command_bundle(device, is_immediate);
  }

  void TearDown() override {
    if (cmd_bundle.list != nullptr) {
      lzt::destroy_command_bundle(cmd_bundle);
    }
  }

  void run_append_memory_fill_test(bool use_madvise) {
    void *memory = lzt::aligned_malloc(size, 1);
    memset(memory, 0, size);

    if (use_madvise) {
      lzt::append_memory_advise(cmd_bundle.list, remote_device, memory, size,
                                ZE_MEMORY_ADVICE_SET_PREFERRED_LOCATION);
    }
    uint8_t pattern = 0xAB;
    lzt::append_memory_fill(cmd_bundle.list, memory, &pattern, sizeof(pattern),
                            size, nullptr);

    lzt::close_command_list(cmd_bundle.list);
    lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

    uint8_t *memory_as_byte = static_cast<uint8_t *>(memory);
    for (size_t i = 0; i < size; i++) {
      ASSERT_EQ(memory_as_byte[i], pattern) << "Memory not matching at: " << i;
    }

    lzt::aligned_free(memory);
  }

  void run_append_memory_copy_test(bool use_madvise_for_src,
                                   bool use_madvise_for_dst) {
    void *src_memory = lzt::aligned_malloc(size, 1);
    void *dst_memory = lzt::aligned_malloc(size, 1);
    memset(src_memory, 0xAB, size);
    memset(dst_memory, 0, size);

    if (use_madvise_for_src) {
      lzt::append_memory_advise(cmd_bundle.list, remote_device, src_memory,
                                size, ZE_MEMORY_ADVICE_SET_PREFERRED_LOCATION);
    }
    if (use_madvise_for_dst) {
      lzt::append_memory_advise(cmd_bundle.list, remote_device, dst_memory,
                                size, ZE_MEMORY_ADVICE_SET_PREFERRED_LOCATION);
    }

    lzt::append_memory_copy(cmd_bundle.list, dst_memory, src_memory, size);

    lzt::close_command_list(cmd_bundle.list);
    lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

    uint8_t *src_memory_as_byte = static_cast<uint8_t *>(src_memory);
    uint8_t *dst_memory_as_byte = static_cast<uint8_t *>(dst_memory);
    for (size_t i = 0; i < size; i++) {
      ASSERT_EQ(src_memory_as_byte[i], dst_memory_as_byte[i])
          << "Memory not matching at: " << i;
    }

    lzt::aligned_free(src_memory);
    lzt::aligned_free(dst_memory);
  }

  void run_append_memory_copy_region_test(bool use_madvise_for_src,
                                          bool use_madvise_for_dst) {
    void *src_memory = lzt::aligned_malloc(size, 1);
    void *dst_memory = lzt::aligned_malloc(size, 1);
    memset(src_memory, 0xAB, size);
    memset(dst_memory, 0, size);

    if (use_madvise_for_src) {
      lzt::append_memory_advise(cmd_bundle.list, remote_device, src_memory,
                                size, ZE_MEMORY_ADVICE_SET_PREFERRED_LOCATION);
    }
    if (use_madvise_for_dst) {
      lzt::append_memory_advise(cmd_bundle.list, remote_device, dst_memory,
                                size, ZE_MEMORY_ADVICE_SET_PREFERRED_LOCATION);
    }

    constexpr size_t region_size = size / 2;
    ze_copy_region_t region;
    region.originX = 0;
    region.originY = 0;
    region.originZ = 0;
    region.width = region_size;
    region.height = 1;
    region.depth = 1;

    lzt::append_memory_copy_region(cmd_bundle.list, dst_memory, &region, 1, 1,
                                   src_memory, &region, 1, 1, nullptr);
    lzt::close_command_list(cmd_bundle.list);
    lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

    uint8_t *src_memory_as_byte = static_cast<uint8_t *>(src_memory);
    uint8_t *dst_memory_as_byte = static_cast<uint8_t *>(dst_memory);
    for (size_t i = 0; i < region_size; i++) {
      ASSERT_EQ(src_memory_as_byte[i], dst_memory_as_byte[i])
          << "Memory not matching at: " << i;
    }

    lzt::aligned_free(src_memory);
    lzt::aligned_free(dst_memory);
  }

  void run_append_memory_image_copy_test(bool use_madvise_for_src,
                                         bool use_madvise_for_dst) {
    auto png_img_src = lzt::ImagePNG32Bit("test_input.png");
    auto image_width = png_img_src.width();
    auto image_height = png_img_src.height();
    auto image_size = image_width * image_height * sizeof(uint32_t);
    auto png_img_dest = lzt::ImagePNG32Bit(image_width, image_height);

    ze_image_desc_t img_desc = {};
    img_desc.stype = ZE_STRUCTURE_TYPE_IMAGE_DESC;
    img_desc.flags = ZE_IMAGE_FLAG_KERNEL_WRITE;
    img_desc.type = ZE_IMAGE_TYPE_2D;
    img_desc.format = {
        ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8, ZE_IMAGE_FORMAT_TYPE_UNORM,
        ZE_IMAGE_FORMAT_SWIZZLE_R,      ZE_IMAGE_FORMAT_SWIZZLE_G,
        ZE_IMAGE_FORMAT_SWIZZLE_B,      ZE_IMAGE_FORMAT_SWIZZLE_A};
    img_desc.width = image_width;
    img_desc.height = image_height;
    img_desc.depth = 1;
    img_desc.arraylevels = 0;
    img_desc.miplevels = 0;

    auto ze_img = lzt::create_ze_image(device, img_desc);

    if (use_madvise_for_src) {
      lzt::append_memory_advise(cmd_bundle.list, remote_device,
                                png_img_src.raw_data(), size,
                                ZE_MEMORY_ADVICE_SET_PREFERRED_LOCATION);
    }
    if (use_madvise_for_dst) {
      lzt::append_memory_advise(cmd_bundle.list, remote_device,
                                png_img_dest.raw_data(), size,
                                ZE_MEMORY_ADVICE_SET_PREFERRED_LOCATION);
    }

    lzt::append_image_copy_from_mem(cmd_bundle.list, ze_img,
                                    png_img_src.raw_data(), nullptr);
    lzt::append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
    lzt::append_image_copy_to_mem(cmd_bundle.list, png_img_dest.raw_data(),
                                  ze_img, nullptr);
    lzt::append_barrier(cmd_bundle.list, nullptr, 0, nullptr);
    lzt::close_command_list(cmd_bundle.list);
    lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

    EXPECT_EQ(png_img_src, png_img_dest);
  }

  void run_append_launch_kernel_test(bool use_madvise_for_src,
                                     bool use_madvise_for_dst,
                                     bool use_cooperative) {
    size_t buffer_size = size * sizeof(int);
    void *src_memory = lzt::aligned_malloc(buffer_size, sizeof(int));
    void *dst_memory = lzt::aligned_malloc(buffer_size, sizeof(int));
    memset(src_memory, 0xAB, buffer_size);
    memset(dst_memory, 0, size);

    auto module = lzt::create_module(device, "copy_module.spv");
    auto kernel = lzt::create_function(module, "copy_data");

    uint32_t offset = 0;
    lzt::set_group_size(kernel, 1, 1, 1);
    lzt::set_argument_value(kernel, 0, sizeof(src_memory), &src_memory);
    lzt::set_argument_value(kernel, 1, sizeof(dst_memory), &dst_memory);
    lzt::set_argument_value(kernel, 2, sizeof(offset), &offset);
    lzt::set_argument_value(kernel, 3, sizeof(size), &size);

    if (use_madvise_for_src) {
      lzt::append_memory_advise(cmd_bundle.list, remote_device, src_memory,
                                size, ZE_MEMORY_ADVICE_SET_PREFERRED_LOCATION);
    }
    if (use_madvise_for_dst) {
      lzt::append_memory_advise(cmd_bundle.list, remote_device, dst_memory,
                                size, ZE_MEMORY_ADVICE_SET_PREFERRED_LOCATION);
    }

    ze_group_count_t group_count{1, 1, 1};
    if (use_cooperative) {
      lzt::append_launch_cooperative_function(
          cmd_bundle.list, kernel, &group_count, nullptr, 0, nullptr);
    } else {
      lzt::append_launch_function(cmd_bundle.list, kernel, &group_count,
                                  nullptr, 0, nullptr);
    }

    lzt::close_command_list(cmd_bundle.list);
    lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

    int *src_memory_as_int = static_cast<int *>(src_memory);
    int *dst_memory_as_int = static_cast<int *>(dst_memory);
    for (size_t i = 0; i < size; i++) {
      ASSERT_EQ(src_memory_as_int[i], dst_memory_as_int[i])
          << "Memory not matching at: " << i;
    }

    lzt::destroy_function(kernel);
    lzt::destroy_module(module);
    lzt::aligned_free(src_memory);
    lzt::aligned_free(dst_memory);
  }

  static constexpr size_t size = 1024;
  ze_device_handle_t device, remote_device;
  lzt::zeCommandBundle cmd_bundle;
};

LZT_TEST_P(
    SharedSystemRemoteDeviceTests,
    GivenMultipleDevicesAndSharedSystemMemoryAdvisedToRemoteDeviceWhenAppendingMemoryFillThenIsSuccessAndValuesAreCorrect) {
  run_append_memory_fill_test(true);
}

LZT_TEST_P(
    SharedSystemRemoteDeviceTests,
    GivenMultipleDevicesAndSrcSharedSystemMemoryAdvisedToRemoteDeviceWhenAppendingMemoryCopyThenIsSuccessAndValuesAreCorrect) {
  run_append_memory_copy_test(true, false);
}

LZT_TEST_P(
    SharedSystemRemoteDeviceTests,
    GivenMultipleDevicesAndDstSharedSystemMemoryAdvisedToRemoteDeviceWhenAppendingMemoryCopyThenIsSuccessAndValuesAreCorrect) {
  run_append_memory_copy_test(false, true);
}

LZT_TEST_P(
    SharedSystemRemoteDeviceTests,
    GivenMultipleDevicesAndSrcAndDstSharedSystemMemoryAdvisedToRemoteDeviceWhenAppendingMemoryCopyThenIsSuccessAndValuesAreCorrect) {
  run_append_memory_copy_test(true, true);
}

LZT_TEST_P(
    SharedSystemRemoteDeviceTests,
    GivenMultipleDevicesAndSrcSharedSystemMemoryAdvisedToRemoteDeviceWhenAppendingMemoryCopyRegionThenIsSuccessAndValuesAreCorrect) {
  run_append_memory_copy_region_test(true, false);
}

LZT_TEST_P(
    SharedSystemRemoteDeviceTests,
    GivenMultipleDevicesAndDstSharedSystemMemoryAdvisedToRemoteDeviceWhenAppendingMemoryCopyRegionThenIsSuccessAndValuesAreCorrect) {
  run_append_memory_copy_region_test(false, true);
}

LZT_TEST_P(
    SharedSystemRemoteDeviceTests,
    GivenMultipleDevicesAndSrcAndDstSharedSystemMemoryAdvisedToRemoteDeviceWhenAppendingMemoryCopyRegionThenIsSuccessAndValuesAreCorrect) {
  run_append_memory_copy_region_test(true, true);
}

LZT_TEST_P(
    SharedSystemRemoteDeviceTests,
    GivenMultipleDevicesAndSrcSharedSystemMemoryAdvisedToRemoteDeviceWhenAppendingImageCopyToAndFromMemoryThenIsSuccessAndImageIsCorrect) {
  if (!(lzt::image_support(device))) {
    GTEST_SKIP() << "Test requires image support.";
  }
  run_append_memory_image_copy_test(true, false);
}

LZT_TEST_P(
    SharedSystemRemoteDeviceTests,
    GivenMultipleDevicesAndDstSharedSystemMemoryAdvisedToRemoteDeviceWhenAppendingImageCopyToAndFromMemoryThenIsSuccessAndImageIsCorrect) {
  if (!(lzt::image_support(device))) {
    GTEST_SKIP() << "Test requires image support.";
  }
  run_append_memory_image_copy_test(false, true);
}

LZT_TEST_P(
    SharedSystemRemoteDeviceTests,
    GivenMultipleDevicesAndSrcAndDstSharedSystemMemoryAdvisedToRemoteDeviceWhenAppendingImageCopyToAndFromMemoryThenIsSuccessAndImageIsCorrect) {
  if (!(lzt::image_support(device))) {
    GTEST_SKIP() << "Test requires image support.";
  }
  run_append_memory_image_copy_test(true, true);
}

LZT_TEST_P(
    SharedSystemRemoteDeviceTests,
    GivenMultipleDevicesAndSrcSharedSystemMemoryAdvisedToRemoteDeviceWhenAppendingKernelCopyThenIsSuccessAndValuesAreCorrect) {
  run_append_launch_kernel_test(true, false, false);
}

LZT_TEST_P(
    SharedSystemRemoteDeviceTests,
    GivenMultipleDevicesAndDstSharedSystemMemoryAdvisedToRemoteDeviceWhenAppendingKernelCopyThenIsSuccessAndValuesAreCorrect) {
  run_append_launch_kernel_test(false, true, false);
}

LZT_TEST_P(
    SharedSystemRemoteDeviceTests,
    GivenMultipleDevicesAndSrcAndDstSharedSystemMemoryAdvisedToRemoteDeviceWhenAppendingKernelCopyThenIsSuccessAndValuesAreCorrect) {
  run_append_launch_kernel_test(true, true, false);
}

LZT_TEST_P(
    SharedSystemRemoteDeviceTests,
    GivenMultipleDevicesAndSrcSharedSystemMemoryAdvisedToRemoteDeviceWhenAppendingCooperativeKernelCopyThenIsSuccessAndValuesAreCorrect) {
  run_append_launch_kernel_test(true, false, true);
}

LZT_TEST_P(
    SharedSystemRemoteDeviceTests,
    GivenMultipleDevicesAndDstSharedSystemMemoryAdvisedToRemoteDeviceWhenAppendingCooperativeKernelCopyThenIsSuccessAndValuesAreCorrect) {
  run_append_launch_kernel_test(false, true, true);
}

LZT_TEST_P(
    SharedSystemRemoteDeviceTests,
    GivenMultipleDevicesAndSrcAndDstSharedSystemMemoryAdvisedToRemoteDeviceWhenAppendingCooperativeKernelCopyThenIsSuccessAndValuesAreCorrect) {
  run_append_launch_kernel_test(true, true, true);
}

INSTANTIATE_TEST_SUITE_P(SharedSystemRemoteDeviceTestParam,
                         SharedSystemRemoteDeviceTests, ::testing::Bool());

} // namespace
