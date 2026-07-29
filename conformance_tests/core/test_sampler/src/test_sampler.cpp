/*
 *
 * Copyright (C) 2019-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include <algorithm>
#include <cctype>

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "test_image/utils.hpp"
#include "logging/logging.hpp"

namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>
#include "image/image.hpp"

namespace {

bool sampler_support() {
  ze_device_image_properties_t properties{};
  properties.stype = ZE_STRUCTURE_TYPE_IMAGE_PROPERTIES;
  properties.pNext = nullptr;
  ze_result_t result = zeDeviceGetImageProperties(
      lzt::zeDevice::get_instance()->get_device(), &properties);
  if ((result != ZE_RESULT_SUCCESS) || (properties.maxSamplers == 0)) {
    return false;
  } else {
    return true;
  }
}

const auto sampler_address_modes = ::testing::Values(
    ZE_SAMPLER_ADDRESS_MODE_NONE, ZE_SAMPLER_ADDRESS_MODE_REPEAT,
    ZE_SAMPLER_ADDRESS_MODE_CLAMP, ZE_SAMPLER_ADDRESS_MODE_MIRROR,
    ZE_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);

const auto sampler_filter_modes = ::testing::Values(
    ZE_SAMPLER_FILTER_MODE_NEAREST, ZE_SAMPLER_FILTER_MODE_LINEAR);

class zeDeviceCreateSamplerTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<
          std::tuple<ze_sampler_address_mode_t, ze_sampler_filter_mode_t,
                     ze_bool_t, lzt::command_list_mode_t>> {};

LZT_TEST_P(
    zeDeviceCreateSamplerTests,
    GivenSamplerDescriptorWhenCreatingSamplerThenNotNullSamplerIsReturned) {
  if (!(sampler_support())) {
    LOG_INFO << "device does not support sampler, cannot run test";
    GTEST_SKIP();
  }
  ze_sampler_desc_t descriptor = {};
  descriptor.stype = ZE_STRUCTURE_TYPE_SAMPLER_DESC;

  descriptor.pNext = nullptr;
  descriptor.addressMode = std::get<0>(GetParam());
  descriptor.filterMode = std::get<1>(GetParam());
  descriptor.isNormalized = std::get<2>(GetParam());

  ze_sampler_handle_t sampler = nullptr;
  EXPECT_ZE_RESULT_SUCCESS(zeSamplerCreate(
      lzt::get_default_context(), lzt::zeDevice::get_instance()->get_device(),
      &descriptor, &sampler));

  EXPECT_NE(nullptr, sampler);

  EXPECT_ZE_RESULT_SUCCESS(zeSamplerDestroy(sampler));
}

INSTANTIATE_TEST_SUITE_P(
    SamplerCreationCombinations, zeDeviceCreateSamplerTests,
    ::testing::Combine(sampler_address_modes, sampler_filter_modes,
                       ::testing::Values(true, false),
                       ::testing::Values(lzt::command_list_mode_t::regular)));

template <lzt::command_list_mode_t Mode>
void RunGivenSamplerWhenPassingAsFunctionArgumentTest() {
  if (!(sampler_support())) {
    LOG_INFO << "device does not support sampler, cannot run test";
    GTEST_SKIP();
  }
  ze_sampler_handle_t sampler = lzt::create_sampler();

  std::string module_name = "sampler.spv";
  ze_module_handle_t module = lzt::create_module(
      lzt::zeDevice::get_instance()->get_device(), module_name);
  std::string func_name = "sampler_noop";

  lzt::FunctionArg arg;
  std::vector<lzt::FunctionArg> args;

  arg.arg_size = sizeof(sampler);
  arg.arg_value = &sampler;
  args.push_back(arg);

  lzt::create_and_execute_function(lzt::zeDevice::get_instance()->get_device(),
                                   module, func_name, 1U, args, Mode);
  lzt::destroy_module(module);
  lzt::destroy_sampler(sampler);
}

LZT_TEST(zeSamplerTests,
         GivenSamplerWhenPassingAsFunctionArgumentThenSuccessIsReturned) {
  RunGivenSamplerWhenPassingAsFunctionArgumentTest<
      lzt::command_list_mode_t::regular>();
}

LZT_TEST(
    zeSamplerTests,
    GivenSamplerWhenPassingAsFunctionArgumentOnImmediateCmdListThenSuccessIsReturned) {
  RunGivenSamplerWhenPassingAsFunctionArgumentTest<
      lzt::command_list_mode_t::immediate>();
}

static ze_image_handle_t create_sampler_image(uint32_t height, uint32_t width) {
  ze_image_desc_t image_description = {};
  image_description.stype = ZE_STRUCTURE_TYPE_IMAGE_DESC;
  image_description.format.layout = ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8;

  image_description.pNext = nullptr;
  image_description.flags = ZE_IMAGE_FLAG_KERNEL_WRITE;
  image_description.type = ZE_IMAGE_TYPE_2D;
  image_description.format.type = ZE_IMAGE_FORMAT_TYPE_UNORM;
  image_description.format.x = ZE_IMAGE_FORMAT_SWIZZLE_R;
  image_description.format.y = ZE_IMAGE_FORMAT_SWIZZLE_G;
  image_description.format.z = ZE_IMAGE_FORMAT_SWIZZLE_B;
  image_description.format.w = ZE_IMAGE_FORMAT_SWIZZLE_A;
  image_description.width = width;
  image_description.height = height;
  image_description.depth = 1;
  ze_image_handle_t image = lzt::create_ze_image(image_description);

  return image;
}

class zeDeviceExecuteSamplerTests : public zeDeviceCreateSamplerTests {};
LZT_TEST_P(
    zeDeviceExecuteSamplerTests,
    GivenSamplerWhenPassingAsFunctionArgumentThenOutputMatchesInKernelSampler) {
  if (!(sampler_support())) {
    LOG_INFO << "device does not support sampler, cannot run test";
    GTEST_SKIP();
  }

  auto address_mode = std::get<0>(GetParam());
  auto filter_mode = std::get<1>(GetParam());
  auto normalize = std::get<2>(GetParam());
  auto mode = std::get<3>(GetParam());

  LOG_DEBUG << "address_mode = " << lzt::to_string(address_mode)
            << " filter_mode = " << lzt::to_string(filter_mode)
            << " normalize = " << lzt::to_string(normalize)
            << " mode = " << lzt::to_string(mode);

  if (!normalize) {
    if (address_mode == ZE_SAMPLER_ADDRESS_MODE_REPEAT ||
        address_mode == ZE_SAMPLER_ADDRESS_MODE_MIRROR) {
      GTEST_SKIP() << "REPEAT/MIRROR address modes require normalized "
                      "coordinates";
    }
    if (address_mode == ZE_SAMPLER_ADDRESS_MODE_NONE &&
        filter_mode == ZE_SAMPLER_FILTER_MODE_LINEAR) {
      GTEST_SKIP()
          << "ADDRESS_MODE_NONE with linear filtering and unnormalized "
             "coordinates samples out of bounds (ub)";
    }
  }

  lzt::ImagePNG32Bit input("test_input.png");
  uint32_t output_width = input.width() / 2;
  uint32_t output_height = input.height() / 2;
  lzt::ImagePNG32Bit output_inhost(output_width, output_height);
  lzt::ImagePNG32Bit output_inkernel(output_width, output_height);
  std::string module_name = "sampler.spv";
  ze_module_handle_t module = lzt::create_module(
      lzt::zeDevice::get_instance()->get_device(), module_name);
  std::string func_name_inhost = "sampler_inhost";

  auto sampler = lzt::create_sampler(address_mode, filter_mode, normalize);

  // ex. ZE_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER -> clamp_to_border
  auto enum_suffix = [](const std::string &enum_name) {
    const auto tokens = lzt::split_string(enum_name, "_");
    std::vector<std::string> parts(tokens.begin() + 4, tokens.end());
    auto suffix = lzt::join_strings(parts, "_");
    std::transform(suffix.begin(), suffix.end(), suffix.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return suffix;
  };

  const std::string func_name_inkernel =
      "sampler_inkernel_adr_" + enum_suffix(lzt::to_string(address_mode)) +
      "_filter_" + enum_suffix(lzt::to_string(filter_mode)) +
      (normalize ? "_normalized" : "_unnormalized");

  LOG_INFO << "kernel_name = " << func_name_inkernel;

  auto input_xeimage = create_sampler_image(input.height(), input.width());
  auto output_xeimage_host = create_sampler_image(output_height, output_width);
  auto output_xeimage_kernel =
      create_sampler_image(output_height, output_width);

  auto kernel_const_sampler_f =
      lzt::create_function(module, func_name_inkernel);
  auto host_input_sampler_f = lzt::create_function(module, func_name_inhost);

  // copy image data to input ze image
  lzt::copy_image_from_mem(input, input_xeimage);

  // input image arg
  lzt::set_argument_value(kernel_const_sampler_f, 0, sizeof(input_xeimage),
                          &input_xeimage);
  lzt::set_argument_value(host_input_sampler_f, 0, sizeof(input_xeimage),
                          &input_xeimage);

  // output image arg
  lzt::set_argument_value(kernel_const_sampler_f, 1,
                          sizeof(output_xeimage_kernel),
                          &output_xeimage_kernel);
  lzt::set_argument_value(host_input_sampler_f, 1, sizeof(output_xeimage_host),
                          &output_xeimage_host);

  // sampler arg
  lzt::set_argument_value(host_input_sampler_f, 2, sizeof(sampler), &sampler);

  auto bundle = lzt::create_command_bundle(
      lzt::zeDevice::get_instance()->get_device(), mode);

  lzt::set_group_size(kernel_const_sampler_f, 16, 16, 1);
  lzt::set_group_size(host_input_sampler_f, 16, 16, 1);
  ze_group_count_t group_count;
  group_count.groupCountX = output_width / 16;
  group_count.groupCountY = output_height / 16;
  group_count.groupCountZ = 1;

  lzt::append_launch_function(bundle.record_list(), kernel_const_sampler_f,
                              &group_count, nullptr, 0, nullptr);
  lzt::execute_and_sync_command_bundle(bundle,
                                       std::numeric_limits<uint64_t>::max());

  lzt::reset_command_bundle(bundle);

  lzt::append_launch_function(bundle.record_list(), host_input_sampler_f,
                              &group_count, nullptr, 0, nullptr);
  lzt::execute_and_sync_command_bundle(bundle,
                                       std::numeric_limits<uint64_t>::max());

  lzt::copy_image_to_mem(output_xeimage_host, output_inhost);
  lzt::copy_image_to_mem(output_xeimage_kernel, output_inkernel);

  // compare output kernel vs host
  EXPECT_EQ(0, memcmp(output_inhost.raw_data(), output_inkernel.raw_data(),
                      output_inhost.size_in_bytes()));

  auto has_nonzero = [](const lzt::ImagePNG32Bit &img) {
    const uint32_t *data = img.raw_data();
    return std::any_of(data, data + img.size(),
                       [](uint32_t pixel) { return pixel != 0; });
  };
  EXPECT_TRUE(has_nonzero(output_inkernel))
      << "in-kernel sampler produced an all-zero output image";
  EXPECT_TRUE(has_nonzero(output_inhost))
      << "host sampler produced an all-zero output image";

  lzt::destroy_sampler(sampler);
  lzt::destroy_function(kernel_const_sampler_f);
  lzt::destroy_function(host_input_sampler_f);
  lzt::destroy_module(module);
  lzt::destroy_ze_image(input_xeimage);
  lzt::destroy_ze_image(output_xeimage_host);
  lzt::destroy_ze_image(output_xeimage_kernel);
  lzt::destroy_command_bundle(bundle);
}

INSTANTIATE_TEST_SUITE_P(
    SamplerKernelExecuteTests, zeDeviceExecuteSamplerTests,
    ::testing::Combine(sampler_address_modes, sampler_filter_modes,
                       ::testing::Values(true, false),
                       ::testing::Values(lzt::command_list_mode_t::regular,
                                         lzt::command_list_mode_t::immediate)));

} // namespace
