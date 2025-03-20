/*
 *
 * Copyright (C) 2023 Intel Corporation
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
#include <level_zero/zes_api.h>

namespace {

// TEST(
//     MetricNegativeTests,
//     GivenZesDevicePropertiesWithInvalidParametersThenGetParameterValidationErrorReturns)
//     {

//   auto driver = lzt::get_default_driver();
//   auto device = lzt::get_default_device(driver);

//   zes_device_properties_t deviceProperties0 = {
//       ZES_STRUCTURE_TYPE_DEVICE_PROPERTIES, nullptr};
//   zes_device_properties_t deviceProperties1 = {(zes_structure_type_t)0xaaaa,
//                                                nullptr};

//   ASSERT_EQ(ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
//             zesDeviceGetProperties(nullptr, &deviceProperties0));

//   ASSERT_EQ(ZE_RESULT_ERROR_INVALID_NULL_POINTER,
//             zesDeviceGetProperties(device, nullptr));

//   ASSERT_EQ(ZE_RESULT_ERROR_INVALID_ARGUMENT,
//             zesDeviceGetProperties(device, &deviceProperties1));

//   deviceProperties0.pNext = &deviceProperties1;
//   ASSERT_EQ(ZE_RESULT_ERROR_INVALID_ARGUMENT,
//             zesDeviceGetProperties(device, &deviceProperties0));
// }

} // namespace
