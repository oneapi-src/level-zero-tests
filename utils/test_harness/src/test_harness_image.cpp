/*
 *
 * Copyright (C) 2019-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "test_harness/test_harness.hpp"
#include "gtest/gtest.h"
#include "logging/logging.hpp"
#include "utils/utils.hpp"
#include <level_zero/ze_api.h>

namespace lzt = level_zero_tests;

namespace level_zero_tests {

ze_result_t create_ze_image(ze_context_handle_t context, ze_device_handle_t dev,
                            ze_image_desc_t image_descriptor,
                            ze_image_handle_t &image) {
  image = nullptr;
  ze_result_t result = zeImageCreate(context, dev, &image_descriptor, &image);
  if (result == ZE_RESULT_SUCCESS) {
    EXPECT_NE(nullptr, image);
  }
  return result;
}

ze_result_t create_ze_image(ze_device_handle_t dev,
                            ze_image_desc_t image_descriptor,
                            ze_image_handle_t &image) {
  return create_ze_image(lzt::get_default_context(), dev, image_descriptor,
                         image);
}

ze_result_t create_ze_image(ze_image_desc_t image_descriptor,
                            ze_image_handle_t &image) {
  return create_ze_image(lzt::get_default_context(),
                         lzt::zeDevice::get_instance()->get_device(),
                         image_descriptor, image);
}

ze_image_handle_t create_ze_image(ze_context_handle_t context,
                                  ze_device_handle_t dev,
                                  ze_image_desc_t image_descriptor) {
  ze_image_handle_t image = nullptr;
  ze_result_t result = create_ze_image(context, dev, image_descriptor, image);
  EXPECT_TRUE(result == ZE_RESULT_SUCCESS ||
              result == ZE_RESULT_ERROR_UNSUPPORTED_IMAGE_FORMAT);
  return image;
}

ze_image_handle_t create_ze_image(ze_device_handle_t dev,
                                  ze_image_desc_t image_descriptor) {
  return create_ze_image(lzt::get_default_context(), dev, image_descriptor);
}

ze_image_handle_t create_ze_image(ze_image_desc_t image_descriptor) {
  return create_ze_image(lzt::get_default_context(),
                         lzt::zeDevice::get_instance()->get_device(),
                         image_descriptor);
}

void destroy_ze_image(ze_image_handle_t image) {
  EXPECT_ZE_RESULT_SUCCESS(zeImageDestroy(image));
}

ze_result_t create_ze_image_view_ext(ze_device_handle_t device,
                                     const ze_image_desc_t *desc,
                                     ze_image_handle_t image,
                                     ze_image_handle_t &image_view) {
  auto context = lzt::get_default_context();

  image_view = nullptr;
  ze_result_t result =
      zeImageViewCreateExt(context, device, desc, image, &image_view);
  if (result == ZE_RESULT_SUCCESS) {
    EXPECT_NE(nullptr, image_view);
  }
  return result;
}

ze_image_handle_t create_ze_image_view_ext(ze_device_handle_t device,
                                           const ze_image_desc_t *desc,
                                           ze_image_handle_t image) {
  ze_image_handle_t image_view = nullptr;
  EXPECT_ZE_RESULT_SUCCESS(
      create_ze_image_view_ext(device, desc, image, image_view));
  return image_view;
}

ze_image_properties_t get_ze_image_properties(ze_image_desc_t image_descriptor,
                                              ze_result_t *result) {

  ze_image_properties_t image_properties = {ZE_STRUCTURE_TYPE_IMAGE_PROPERTIES};
  ze_result_t res =
      zeImageGetProperties(lzt::zeDevice::get_instance()->get_device(),
                           &image_descriptor, &image_properties);
  if (result != nullptr) {
    *result = res;
  } else {
    EXPECT_ZE_RESULT_SUCCESS(res);
  }

  return image_properties;
}

ze_image_allocation_ext_properties_t
get_ze_image_alloc_properties_ext(ze_image_handle_t image) {
  ze_image_allocation_ext_properties_t image_alloc_properties = {};
  image_alloc_properties.stype =
      ZE_STRUCTURE_TYPE_IMAGE_ALLOCATION_EXT_PROPERTIES;
  EXPECT_ZE_RESULT_SUCCESS(zeImageGetAllocPropertiesExt(
      lzt::get_default_context(), image, &image_alloc_properties));
  return image_alloc_properties;
}

ze_image_memory_properties_exp_t
get_ze_image_mem_properties_exp(ze_image_handle_t image) {
  ze_image_memory_properties_exp_t image_mem_properties = {};
  image_mem_properties.stype = ZE_STRUCTURE_TYPE_IMAGE_MEMORY_EXP_PROPERTIES;
  image_mem_properties.pNext = nullptr;
  EXPECT_ZE_RESULT_SUCCESS(
      zeImageGetMemoryPropertiesExp(image, &image_mem_properties));
  return image_mem_properties;
}

uint64_t get_image_device_offset_exp(ze_image_handle_t image,
                                     uint64_t *device_offset) {
  ze_image_handle_t initial_image = image;
  EXPECT_NE(device_offset, nullptr);
  EXPECT_ZE_RESULT_SUCCESS(zeImageGetDeviceOffsetExp(image, device_offset));
  EXPECT_EQ(image, initial_image);
  return device_offset == nullptr ? 0 : *device_offset;
}

}; // namespace level_zero_tests
