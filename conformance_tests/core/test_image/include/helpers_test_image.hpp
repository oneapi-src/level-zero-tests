/*
 *
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef CONFORMANCE_TESTS_CORE_TEST_IMAGE_INCLUDE_HELPERS_TEST_IMAGE_HPP
#define CONFORMANCE_TESTS_CORE_TEST_IMAGE_INCLUDE_HELPERS_TEST_IMAGE_HPP

#include <string>
#include <vector>
#include <algorithm>

#include "test_harness/test_harness.hpp"

struct Dims {
  uint64_t width;
  uint32_t height;
  uint32_t depth;
};

std::string shortened_string(ze_image_type_t type);

Dims get_sample_image_dims(ze_image_type_t image_type);

std::vector<ze_image_type_t> get_supported_image_types(
    ze_device_handle_t device,
    bool exclude_arrays = false,
    bool exclude_buffer = false);

#endif