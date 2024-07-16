/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "cl_image_copy.h"

int ClImageCopy::parse_command_line(int argc, char **argv) {

  std::string channel_order = "";
  std::string flags = "";
  std::string image_type = "";
  std::string channel_datatype = "";

  // Declare the supported options.
  po::variables_map vm;
  po::options_description desc("Allowed options");
  try {
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
        "num_image_copies",
        po::value<uint32_t>(&num_image_copy)->default_value(100),
        "set number of image copies ")(
        "iter", po::value<uint32_t>(&number_iterations)->default_value(50),
        "set number of iterations")(
        "channel_order", po::value<string>(&channel_order),
        " image channel_order like "
        "R/A/INTENSITY/LUMINANCE/RG/RA/RGB/RGBA/ARGB/BGRA")(
        "image_type", po::value<string>(&image_type),
        "Image  type like BUFFER/1D/2D/3D/1DARRAY/2DARRAY/1DBUFFER")(
        "channel_datatype", po::value<string>(&channel_datatype),
        "image channel data type  like "
        "SNORM_INT8/SNORM_INT16/UNORM_INT8/UNORM_INT16/UNORM_SHORT565/"
        "UNORM_SHORT555/UNORM_INT_101010/SIGNED_INT8/SIGNED_INT16/SIGNED_INT32/"
        "UNSIGNED_INT8/UNSIGNED_INT16/UNSIGNED_INT32/HALF_FLOAT/FLOAT")(
        "data-validation", po::value<uint32_t>(&data_validation),
        "optional param for validating the copied image is correct or not")(
        "json-output-file", po::value<string>(&JsonFileName),
        "test output format file name to be specified");

    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    std::cout << desc << "\n";
    return 1;
  }

  if (channel_datatype.size() != 0)
    clChannelDataType = to_channel_datatype(channel_datatype);
  if (channel_order.size() != 0)
    clImageChannelOrder = to_channel_order(channel_order);
  if (image_type.size() != 0)
    clImagetype = to_image_type(image_type);

  if (vm.count("help")) {
    std::cout << desc << "\n";
    return 1;
  } else if (clChannelDataType == -1) {
    std::cout << "unknown channel datatype" << endl;
    std::cout << desc << "\n";
    return 1;
  } else if (clImageChannelOrder == -1) {
    std::cout << "unknown  channel order" << endl;
    std::cout << desc << "\n";
    return 1;
  } else if (clImagetype == -1) {
    std::cout << "unknown  Imagetype" << endl;
    std::cout << desc << "\n";
    return 1;
  }

  return 0;
}
