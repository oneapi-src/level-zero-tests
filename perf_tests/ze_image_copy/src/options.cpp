/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "ze_image_copy.h"

#include "command_line/command_line.hpp"

int ZeImageCopy::parse_command_line(int argc, char **argv) {

  std::string layout = "";
  std::string flags = "";
  std::string type = "";
  std::string format = "";

  // Declare the supported options.
  level_zero_tests::command_line_options options;
  options.flag("help", "produce help message")
      .value("width,w", width, "set image width")
      .value("height,h", height, "set image height")
      .value("depth,d", depth, "set image depth")
      .value("offx", xOffset, "set image xoffset")
      .value("offy", yOffset, "set image yoffset")
      .value("offz", zOffset, "set image zoffset")
      .value("warmup", warm_up_iterations, "set number of warmup operations")
      .value("num-iter", num_iterations, "set number of iterations")
      .value("noofimg", num_image_copies, "set number of image copies ")
      .value("layout", layout,
             " image layout like "
             "8/16/32/8_8/8_8_8_8/16_16/16_16_16_16/32_32/32_32_32_32/"
             "10_10_10_2/11_11_10/5_6_5/5_5_5_1/4_4_4_4/Y8/NV12/YUYV/VYUY/"
             "YVYU/UYVY/AYUV/YUAV/P010/Y410/P012/Y16/P016/Y216/P216/P416")
      .value("flags", flags,
             "image program flags like READ/WRITE/CACHED/UNCACHED")
      .value("type", type, "Image  type like 1D/2D/3D/1DARRAY/2DARRAY")
      .value("format", format, "image format like UINT/SINT/UNORM/SNORM/FLOAT")
      .value("data-validation", data_validation,
             "optional param for validating the copied image is correct or "
             "not")
      .value("json-output-file", JsonFileName,
             "test output format file name to be specified");
  try {
    options.parse(argc, argv);
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    std::cerr << options << std::endl;
    return 1;
  }

  if (layout.size() != 0)
    Imagelayout = level_zero_tests::to_layout(layout);
  if (flags.size() != 0)
    Imageflags = level_zero_tests::to_image_flag(flags);
  if (format.size() != 0)
    Imageformat = level_zero_tests::to_format_type(format);
  if (type.size() != 0)
    Imagetype = level_zero_tests::to_image_type(type);

  if (options.given("help")) {
    std::cout << options << std::endl;
    exit(0);
  } else if (Imagelayout == ZE_IMAGE_FORMAT_LAYOUT_FORCE_UINT32) {
    std::cout << "unknown format layout" << std::endl;
    std::cout << options << std::endl;
    return 1;
  } else if (Imageflags == static_cast<uint32_t>(-1)) {
    std::cout << "unknown image flags" << std::endl;
    std::cout << options << std::endl;
    return 1;
  } else if (Imageformat == ZE_IMAGE_FORMAT_TYPE_FORCE_UINT32) {
    std::cout << "unknown image format" << std::endl;
    std::cout << options << std::endl;
    return 1;
  } else if (Imagetype == ZE_IMAGE_TYPE_FORCE_UINT32) {
    std::cout << "unknown image type" << std::endl;
    std::cout << options << std::endl;
    return 1;
  }

  return 0;
}
