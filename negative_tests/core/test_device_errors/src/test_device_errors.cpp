/*
 *
 * Copyright (C) 2020-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"

namespace lzt = level_zero_tests;

#include <level_zero/ze_api.h>

namespace {

LZT_TEST(
    DeviceGetNegativeTests,
    GivenInvalidDriverHandleWhileCallingzeDeviceGetThenInvalidHandleIsReturned) {
  uint32_t pCount = 0;
  EXPECT_EQ(ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
            zeDeviceGet(nullptr, &pCount, nullptr));
}

LZT_TEST(
    DeviceGetNegativeTests,
    GivenInvalidOutputCountPointerWhileCallingzeDeviceGetThenInvalidNullPointerIsReturned) {
  auto drivers = lzt::get_all_driver_handles();
  for (auto driver : drivers) {
    EXPECT_EQ(ZE_RESULT_ERROR_INVALID_NULL_POINTER,
              zeDeviceGet(driver, nullptr, nullptr));
  }
}

// Temporarily disabling this test until stype and pNext validation in
// validation layer can be fixed. TEST(DeviceNegativeTests,
// GivenInvalidStypeWhen) {
//   auto driver = lzt::get_default_driver();
//   auto device = lzt::get_default_device(driver);

//   ASSERT_EQ(ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
//             zeDeviceGetProperties(nullptr, nullptr));

//   ASSERT_EQ(ZE_RESULT_ERROR_INVALID_NULL_POINTER,
//             zeDeviceGetProperties(device, nullptr));

//   ze_device_properties_t props0 = {ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES,
//                                    nullptr};
//   ASSERT_ZE_RESULT_SUCCESS( zeDeviceGetProperties(device, &props0));

//   ze_device_properties_t props1 = {(ze_structure_type_t)0xaaaa, nullptr};
//   ASSERT_EQ(ZE_RESULT_ERROR_INVALID_ARGUMENT,
//             zeDeviceGetProperties(device, &props1));

//   props0.pNext = &props1;
//   ASSERT_EQ(ZE_RESULT_ERROR_INVALID_ARGUMENT,
//             zeDeviceGetProperties(device, &props0));

//   ze_device_properties_t props2 = {ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES_1_2,
//                                    nullptr};
//   ASSERT_ZE_RESULT_SUCCESS( zeDeviceGetProperties(device, &props2));

//   props2.pNext = &props0;
//   ASSERT_EQ(ZE_RESULT_ERROR_INVALID_ARGUMENT,
//             zeDeviceGetProperties(device, &props2));
// }

// TEST(DeviceNegativeTests,
//      GivenDeviceCacheWithMultipleStypeWithInvalidStypeWhen) {
//   auto driver = lzt::get_default_driver();
//   auto device = lzt::get_default_device(driver);

//   uint32_t count = 0;

//   ASSERT_EQ(ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
//             zeDeviceGetCacheProperties(nullptr, &count, nullptr));

//   ASSERT_EQ(ZE_RESULT_ERROR_INVALID_NULL_POINTER,
//             zeDeviceGetCacheProperties(device, nullptr, nullptr));

//   ASSERT_ZE_RESULT_SUCCESS(
//             zeDeviceGetCacheProperties(device, &count, nullptr));

//   std::vector<ze_device_cache_properties_t> cacheProperties(count);
//   for (auto &properties : cacheProperties) {
//     properties = {ZE_STRUCTURE_TYPE_DEVICE_CACHE_PROPERTIES, nullptr};
//   }

//   ASSERT_ZE_RESULT_SUCCESS(
//             zeDeviceGetCacheProperties(device, &count,
//             cacheProperties.data()));

//   ze_device_cache_properties_t dummyCacheProperties = {
//       (ze_structure_type_t)0xaaaa, nullptr};
//   cacheProperties[0].pNext = &dummyCacheProperties;

//   ASSERT_EQ(ZE_RESULT_ERROR_INVALID_ARGUMENT,
//             zeDeviceGetCacheProperties(device, &count,
//             cacheProperties.data()));
// }

} // namespace
