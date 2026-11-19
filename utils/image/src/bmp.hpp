/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include <cstdint>
#include <stddef.h>

namespace level_zero_tests {

class BmpUtils {
public:
  static bool load_bmp_image(uint8_t *&data, uint32_t &width, uint32_t &height,
                             uint32_t &pitch, uint16_t &bits_per_pixel,
                             const char *file_name);
  static bool load_bmp_image_8u(uint8_t *&data, uint32_t &width,
                                uint32_t &height, const char *file_name);
};
} // namespace level_zero_tests
