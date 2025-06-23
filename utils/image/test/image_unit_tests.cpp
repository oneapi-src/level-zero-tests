/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "image/image.hpp"
#include "gtest/gtest.h"
#include "utils/utils.hpp"

LZT_TEST(ImagePNG32Bit, GetPixel) {
  const std::vector<uint32_t> pixels = {
      0xFF0000FF, //
      0x00FF00FF, //
      0x0000FFFF, //
      0x0000FFFF, //
      0xFF0000FF, //
      0x00FF00FF  //
  };
  const level_zero_tests::ImagePNG32Bit image(3, 2, pixels);
  EXPECT_EQ(image.get_pixel(0, 0), pixels[0]);
  EXPECT_EQ(image.get_pixel(1, 0), pixels[1]);
  EXPECT_EQ(image.get_pixel(2, 0), pixels[2]);
  EXPECT_EQ(image.get_pixel(0, 1), pixels[3]);
  EXPECT_EQ(image.get_pixel(1, 1), pixels[4]);
  EXPECT_EQ(image.get_pixel(2, 1), pixels[5]);
}

LZT_TEST(ImagePNG32Bit, SetPixel) {
  const std::vector<uint32_t> pixels = {
      0xFF0000FF, //
      0x00FF00FF, //
      0x0000FFFF, //
      0x0000FFFF, //
      0xFF0000FF, //
      0x00FF00FF  //
  };
  level_zero_tests::ImagePNG32Bit image(3, 2);
  image.set_pixel(0, 0, pixels[0]);
  image.set_pixel(1, 0, pixels[1]);
  image.set_pixel(2, 0, pixels[2]);
  image.set_pixel(0, 1, pixels[3]);
  image.set_pixel(1, 1, pixels[4]);
  image.set_pixel(2, 1, pixels[5]);
  EXPECT_EQ(pixels, image.get_pixels());
}

LZT_TEST(ImageBMP32Bit, GetPixel) {
  const std::vector<uint32_t> pixels = {
      0xFF0000FF, //
      0x00FF00FF, //
      0x0000FFFF, //
      0x0000FFFF, //
      0xFF0000FF, //
      0x00FF00FF  //
  };
  const level_zero_tests::ImageBMP32Bit image(3, 2, pixels);
  EXPECT_EQ(image.get_pixel(0, 0), pixels[0]);
  EXPECT_EQ(image.get_pixel(1, 0), pixels[1]);
  EXPECT_EQ(image.get_pixel(2, 0), pixels[2]);
  EXPECT_EQ(image.get_pixel(0, 1), pixels[3]);
  EXPECT_EQ(image.get_pixel(1, 1), pixels[4]);
  EXPECT_EQ(image.get_pixel(2, 1), pixels[5]);
}

LZT_TEST(ImageBMP32Bit, SetPixel) {
  const std::vector<uint32_t> pixels = {
      0xFF0000FF, //
      0x00FF00FF, //
      0x0000FFFF, //
      0x0000FFFF, //
      0xFF0000FF, //
      0x00FF00FF  //
  };
  level_zero_tests::ImageBMP32Bit image(3, 2);
  image.set_pixel(0, 0, pixels[0]);
  image.set_pixel(1, 0, pixels[1]);
  image.set_pixel(2, 0, pixels[2]);
  image.set_pixel(0, 1, pixels[3]);
  image.set_pixel(1, 1, pixels[4]);
  image.set_pixel(2, 1, pixels[5]);
  EXPECT_EQ(pixels, image.get_pixels());
}

LZT_TEST(ImageBMP8Bit, GetPixel) {
  const std::vector<uint8_t> pixels = {
      0x00, //
      0xFF, //
      0x00, //
      0xFF, //
      0xFF, //
      0x00  //
  };
  const level_zero_tests::ImageBMP8Bit image(3, 2, pixels);
  EXPECT_EQ(image.get_pixel(0, 0), pixels[0]);
  EXPECT_EQ(image.get_pixel(1, 0), pixels[1]);
  EXPECT_EQ(image.get_pixel(2, 0), pixels[2]);
  EXPECT_EQ(image.get_pixel(0, 1), pixels[3]);
  EXPECT_EQ(image.get_pixel(1, 1), pixels[4]);
  EXPECT_EQ(image.get_pixel(2, 1), pixels[5]);
}

LZT_TEST(ImageBMP8Bit, SetPixel) {
  const std::vector<uint8_t> pixels = {
      0x00, //
      0xFF, //
      0x00, //
      0xFF, //
      0xFF, //
      0x00  //
  };
  level_zero_tests::ImageBMP8Bit image(3, 2);
  image.set_pixel(0, 0, pixels[0]);
  image.set_pixel(1, 0, pixels[1]);
  image.set_pixel(2, 0, pixels[2]);
  image.set_pixel(0, 1, pixels[3]);
  image.set_pixel(1, 1, pixels[4]);
  image.set_pixel(2, 1, pixels[5]);
  EXPECT_EQ(pixels, image.get_pixels());
}

template <typename T> class SizeInBytes : public testing::Test {};
typedef testing::Types<level_zero_tests::ImagePNG32Bit,
                       level_zero_tests::ImageBMP8Bit,
                       level_zero_tests::ImageBMP32Bit>
    ImageTypes;
TYPED_TEST_CASE(SizeInBytes, ImageTypes);

TYPED_TEST(SizeInBytes, EmptyImage) {
  const TypeParam image(0, 0);
  EXPECT_EQ(image.size_in_bytes(), level_zero_tests::size_in_bytes(image));
}

TYPED_TEST(SizeInBytes, SinglePixel) {
  const TypeParam image(1, 1);
  EXPECT_EQ(image.size_in_bytes(), level_zero_tests::size_in_bytes(image));
}

TYPED_TEST(SizeInBytes, MultiplePixels) {
  const TypeParam image(2, 2);
  EXPECT_EQ(image.size_in_bytes(), level_zero_tests::size_in_bytes(image));
}
