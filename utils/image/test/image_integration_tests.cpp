/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "image/image.hpp"
#include "gtest/gtest.h"

TEST(ImageIntegrationTests, ReadsPNGFile) {
  level_zero_tests::ImagePNG32Bit image("rgb_brg_3x2.png");
  const std::vector<uint32_t> pixels = {
      0xFF0000FF, //
      0x00FF00FF, //
      0x0000FFFF, //
      0x0000FFFF, //
      0xFF0000FF, //
      0x00FF00FF  //
  };
  EXPECT_EQ(image.get_pixels(), pixels);
}

TEST(ImageIntegrationTests, WritesPNGFile) {
  const std::vector<uint32_t> pixels = {
      0xFF0000FF, //
      0x00FF00FF, //
      0x0000FFFF, //
      0x0000FFFF, //
      0xFF0000FF, //
      0x00FF00FF  //
  };
  level_zero_tests::ImagePNG32Bit image(3, 2);
  image.write("output.png", pixels.data());

  level_zero_tests::ImagePNG32Bit output("output.png");
  EXPECT_EQ(output.get_pixels(), pixels);
  auto result = std::remove("output.png");
  if (result != 0) {
    std::cerr << "Error deleting file output.png" << std::endl;
  }

  EXPECT_EQ(image.get_pixels(), pixels);
}

TEST(ImageIntegrationTests, ReadsGrayscaleBMPFile) {
  level_zero_tests::ImageBMP8Bit image("kwkw_wwkk_4x2_mono.bmp");
  const std::vector<uint8_t> pixels = {
      0x00, //
      0xFF, //
      0x00, //
      0xFF, //
      0xFF, //
      0xFF, //
      0x00, //
      0x00  //
  };
  EXPECT_EQ(image.get_pixels(), pixels);
}

TEST(ImageIntegrationTests, WritesGrayscaleBMPFile) {
  const std::vector<uint8_t> pixels = {
      0x00, //
      0xFF, //
      0x00, //
      0xFF, //
      0xFF, //
      0xFF, //
      0x00, //
      0x00  //
  };
  level_zero_tests::ImageBMP8Bit image(4, 2);
  image.write("output.bmp", pixels.data());

  level_zero_tests::ImageBMP8Bit output("output.bmp");
  EXPECT_EQ(output.get_pixels(), pixels);
  auto result = std::remove("output.bmp");
  if (result != 0) {
    std::cerr << "Error deleting file output.bmp" << std::endl;
  }

  EXPECT_EQ(image.get_pixels(), pixels);
}

TEST(ImageIntegrationTests, ReadsColorBMPFile) {
  level_zero_tests::ImageBMP32Bit image("rgb_brg_3x2_argb.bmp");
  const std::vector<uint32_t> pixels = {
      0xFFFF0000, //
      0xFF00FF00, //
      0xFF0000FF, //
      0xFF0000FF, //
      0xFFFF0000, //
      0xFF00FF00  //
  };
  EXPECT_EQ(image.get_pixels(), pixels);
}

TEST(ImageIntegrationTests, WritesColorBMPFile) {
  const std::vector<uint32_t> pixels = {
      0xFFFF0000, //
      0xFF00FF00, //
      0xFF0000FF, //
      0xFF0000FF, //
      0xFFFF0000, //
      0xFF00FF00  //
  };
  level_zero_tests::ImageBMP32Bit image(3, 2);
  image.write("output.bmp", pixels.data());

  level_zero_tests::ImageBMP32Bit output("output.bmp");
  EXPECT_EQ(output.get_pixels(), pixels);
  auto result = std::remove("output.bmp");
  if (result != 0) {
    std::cerr << "Error deleting file output.bmp" << std::endl;
  }

  EXPECT_EQ(image.get_pixels(), pixels);
}
