/*
 *
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef CONFORMANCE_TESTS_CORE_TEST_IMAGE_INCLUDE_TEST_IMAGE_UTILS_HPP
#define CONFORMANCE_TESTS_CORE_TEST_IMAGE_INCLUDE_TEST_IMAGE_UTILS_HPP

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

#include "image/image.hpp"
#include "test_harness/test_harness.hpp"
#include <level_zero/ze_api.h>

struct Dims {
  uint64_t width;
  uint32_t height;
  uint32_t depth;
};

std::string shortened_string(ze_image_type_t type);

Dims get_sample_image_dims(ze_image_type_t image_type);

std::vector<ze_image_type_t>
get_supported_image_types(ze_device_handle_t device,
                          bool exclude_arrays = false,
                          bool exclude_buffer = false);

class ImageFuncDispatcher;
class ImageCall {
public:
  ImageCall(ImageFuncDispatcher &owner, ze_result_t result, const char *api)
      : owner_(owner), result_(result), api_(api) {}

  ze_result_t result() const { return result_; }

  bool check_or_expect();
  template <typename F> bool check_or_expect(F &&on_skip) {
    if (check_or_expect()) {
      on_skip();
      return true;
    }
    return false;
  }

private:
  ImageFuncDispatcher &owner_;
  ze_result_t result_;
  const char *api_;
};

class ImageFuncDispatcher {
public:
  ImageCall ze_image_create(ze_context_handle_t context,
                            ze_device_handle_t device,
                            ze_image_desc_t image_descriptor,
                            ze_image_handle_t &out);
  ImageCall ze_image_create(ze_device_handle_t device,
                            ze_image_desc_t image_descriptor,
                            ze_image_handle_t &out);
  ImageCall ze_image_create(ze_image_desc_t image_descriptor,
                            ze_image_handle_t &out);

  ImageCall ze_image_get_properties(ze_image_desc_t image_descriptor,
                                    ze_image_properties_t *out);
  ImageCall ze_kernel_set_argument_value(ze_kernel_handle_t kernel,
                                         uint32_t index, size_t size,
                                         const void *value);

  // Escape hatch: adopt a result produced out-of-band into an ImageCall, for
  // the few sites where the create cannot go through a wrapper -- a
  // cross-process export/import result, or a create issued from a worker
  // thread.
  ImageCall check(ze_result_t result, const char *api) {
    return ImageCall(*this, result, api);
  }

  bool triggered() const { return !reason_.str().empty(); }
  std::string reason() const { return reason_.str(); }
  void clear() {
    reason_.str("");
    reason_.clear();
  }

  void apply() const;

  bool record(ze_result_t result, const char *api);

private:
  std::stringstream reason_;
};

namespace level_zero_tests {

bool image_support();
bool image_support(ze_device_handle_t device);

void copy_image_from_mem(level_zero_tests::ImagePNG32Bit input,
                         ze_image_handle_t output);
void copy_image_to_mem(ze_image_handle_t input,
                       level_zero_tests::ImagePNG32Bit output);

class zeImageCreateCommon {
public:
  zeImageCreateCommon();
  ~zeImageCreateCommon();
  static const ze_image_desc_t dflt_ze_image_desc;

  static const int8_t dflt_data_pattern = 1;
  level_zero_tests::ImagePNG32Bit dflt_host_image_;
  ze_image_handle_t dflt_device_image_ = nullptr;
  ze_image_handle_t dflt_device_image_2_ = nullptr;

  ImageFuncDispatcher skip;
};

size_t get_format_component_count(ze_image_format_layout_t layout);

void print_image_format_descriptor(const ze_image_format_t descriptor);
void print_image_descriptor(const ze_image_desc_t descriptor);

// write_image_data_pattern() writes the image in the default color order,
// that is defined here:
// a bits  0 ...  7
// b bits  8 ... 15
// g bits 16 ... 23
// r bits 24 ... 31
void write_image_data_pattern(level_zero_tests::ImagePNG32Bit &image,
                              int8_t dp);
// The following uses arbitrary color order as defined in image_format:
void write_image_data_pattern(level_zero_tests::ImagePNG32Bit &image, int8_t dp,
                              const ze_image_format_t &image_format);

// Returns number of errors found, assumes default color order:
int compare_data_pattern(const level_zero_tests::ImagePNG32Bit &imagepng1,
                         const level_zero_tests::ImagePNG32Bit &imagepng2,
                         uint32_t origin1X, uint32_t origin1Y, uint32_t width1,
                         uint32_t height1, uint32_t origin2X, uint32_t origin2Y,
                         uint32_t width2, uint32_t height2);
// Returns number of errors found, color order for both images are
// defined in the image_format parameters:
int compare_data_pattern(const level_zero_tests::ImagePNG32Bit &imagepng1,
                         const ze_image_format_t &image1_format,
                         const level_zero_tests::ImagePNG32Bit &imagepng2,
                         const ze_image_format_t &image2_format,
                         uint32_t origin1X, uint32_t origin1Y, uint32_t width1,
                         uint32_t height1, uint32_t origin2X, uint32_t origin2Y,
                         uint32_t width2, uint32_t height2);

// The following function compares all of the pixels in image
// against the pixels that are expected in the foreground and background of the
// image. The foreground pixels are chosen when the pixels reside in the
// specified region, else the background pixels are chosen. Returns number of
// errors found:
int compare_data_pattern(
    const level_zero_tests::ImagePNG32Bit &image,
    const ze_image_region_t *region,
    const level_zero_tests::ImagePNG32Bit &expected_fg, // expected foreground
    const level_zero_tests::ImagePNG32Bit &expected_bg  // expected background
);

} // namespace level_zero_tests

#endif
