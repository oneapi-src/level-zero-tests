/*
 *
 * Copyright (C) 2020-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <limits>

#include "gtest/gtest.h"

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "test_image/utils.hpp"
#include "logging/logging.hpp"

namespace lzt = level_zero_tests;

namespace {

using lzt::to_u32;

class zeCommandListAppendImageCopyWithSwizzleTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<
          std::tuple<ze_image_type_t, lzt::command_list_mode_t>> {
protected:
  void SetUp() override {
    if (!(lzt::image_support())) {
      GTEST_SKIP() << "Device does not support images";
    }
    auto device = lzt::zeDevice::get_instance()->get_device();
    module = lzt::create_module(device, "image_swizzle_tests.spv");
    supported_image_types =
        get_supported_image_types(device, skip_array_type, skip_buffer_type);
  }

  void TearDown() override {
    if (img_in != nullptr)
      lzt::destroy_ze_image(img_in);
    if (img_out != nullptr)
      lzt::destroy_ze_image(img_out);
    if (module != nullptr)
      lzt::destroy_module(module);
    img_dispatcher.apply();
  }

  void create_in_out_images(ze_image_type_t image_type);

public:
  void run_test(ze_image_type_t image_type, lzt::command_list_mode_t mode,
                bool is_shared_system);

  static const bool skip_array_type = false;
  static const bool skip_buffer_type = true;

  ze_image_handle_t img_in = nullptr, img_out = nullptr;
  ze_module_handle_t module = nullptr;
  std::vector<ze_image_type_t> supported_image_types;
  Dims image_dims;
  size_t image_size;
  ImageFuncDispatcher img_dispatcher;
};

void zeCommandListAppendImageCopyWithSwizzleTests::run_test(
    ze_image_type_t image_type, lzt::command_list_mode_t mode,
    bool is_shared_system) {
  LOG_INFO << "TYPE - " << image_type;

  std::string kernel_name = "swizzle_test_" + shortened_string(image_type);

  image_dims = get_sample_image_dims(image_type);
  image_size = static_cast<size_t>(image_dims.width * image_dims.height *
                                   image_dims.depth);

  create_in_out_images(image_type);
  if (img_dispatcher.triggered()) {
    return;
  }

  uint32_t *inbuff =
      (uint32_t *)lzt::allocate_host_memory_with_allocator_selector(
          image_size * sizeof(uint32_t), is_shared_system);
  uint32_t *outbuff =
      (uint32_t *)lzt::allocate_host_memory_with_allocator_selector(
          image_size * sizeof(uint32_t), is_shared_system);

  for (size_t i = 0U; i < image_size; i++) {
    inbuff[i] = 0x12345678;
  }

  auto bundle = lzt::create_command_bundle(mode);

  uint32_t group_size_x, group_size_y, group_size_z;
  ze_kernel_handle_t kernel = lzt::create_function(module, kernel_name);
  lzt::append_image_copy_from_mem(bundle.list, img_in, inbuff, nullptr);
  lzt::append_barrier(bundle.list, nullptr, 0, nullptr);
  lzt::suggest_group_size(kernel, to_u32(image_dims.width), image_dims.height,
                          image_dims.depth, group_size_x, group_size_y,
                          group_size_z);
  lzt::set_group_size(kernel, group_size_x, group_size_y, group_size_z);

  if (img_dispatcher
          .ze_kernel_set_argument_value(kernel, 0, sizeof(img_in), &img_in)
          .check_or_expect() ||
      img_dispatcher
          .ze_kernel_set_argument_value(kernel, 1, sizeof(img_out), &img_out)
          .check_or_expect()) {
    lzt::free_memory_with_allocator_selector(outbuff, is_shared_system);
    lzt::free_memory_with_allocator_selector(inbuff, is_shared_system);
    lzt::destroy_function(kernel);
    lzt::destroy_command_bundle(bundle);
    return;
  }

  ze_group_count_t group_dems = {to_u32(image_dims.width / group_size_x),
                                 image_dims.height / group_size_y,
                                 image_dims.depth / group_size_z};
  lzt::append_launch_function(bundle.list, kernel, &group_dems, nullptr, 0,
                              nullptr);
  lzt::append_barrier(bundle.list, nullptr, 0, nullptr);
  lzt::append_image_copy_to_mem(bundle.list, outbuff, img_out, nullptr);
  lzt::close_command_list(bundle.list);
  lzt::execute_and_sync_command_bundle(bundle, UINT64_MAX);

  for (size_t i = 0U; i < image_size; i++) {
    EXPECT_EQ(outbuff[i], 0x78563412); // After swizzle from RGBA to AGBR the
                                       // data format will be reversed from 12
                                       // 34 56 78 -> 78 56 34 12
  }

  lzt::free_memory_with_allocator_selector(outbuff, is_shared_system);
  lzt::free_memory_with_allocator_selector(inbuff, is_shared_system);
  lzt::destroy_function(kernel);
  lzt::destroy_command_bundle(bundle);

  static const bool skip_array_type = false;
  static const bool skip_buffer_type = true;
}

void zeCommandListAppendImageCopyWithSwizzleTests::create_in_out_images(
    ze_image_type_t image_type) {
  uint32_t array_levels = 0;
  if (image_type == ZE_IMAGE_TYPE_1DARRAY) {
    array_levels = image_dims.height;
  }
  if (image_type == ZE_IMAGE_TYPE_2DARRAY) {
    array_levels = image_dims.depth;
  }
  ze_image_desc_t image_descriptor_source = {};
  image_descriptor_source.stype = ZE_STRUCTURE_TYPE_IMAGE_DESC;
  image_descriptor_source.type = image_type;
  image_descriptor_source.flags =
      (ZE_IMAGE_FLAG_KERNEL_WRITE | ZE_IMAGE_FLAG_BIAS_UNCACHED);
  image_descriptor_source.format.layout = ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8;
  image_descriptor_source.format.x = ZE_IMAGE_FORMAT_SWIZZLE_R;
  image_descriptor_source.format.y = ZE_IMAGE_FORMAT_SWIZZLE_G;
  image_descriptor_source.format.z = ZE_IMAGE_FORMAT_SWIZZLE_B;
  image_descriptor_source.format.w = ZE_IMAGE_FORMAT_SWIZZLE_A;
  image_descriptor_source.width = image_dims.width;
  image_descriptor_source.height = image_dims.height;
  image_descriptor_source.depth = image_dims.depth;
  image_descriptor_source.arraylevels = array_levels;
  image_descriptor_source.format.type = ZE_IMAGE_FORMAT_TYPE_UINT;
  img_dispatcher.ze_image_create(image_descriptor_source, img_in)
      .check_or_expect();

  ze_image_desc_t image_descriptor_dest = {};
  image_descriptor_dest.stype = ZE_STRUCTURE_TYPE_IMAGE_DESC;
  image_descriptor_dest.type = image_type;
  image_descriptor_dest.flags =
      (ZE_IMAGE_FLAG_KERNEL_WRITE | ZE_IMAGE_FLAG_BIAS_UNCACHED);
  image_descriptor_dest.format.layout = ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8;
  image_descriptor_dest.format.x = ZE_IMAGE_FORMAT_SWIZZLE_A;
  image_descriptor_dest.format.y = ZE_IMAGE_FORMAT_SWIZZLE_B;
  image_descriptor_dest.format.z = ZE_IMAGE_FORMAT_SWIZZLE_G;
  image_descriptor_dest.format.w = ZE_IMAGE_FORMAT_SWIZZLE_R;
  image_descriptor_dest.width = image_dims.width;
  image_descriptor_dest.height = image_dims.height;
  image_descriptor_dest.depth = image_dims.depth;
  image_descriptor_dest.arraylevels = array_levels;
  image_descriptor_dest.format.type = ZE_IMAGE_FORMAT_TYPE_UINT;
  img_dispatcher.ze_image_create(image_descriptor_dest, img_out)
      .check_or_expect();
}

LZT_TEST_P(
    zeCommandListAppendImageCopyWithSwizzleTests,
    GivenDeviceImageAndHostImagesWithDifferentSwizzleWhenLaunchingCopyFromKernelThenImageIsCorrectAndSuccessIsReturned) {
  auto image_type = std::get<0>(GetParam());
  if (std::find(supported_image_types.begin(), supported_image_types.end(),
                image_type) == supported_image_types.end()) {
    GTEST_SKIP() << "Unsupported type: " << lzt::to_string(image_type);
  }
  auto mode = std::get<1>(GetParam());
  run_test(image_type, mode, false);
}

LZT_TEST_P(
    zeCommandListAppendImageCopyWithSwizzleTests,
    GivenDeviceImageAndHostImagesWithDifferentSwizzleWhenLaunchingCopyFromKernelThenImageIsCorrectAndSuccessIsReturnedWithSharedSystemAllocator) {
  SKIP_IF_SHARED_SYSTEM_ALLOC_UNSUPPORTED();
  auto image_type = std::get<0>(GetParam());
  if (std::find(supported_image_types.begin(), supported_image_types.end(),
                image_type) == supported_image_types.end()) {
    GTEST_SKIP() << "Unsupported type: " << lzt::to_string(image_type);
  }
  auto mode = std::get<1>(GetParam());
  run_test(image_type, mode, true);
}

INSTANTIATE_TEST_SUITE_P(
    SwizzleTestsParam, zeCommandListAppendImageCopyWithSwizzleTests,
    ::testing::Combine(::testing::ValuesIn(lzt::image_types_buffer_excluded),
                       ::testing::Values(lzt::command_list_mode_t::regular,
                                         lzt::command_list_mode_t::immediate)));

} // namespace
