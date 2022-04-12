/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef level_zero_tests_ZE_TEST_HARNESS_IMAGE_HPP
#define level_zero_tests_ZE_TEST_HARNESS_IMAGE_HPP

#include "test_harness_device.hpp"
#include <level_zero/ze_api.h>
#include "gtest/gtest.h"
#include "utils/utils.hpp"
#include "image/image.hpp"

namespace lzt = level_zero_tests;
namespace level_zero_tests {

const ze_image_flags_t image_rw_flags[2] = {0, ZE_IMAGE_FLAG_KERNEL_WRITE};
const ze_image_flags_t image_cache_flags[2] = {0, ZE_IMAGE_FLAG_BIAS_UNCACHED};

const std::vector<uint64_t> image_widths = {1, 1920};

const std::vector<uint32_t> image_heights = {1, 1080};

const std::vector<uint32_t> image_depths = {1, 8};

const std::vector<uint32_t> image_array_levels = {0, 3};

const auto image_format_types =
    ::testing::Values(ZE_IMAGE_FORMAT_TYPE_UINT, ZE_IMAGE_FORMAT_TYPE_SINT,
                      ZE_IMAGE_FORMAT_TYPE_UNORM, ZE_IMAGE_FORMAT_TYPE_SNORM,
                      ZE_IMAGE_FORMAT_TYPE_FLOAT);

const std::vector<ze_image_format_layout_t> image_format_layout_uint = {
    ZE_IMAGE_FORMAT_LAYOUT_8,           ZE_IMAGE_FORMAT_LAYOUT_8_8,
    ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8,     ZE_IMAGE_FORMAT_LAYOUT_16,
    ZE_IMAGE_FORMAT_LAYOUT_16_16,       ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16,
    ZE_IMAGE_FORMAT_LAYOUT_32,          ZE_IMAGE_FORMAT_LAYOUT_32_32,
    ZE_IMAGE_FORMAT_LAYOUT_32_32_32_32, ZE_IMAGE_FORMAT_LAYOUT_10_10_10_2};
const std::vector<ze_image_format_layout_t> image_format_layout_sint = {
    ZE_IMAGE_FORMAT_LAYOUT_8,           ZE_IMAGE_FORMAT_LAYOUT_8_8,
    ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8,     ZE_IMAGE_FORMAT_LAYOUT_16,
    ZE_IMAGE_FORMAT_LAYOUT_16_16,       ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16,
    ZE_IMAGE_FORMAT_LAYOUT_32,          ZE_IMAGE_FORMAT_LAYOUT_32_32,
    ZE_IMAGE_FORMAT_LAYOUT_32_32_32_32, ZE_IMAGE_FORMAT_LAYOUT_10_10_10_2};
const std::vector<ze_image_format_layout_t> image_format_layout_unorm = {
    ZE_IMAGE_FORMAT_LAYOUT_8,           ZE_IMAGE_FORMAT_LAYOUT_8_8,
    ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8,     ZE_IMAGE_FORMAT_LAYOUT_16,
    ZE_IMAGE_FORMAT_LAYOUT_16_16,       ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16,
    ZE_IMAGE_FORMAT_LAYOUT_32,          ZE_IMAGE_FORMAT_LAYOUT_32_32,
    ZE_IMAGE_FORMAT_LAYOUT_32_32_32_32, ZE_IMAGE_FORMAT_LAYOUT_10_10_10_2,
    ZE_IMAGE_FORMAT_LAYOUT_5_6_5,       ZE_IMAGE_FORMAT_LAYOUT_5_5_5_1,
    ZE_IMAGE_FORMAT_LAYOUT_4_4_4_4};
const std::vector<ze_image_format_layout_t> image_format_layout_snorm = {
    ZE_IMAGE_FORMAT_LAYOUT_8,           ZE_IMAGE_FORMAT_LAYOUT_8_8,
    ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8,     ZE_IMAGE_FORMAT_LAYOUT_16,
    ZE_IMAGE_FORMAT_LAYOUT_16_16,       ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16,
    ZE_IMAGE_FORMAT_LAYOUT_32,          ZE_IMAGE_FORMAT_LAYOUT_32_32,
    ZE_IMAGE_FORMAT_LAYOUT_32_32_32_32, ZE_IMAGE_FORMAT_LAYOUT_10_10_10_2};
const std::vector<ze_image_format_layout_t> image_format_layout_float = {
    ZE_IMAGE_FORMAT_LAYOUT_16,          ZE_IMAGE_FORMAT_LAYOUT_16_16,
    ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16, ZE_IMAGE_FORMAT_LAYOUT_32,
    ZE_IMAGE_FORMAT_LAYOUT_32_32,       ZE_IMAGE_FORMAT_LAYOUT_32_32_32_32,
    ZE_IMAGE_FORMAT_LAYOUT_10_10_10_2,  ZE_IMAGE_FORMAT_LAYOUT_11_11_10};

const std::vector<ze_image_format_layout_t> image_format_media_layouts = {
    ZE_IMAGE_FORMAT_LAYOUT_Y8,   ZE_IMAGE_FORMAT_LAYOUT_NV12,
    ZE_IMAGE_FORMAT_LAYOUT_YUYV, ZE_IMAGE_FORMAT_LAYOUT_VYUY,
    ZE_IMAGE_FORMAT_LAYOUT_YVYU, ZE_IMAGE_FORMAT_LAYOUT_UYVY,
    ZE_IMAGE_FORMAT_LAYOUT_AYUV, ZE_IMAGE_FORMAT_LAYOUT_P010,
    ZE_IMAGE_FORMAT_LAYOUT_Y410, ZE_IMAGE_FORMAT_LAYOUT_P012,
    ZE_IMAGE_FORMAT_LAYOUT_Y16,  ZE_IMAGE_FORMAT_LAYOUT_P016,
    ZE_IMAGE_FORMAT_LAYOUT_Y216, ZE_IMAGE_FORMAT_LAYOUT_P216};

const std::vector<ze_image_format_swizzle_t> image_format_swizzles_all = {
    ZE_IMAGE_FORMAT_SWIZZLE_R, ZE_IMAGE_FORMAT_SWIZZLE_G,
    ZE_IMAGE_FORMAT_SWIZZLE_B, ZE_IMAGE_FORMAT_SWIZZLE_A,
    ZE_IMAGE_FORMAT_SWIZZLE_0, ZE_IMAGE_FORMAT_SWIZZLE_1,
    ZE_IMAGE_FORMAT_SWIZZLE_X};

void print_image_format_descriptor(const ze_image_format_t descriptor);
void print_image_descriptor(const ze_image_desc_t descriptor);
void print_image_descriptor_unsupported(const ze_image_desc_t descriptor);
void generate_ze_image_creation_flags_list(
    std::vector<ze_image_flag_t> &image_creation_flags_list);

ze_image_handle_t create_ze_image(ze_context_handle_t context,
                                  ze_device_handle_t dev,
                                  ze_image_desc_t image_descriptor);
ze_image_handle_t create_ze_image(ze_device_handle_t dev,
                                  ze_image_desc_t image_descriptor);
ze_image_handle_t create_ze_image(ze_image_desc_t image_descriptor);

void destroy_ze_image(ze_image_handle_t image);

ze_image_properties_t get_ze_image_properties(ze_image_desc_t image_descriptor);

void copy_image_from_mem(lzt::ImagePNG32Bit input, ze_image_handle_t output);
void copy_image_to_mem(ze_image_handle_t input, lzt::ImagePNG32Bit output);

class zeImageCreateCommon {
public:
  zeImageCreateCommon();
  ~zeImageCreateCommon();
  static const ze_image_desc_t dflt_ze_image_desc;

  static const int8_t dflt_data_pattern = 1;
  level_zero_tests::ImagePNG32Bit dflt_host_image_;
  ze_image_handle_t dflt_device_image_ = nullptr;
  ze_image_handle_t dflt_device_image_2_ = nullptr;
};

// write_image_data_pattern() writes the image in the default color order,
// that I define here:
// a bits  0 ...  7
// b bits  8 ... 15
// g bits 16 ... 23
// r bits 24 ... 31
void write_image_data_pattern(lzt::ImagePNG32Bit &image, int8_t dp);
// The following uses arbitrary color order as defined in image_format:
void write_image_data_pattern(lzt::ImagePNG32Bit &image, int8_t dp,
                              const ze_image_format_t &image_format);

// Returns number of errors found, assumes default color order:
int compare_data_pattern(const lzt::ImagePNG32Bit &imagepng1,
                         const lzt::ImagePNG32Bit &imagepng2, int origin1X,
                         int origin1Y, int width1, int height1, int origin2X,
                         int origin2Y, int width2, int height2);
// Returns number of errors found, color order for both images are
// define in the image_format parameters:
int compare_data_pattern(const lzt::ImagePNG32Bit &imagepng1,
                         const ze_image_format_t &image1_format,
                         const lzt::ImagePNG32Bit &imagepng2,
                         const ze_image_format_t &image2_format, int origin1X,
                         int origin1Y, int width1, int height1, int origin2X,
                         int origin2Y, int width2, int height2);

// The following function, compares all of the pixels in image
// against the pixels that are expected in the foreground and background of the
// image. The foreground pixels are chosen when the pixels reside in the
// specified region, else the background pixels are chosen. Returns number of
// errors found:
int compare_data_pattern(
    const lzt::ImagePNG32Bit &image, const ze_image_region_t *region,
    const lzt::ImagePNG32Bit &expected_fg, // expected foreground
    const lzt::ImagePNG32Bit &expected_bg  // expected background
);
// Check if the functionality being testing is unsupported
bool check_unsupported(ze_result_t result);

}; // namespace level_zero_tests

#endif
