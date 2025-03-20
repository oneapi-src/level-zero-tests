/*
 *
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "helpers_test_image.hpp"

namespace lzt = level_zero_tests;

std::string shortened_string(ze_image_type_t type) {
  switch (type) {
  case ZE_IMAGE_TYPE_1D:
    return "img_1d";
  case ZE_IMAGE_TYPE_1DARRAY:
    return "img_1d_arr";
  case ZE_IMAGE_TYPE_2D:
    return "img_2d";
  case ZE_IMAGE_TYPE_2DARRAY:
    return "img_2d_arr";
  case ZE_IMAGE_TYPE_3D:
    return "img_3d";
  case ZE_IMAGE_TYPE_BUFFER:
    return "img_buffer";
  default:
    return "Unknown ze_image_type_t value: " +
           std::to_string(static_cast<int>(type));
  }
}

Dims get_sample_image_dims(ze_image_type_t image_type) {
  switch (image_type) {
  case ZE_IMAGE_TYPE_1DARRAY:
    return {256, 2, 1};
  case ZE_IMAGE_TYPE_2D:
    return {32, 16, 1};
  case ZE_IMAGE_TYPE_2DARRAY:
    return {16, 16, 2};
  case ZE_IMAGE_TYPE_3D:
    return {8, 8, 8};
  default:
    return {512, 1, 1};
  }
}

std::vector<ze_image_type_t>
get_supported_image_types(ze_device_handle_t device, bool exclude_arrays,
                          bool exclude_buffer) {
  std::vector<ze_image_type_t> supported_types{};

  auto properties = lzt::get_image_properties(device);

  if (properties.maxImageDims1D > 0) {
    supported_types.emplace_back(ZE_IMAGE_TYPE_1D);
  } else {
    LOG_INFO << lzt::to_string(ZE_IMAGE_TYPE_1D) << " unsupported";
  }
  if (properties.maxImageDims2D > 0) {
    supported_types.emplace_back(ZE_IMAGE_TYPE_2D);
  } else {
    LOG_INFO << lzt::to_string(ZE_IMAGE_TYPE_2D) << " unsupported";
  }
  if (properties.maxImageDims3D > 0) {
    supported_types.emplace_back(ZE_IMAGE_TYPE_3D);
  } else {
    LOG_INFO << lzt::to_string(ZE_IMAGE_TYPE_3D) << " unsupported";
  }

  if (properties.maxImageArraySlices > 0) {
    if (!exclude_arrays) {
      supported_types.insert(supported_types.end(),
                             {ZE_IMAGE_TYPE_1DARRAY, ZE_IMAGE_TYPE_2DARRAY});
    }
  } else {
    LOG_INFO << lzt::to_string(ZE_IMAGE_TYPE_1DARRAY) << ", "
             << lzt::to_string(ZE_IMAGE_TYPE_2DARRAY) << " unsupported";
  }

  if (properties.maxImageBufferSize > 0) {
    if (!exclude_buffer) {
      supported_types.emplace_back(ZE_IMAGE_TYPE_BUFFER);
    }
  } else {
    LOG_INFO << lzt::to_string(ZE_IMAGE_TYPE_BUFFER) << " unsupported";
  }
  return supported_types;
}
