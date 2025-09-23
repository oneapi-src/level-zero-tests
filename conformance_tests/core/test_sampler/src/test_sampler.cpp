/*
 *
 * Copyright (C) 2019-2023 Intel Corporation
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
                     ze_bool_t, bool>> {};

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

INSTANTIATE_TEST_SUITE_P(SamplerCreationCombinations,
                         zeDeviceCreateSamplerTests,
                         ::testing::Combine(sampler_address_modes,
                                            sampler_filter_modes,
                                            ::testing::Values(true, false),
                                            ::testing::Values(false)));

void RunGivenSamplerWhenPassingAsFunctionArgumentTest(bool is_immediate) {
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
                                   module, func_name, 1U, args, is_immediate);
  lzt::destroy_module(module);
  lzt::destroy_sampler(sampler);
}

LZT_TEST(zeSamplerTests,
         GivenSamplerWhenPassingAsFunctionArgumentThenSuccessIsReturned) {
  RunGivenSamplerWhenPassingAsFunctionArgumentTest(false);
}

LZT_TEST(
    zeSamplerTests,
    GivenSamplerWhenPassingAsFunctionArgumentOnImmediateCmdListThenSuccessIsReturned) {
  RunGivenSamplerWhenPassingAsFunctionArgumentTest(true);
}

static ze_image_handle_t create_sampler_image(lzt::ImagePNG32Bit png_image,
                                              uint32_t height, uint32_t width) {
  ze_image_desc_t image_description = {};
  image_description.stype = ZE_STRUCTURE_TYPE_IMAGE_DESC;
  image_description.format.layout = ZE_IMAGE_FORMAT_LAYOUT_32;

  image_description.pNext = nullptr;
  image_description.flags = ZE_IMAGE_FLAG_KERNEL_WRITE;
  image_description.type = ZE_IMAGE_TYPE_2D;
  image_description.format.type = ZE_IMAGE_FORMAT_TYPE_UINT;
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

static ze_image_handle_t create_sampler_image(lzt::ImagePNG32Bit png_image) {
  return create_sampler_image(png_image, png_image.height(), png_image.width());
}

class zeDeviceExecuteSamplerTests : public zeDeviceCreateSamplerTests {};
LZT_TEST_P(
    zeDeviceExecuteSamplerTests,
    GivenSamplerWhenPassingAsFunctionArgumentThenOutputMatchesInKernelSampler) {
  if (!(sampler_support())) {
    LOG_INFO << "device does not support sampler, cannot run test";
    GTEST_SKIP();
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
  std::string func_name_inkernel = "sampler_inkernel";

  auto address_mode = std::get<0>(GetParam());
  auto filter_mode = std::get<1>(GetParam());
  auto normalize = std::get<2>(GetParam());
  auto is_immediate = std::get<3>(GetParam());

  auto sampler = lzt::create_sampler(address_mode, filter_mode, normalize);

  /*  sampler.spv defines a unique sampler for each combination
      of address mode/filter mode/normalized. This defines a
      mapping to select the appropriate kernel for the current
      test inputs
  */
  int address_mode_kernel, filter_mode_kernel;
  switch (address_mode) {
  case ZE_SAMPLER_ADDRESS_MODE_NONE:
    address_mode_kernel = 0;
    break;
  case ZE_SAMPLER_ADDRESS_MODE_REPEAT:
    address_mode_kernel = 1;
    break;
  case ZE_SAMPLER_ADDRESS_MODE_CLAMP:
    address_mode_kernel = 2;
    break;
  case ZE_SAMPLER_ADDRESS_MODE_MIRROR:
    address_mode_kernel = 3;
    break;
  case ZE_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER:
    address_mode_kernel = 4;
    break;
  default:
    FAIL() << "Unknown Sampler Address Mode";
    break;
  }
  switch (filter_mode) {
  case ZE_SAMPLER_FILTER_MODE_NEAREST:
    filter_mode_kernel = 0;
    break;
  case ZE_SAMPLER_FILTER_MODE_LINEAR:
    filter_mode_kernel = 1;
    break;
  default:
    FAIL() << "Unknown Sampler Filter Mode";
  }

  int idx = 0;
  // translate inputs to corresponding kernel
  idx = address_mode_kernel * 4 + filter_mode_kernel * 2 + (normalize ? 0 : 1);
  func_name_inkernel += std::to_string(idx);

  auto input_xeimage = create_sampler_image(input);
  auto output_xeimage_host =
      create_sampler_image(input, output_height, output_width);
  auto output_xeimage_kernel =
      create_sampler_image(input, output_height, output_width);

  lzt::FunctionArg arg;
  std::vector<lzt::FunctionArg> args_inkernel;
  std::vector<lzt::FunctionArg> args_inhost;

  // copy image data to input ze image
  lzt::copy_image_from_mem(input, input_xeimage);

  // input image arg
  arg.arg_size = sizeof(input_xeimage);
  arg.arg_value = &input_xeimage;
  args_inhost.push_back(arg);
  args_inkernel.push_back(arg);

  // output image arg
  arg.arg_size = sizeof(output_xeimage_host);
  arg.arg_value = &output_xeimage_host;
  args_inhost.push_back(arg);

  arg.arg_size = sizeof(output_xeimage_kernel);
  arg.arg_value = &output_xeimage_kernel;
  args_inkernel.push_back(arg);

  // sampler arg
  arg.arg_size = sizeof(sampler);
  arg.arg_value = &sampler;
  args_inhost.push_back(arg);

  lzt::create_and_execute_function(lzt::zeDevice::get_instance()->get_device(),
                                   module, func_name_inhost, 1U, args_inhost,
                                   is_immediate);

  lzt::create_and_execute_function(lzt::zeDevice::get_instance()->get_device(),
                                   module, func_name_inkernel, 1U,
                                   args_inkernel, is_immediate);

  lzt::copy_image_to_mem(output_xeimage_host, output_inhost);
  lzt::copy_image_to_mem(output_xeimage_kernel, output_inkernel);

  // compare output kernel vs host
  EXPECT_EQ(0, memcmp(output_inhost.raw_data(), output_inkernel.raw_data(),
                      output_inhost.size_in_bytes()));

  EXPECT_ZE_RESULT_SUCCESS(zeSamplerDestroy(sampler));
  lzt::destroy_module(module);
  lzt::destroy_ze_image(input_xeimage);
  lzt::destroy_ze_image(output_xeimage_host);
  lzt::destroy_ze_image(output_xeimage_kernel);
}

INSTANTIATE_TEST_SUITE_P(SamplerKernelExecuteTests, zeDeviceExecuteSamplerTests,
                         ::testing::Combine(sampler_address_modes,
                                            sampler_filter_modes,
                                            ::testing::Values(true, false),
                                            ::testing::Bool()));

} // namespace
