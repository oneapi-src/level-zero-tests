/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "cl_image_copy.h"

#include "command_line/command_line.hpp"

int ClImageCopy::parse_command_line(int argc, char **argv) {

  std::string channel_order = "";
  std::string flags = "";
  std::string image_type = "";
  std::string channel_datatype = "";

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
      .value("num_image_copies", num_image_copy, "set number of image copies ")
      .value("iter", number_iterations, "set number of iterations")
      .value("channel_order", channel_order,
             " image channel_order like "
             "R/A/INTENSITY/LUMINANCE/RG/RA/RGB/RGBA/ARGB/BGRA")
      .value("image_type", image_type,
             "Image  type like BUFFER/1D/2D/3D/1DARRAY/2DARRAY/1DBUFFER")
      .value("channel_datatype", channel_datatype,
             "image channel data type  like "
             "SNORM_INT8/SNORM_INT16/UNORM_INT8/UNORM_INT16/UNORM_SHORT565/"
             "UNORM_SHORT555/UNORM_INT_101010/SIGNED_INT8/SIGNED_INT16/"
             "SIGNED_INT32/UNSIGNED_INT8/UNSIGNED_INT16/UNSIGNED_INT32/"
             "HALF_FLOAT/FLOAT")
      .value("data-validation", data_validation,
             "optional param for validating the copied image is correct or "
             "not")
      .value("json-output-file", JsonFileName,
             "test output format file name to be specified");
  try {
    options.parse(argc, argv);
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    std::cout << options << "\n";
    return 1;
  }

  if (channel_datatype.size() != 0)
    clChannelDataType = to_channel_datatype(channel_datatype);
  if (channel_order.size() != 0)
    clImageChannelOrder = to_channel_order(channel_order);
  if (image_type.size() != 0)
    clImagetype = to_image_type(image_type);

  if (options.given("help")) {
    std::cout << options << "\n";
    return 1;
  }

  return 0;
}
