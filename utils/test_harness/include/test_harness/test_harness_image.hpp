/*
 *
 * Copyright (C) 2019-2025 Intel Corporation
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

const std::vector<ze_image_flags_t> image_rw_flags = {
    0, ZE_IMAGE_FLAG_KERNEL_WRITE};

const std::vector<ze_image_flags_t> image_cache_flags = {
    0, ZE_IMAGE_FLAG_BIAS_UNCACHED};

const std::vector<uint64_t> image_widths = {1, 1920};

const std::vector<uint32_t> image_heights = {1, 1080};

const std::vector<uint32_t> image_depths = {1, 8};

const std::vector<uint32_t> image_array_levels = {0, 3};

const std::vector<ze_image_format_type_t> image_format_types = {
    ZE_IMAGE_FORMAT_TYPE_UINT, ZE_IMAGE_FORMAT_TYPE_SINT,
    ZE_IMAGE_FORMAT_TYPE_UNORM, ZE_IMAGE_FORMAT_TYPE_SNORM,
    ZE_IMAGE_FORMAT_TYPE_FLOAT};

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
    ZE_IMAGE_FORMAT_LAYOUT_Y216, ZE_IMAGE_FORMAT_LAYOUT_P216,
    ZE_IMAGE_FORMAT_LAYOUT_P8,   ZE_IMAGE_FORMAT_LAYOUT_YUY2,
    ZE_IMAGE_FORMAT_LAYOUT_A8P8, ZE_IMAGE_FORMAT_LAYOUT_IA44,
    ZE_IMAGE_FORMAT_LAYOUT_AI44, ZE_IMAGE_FORMAT_LAYOUT_Y416,
    ZE_IMAGE_FORMAT_LAYOUT_Y210, ZE_IMAGE_FORMAT_LAYOUT_I420,
    ZE_IMAGE_FORMAT_LAYOUT_YV12, ZE_IMAGE_FORMAT_LAYOUT_400P,
    ZE_IMAGE_FORMAT_LAYOUT_422H, ZE_IMAGE_FORMAT_LAYOUT_422V,
    ZE_IMAGE_FORMAT_LAYOUT_444P, ZE_IMAGE_FORMAT_LAYOUT_RGBP,
    ZE_IMAGE_FORMAT_LAYOUT_BRGP};

const std::vector<ze_image_type_t> image_types = {
    ZE_IMAGE_TYPE_1D,      ZE_IMAGE_TYPE_1DARRAY, ZE_IMAGE_TYPE_2D,
    ZE_IMAGE_TYPE_2DARRAY, ZE_IMAGE_TYPE_3D,      ZE_IMAGE_TYPE_BUFFER};

const std::vector<ze_image_type_t> image_types_buffer_excluded = {
    ZE_IMAGE_TYPE_1D, ZE_IMAGE_TYPE_1DARRAY, ZE_IMAGE_TYPE_2D,
    ZE_IMAGE_TYPE_2DARRAY, ZE_IMAGE_TYPE_3D};

const std::vector<ze_image_format_swizzle_t> image_format_swizzles_all = {
    ZE_IMAGE_FORMAT_SWIZZLE_R, ZE_IMAGE_FORMAT_SWIZZLE_G,
    ZE_IMAGE_FORMAT_SWIZZLE_B, ZE_IMAGE_FORMAT_SWIZZLE_A,
    ZE_IMAGE_FORMAT_SWIZZLE_0, ZE_IMAGE_FORMAT_SWIZZLE_1,
    ZE_IMAGE_FORMAT_SWIZZLE_X};

ze_result_t create_ze_image(ze_context_handle_t context, ze_device_handle_t dev,
                            ze_image_desc_t image_descriptor,
                            ze_image_handle_t &image);
ze_result_t create_ze_image(ze_device_handle_t dev,
                            ze_image_desc_t image_descriptor,
                            ze_image_handle_t &image);
ze_result_t create_ze_image(ze_image_desc_t image_descriptor,
                            ze_image_handle_t &image);

// Convenience overloads returning the created handle directly, for callers that
// do not need the ze_result_t. Prefer the ze_result_t-returning variants above
// in new code.
ze_image_handle_t create_ze_image(ze_context_handle_t context,
                                  ze_device_handle_t dev,
                                  ze_image_desc_t image_descriptor);
ze_image_handle_t create_ze_image(ze_device_handle_t dev,
                                  ze_image_desc_t image_descriptor);
ze_image_handle_t create_ze_image(ze_image_desc_t image_descriptor);

void destroy_ze_image(ze_image_handle_t image);

ze_result_t create_ze_image_view_ext(ze_device_handle_t device,
                                     const ze_image_desc_t *desc,
                                     ze_image_handle_t image,
                                     ze_image_handle_t &image_view);
ze_image_handle_t create_ze_image_view_ext(ze_device_handle_t device,
                                           const ze_image_desc_t *desc,
                                           ze_image_handle_t image);

ze_image_properties_t get_ze_image_properties(ze_image_desc_t image_descriptor,
                                              ze_result_t *result = nullptr);

ze_image_allocation_ext_properties_t
get_ze_image_alloc_properties_ext(ze_image_handle_t image);

ze_image_memory_properties_exp_t
get_ze_image_mem_properties_exp(ze_image_handle_t image);

uint64_t get_image_device_offset_exp(ze_image_handle_t image,
                                     uint64_t *device_offset);

}; // namespace level_zero_tests

#endif
