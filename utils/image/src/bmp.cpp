/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "bmp.hpp"

#include <memory.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "logging/logging.hpp"

namespace level_zero_tests {

#pragma pack(push, 1)

struct BMPFileHeader {
  uint16_t bf_type_; // 'BM' for BitMap
  uint32_t bf_size_; // file size in bytes
  uint16_t bf_reserved1_;
  uint16_t bf_reserved2_;
  uint32_t bf_off_bits_; // offset of bitmap in file
};

struct BMPInfoHeader {
  uint32_t bi_size_;      // length of info header
  uint32_t bi_width_;     // width of bitmap in pixels
  int32_t bi_height_;     // height of bitmap in pixels (negative height means a
                          // top-down bitmap)
  uint16_t bi_planes_;    // number of color planes - must be 1
  uint16_t bi_bit_count_; // bit depth
  uint32_t bi_compression_;
  uint32_t bi_size_image_; // size of picture in bytes
  uint32_t bi_x_pels_per_meter_;
  uint32_t bi_y_pels_per_meter_;
  uint32_t bi_clr_used_;
  uint32_t bi_clr_important_;
};

#pragma pack(pop)

template <typename T> inline constexpr uint8_t to_u8(T val) {
  return static_cast<uint8_t>(val);
}

template <typename T> inline constexpr uint16_t to_u16(T val) {
  return static_cast<uint16_t>(val);
}

template <typename T> inline constexpr uint32_t to_u32(T val) {
  return static_cast<uint32_t>(val);
}

bool BmpUtils::load_bmp_image(uint8_t *&data, uint32_t &width, uint32_t &height,
                              uint32_t &pitch, uint16_t &bits_per_pixel,
                              const char *file_name) {
  FILE *stream = nullptr;

  stream = fopen(file_name, "rb");

  if (nullptr == stream)
    return false;

  BMPFileHeader file_header;
  BMPInfoHeader info_header;
  uint32_t gap = 0;

  constexpr size_t file_hdr_size = sizeof(file_header);
  constexpr size_t info_hdr_size = sizeof(info_header);

  if (file_hdr_size != fread(&file_header, 1, file_hdr_size, stream)) {
    goto error_exit;
  }

  if (info_hdr_size != fread(&info_header, 1, info_hdr_size, stream)) {
    goto error_exit;
  }

  gap = to_u32(file_header.bf_off_bits_ - file_hdr_size - info_hdr_size);
  if (fseek(stream, gap, SEEK_CUR)) {
    LOG_ERROR << "Failed to seek in BMP file";
    goto error_exit;
  }

  width = info_header.bi_width_;
  height = to_u32(info_header.bi_height_ > 0 ? info_header.bi_height_
                                             : -info_header.bi_height_);
  bits_per_pixel = info_header.bi_bit_count_;
  pitch = info_header.bi_width_ * ((info_header.bi_bit_count_ + 7U) / 8U);

  // Pitch is a multiple of four bytes:
  pitch = (pitch + 3U) & ~3U;

  if ((height > (1 << 16)) || (pitch > (1 << 16)))
    goto error_exit;

  data = new uint8_t[height * pitch];
  if (nullptr == data)
    goto error_exit;

  if (info_header.bi_height_ > 0) {
    for (uint32_t h = 0; h < height; h++) {
      uint8_t *dst = data + (height - h - 1) * pitch;
      if (pitch != to_u32(fread(dst, 1, pitch, stream))) {
        goto error_exit;
      }
    }
  } else {
    if (height * pitch != to_u32(fread(data, 1, height * pitch, stream))) {
      goto error_exit;
    }
  }

  fclose(stream);
  return true;

error_exit:
  fclose(stream);
  return false;
}

bool BmpUtils::load_bmp_image_8u(uint8_t *&data, uint32_t &width,
                                 uint32_t &height, const char *file_name) {
  // First, load the BMP image generally:
  uint32_t src_pitch = 0;
  uint16_t src_bits_per_pixel = 0;

  bool success = load_bmp_image(data, width, height, src_pitch,
                                src_bits_per_pixel, file_name);

  if (src_bits_per_pixel != 8 && src_bits_per_pixel != 24 &&
      src_bits_per_pixel != 32) {
    success = false;
  }

  if ((width > (1U << 16)) || (height > (1U << 16))) {
    success = false;
  }

  uint32_t dst_pitch = width;
  uint8_t *data8u = nullptr;

  if (success) {
    data8u = new uint8_t[height * dst_pitch];
    if (nullptr == data8u) {
      success = false;
    }
    if ((width > (1U << 16)) || (height > (1U << 16))) {
      success = false;
    }
  }

  if (success) {
    uint8_t *src_start = data;
    uint8_t *dst_start = data8u;
    uint16_t src_bytes_per_pixel = to_u16((src_bits_per_pixel + 7U) / 8U);
    uint16_t dst_bytes_per_pixel = 1;

    for (uint32_t h = 0U; h < height;
         h++, src_start += src_pitch, dst_start += dst_pitch) {
      uint8_t *src_pixel = src_start;
      uint8_t *dst_pixel = dst_start;

      for (uint32_t w = 0U; w < width; w++, src_pixel += src_bytes_per_pixel,
                    dst_pixel += dst_bytes_per_pixel) {
        switch (src_bits_per_pixel) {
        case 8:
          dst_pixel[0] = src_pixel[0];
          break;
        case 24:
        case 32:
          dst_pixel[0] = to_u8(0.21f * src_pixel[0] + 0.72f * src_pixel[1] +
                               0.07f * src_pixel[2]);
          break;
        default:
          LOG_WARNING << "Unexpected bits per pixel: " << src_bits_per_pixel;
        }
      }
    }
  }

  delete[] data;
  data = data8u;

  return success;
}
} // namespace level_zero_tests
