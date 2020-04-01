/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "ze_image_copy.h"

int ZeImageCopy::parse_command_line(int argc, char **argv) {

  std::string layout = "";
  std::string flags = "";
  std::string type = "";
  std::string format = "";

  // Declare the supported options.
  po::options_description desc("Allowed options");
  desc.add_options()("help", "produce help message")(
      "width,w", po::value<uint32_t>(&width)->default_value(2048),
      "set image width")("height,h",
                         po::value<uint32_t>(&height)->default_value(2048),
                         "set image height")(
      "depth,d", po::value<uint32_t>(&depth)->default_value(1),
      "set image depth")("offx",
                         po::value<uint32_t>(&xOffset)->default_value(0),
                         "set image xoffset")(
      "offy", po::value<uint32_t>(&yOffset)->default_value(0),
      "set image yoffset")("offz",
                           po::value<uint32_t>(&zOffset)->default_value(0),
                           "set image zoffset")(
      "warmup", po::value<uint32_t>(&warm_up_iterations)->default_value(10),
      "set number of warmup operations")(
      "num-iter", po::value<uint32_t>(&num_iterations)->default_value(50),
      "set number of iterations")(
      "noofimg", po::value<uint32_t>(&num_image_copies)->default_value(100),
      "set number of image copies ")(
      "layout", po::value<std::string>(&layout),
      " image layout like "
      "8/16/32/8_8/8_8_8_8/16_16/16_16_16_16/32_32/32_32_32_32/10_10_10_2/"
      "11_11_10/5_6_5/5_5_5_1/4_4_4_4/Y8/NV12/YUYV/VYUY/YVYU/UYVY/AYUV/YUAV/"
      "P010/Y410/P012/Y16/P016/Y216/P216/P416")(
      "flags", po::value<std::string>(&flags),
      "image program flags like READ/WRITE/CACHED/UNCACHED")(
      "type", po::value<std::string>(&type),
      "Image  type like 1D/2D/3D/1DARRAY/2DARRAY")(
      "format", po::value<std::string>(&format),
      "image format like UINT/SINT/UNORM/SNORM/FLOAT")(
      "data-validation", po::value<uint32_t>(&data_validation),
      "optional param for validating the copied image is correct or not")(
      "json-output-file", po::value<std::string>(&JsonFileName),
      "test output format file name to be specified");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  if (layout.size() != 0)
    Imagelayout = level_zero_tests::to_layout(layout);
  if (flags.size() != 0)
    Imageflags = level_zero_tests::to_flag(flags);
  if (format.size() != 0)
    Imageformat = level_zero_tests::to_format_type(format);
  if (type.size() != 0)
    Imagetype = level_zero_tests::to_image_type(type);

  if (vm.count("help")) {
    std::cout << desc << std::endl;
    return 1;
  } else if (Imagelayout == -1) {
    std::cout << "unknown format layout" << std::endl;
    std::cout << desc << std::endl;
    return 1;
  } else if (Imageflags == -1) {
    std::cout << "unknown image flags" << std::endl;
    std::cout << desc << std::endl;
    return 1;
  } else if (Imageformat == -1) {
    std::cout << "unknown  Imageformat" << std::endl;
    std::cout << desc << std::endl;
    return 1;
  } else if (Imagetype == -1) {
    std::cout << "unknown  Imagetype" << std::endl;
    std::cout << desc << std::endl;
    return 1;
  }

  return 0;
}
