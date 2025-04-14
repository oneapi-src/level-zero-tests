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
#include <level_zero/zet_api.h>

namespace {

// Temporarily disabling this test until stype and pNext validation in
// validation layer can be fixed. TEST(MetricNegativeTests,
//      GivenInvalidMetricParametersThenGetParameterValidationErrorReturns) {

//   auto driver = lzt::get_default_driver();
//   auto device = lzt::get_default_device(driver);

//   zet_metric_group_handle_t test_handle =
//       reinterpret_cast<zet_metric_group_handle_t>(0xDEADBEEF);
//   zet_metric_streamer_handle_t streamer;
//   zet_metric_streamer_desc_t streamer_desc0 = {(zet_structure_type_t)0xaaaa,
//                                                nullptr, 1000, 40000};
//   zet_metric_streamer_desc_t streamer_desc1 = {
//       ZET_STRUCTURE_TYPE_METRIC_STREAMER_DESC, nullptr, 1000, 40000};

//   ASSERT_EQ(ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
//             zetMetricStreamerOpen(nullptr, device, test_handle,
//             &streamer_desc1,
//                                   nullptr, &streamer));
//   ASSERT_EQ(ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
//             zetMetricStreamerOpen(lzt::get_default_context(), nullptr,
//                                   test_handle, &streamer_desc1, nullptr,
//                                   &streamer));
//   ASSERT_EQ(ZE_RESULT_ERROR_INVALID_NULL_HANDLE,
//             zetMetricStreamerOpen(lzt::get_default_context(), device,
//             nullptr,
//                                   &streamer_desc1, nullptr, &streamer));
//   ASSERT_EQ(ZE_RESULT_ERROR_INVALID_NULL_POINTER,
//             zetMetricStreamerOpen(lzt::get_default_context(), device,
//                                   test_handle, nullptr, nullptr, &streamer));
//   ASSERT_EQ(ZE_RESULT_ERROR_INVALID_NULL_POINTER,
//             zetMetricStreamerOpen(lzt::get_default_context(), device,
//                                   test_handle, &streamer_desc1, nullptr,
//                                   nullptr));
//   ASSERT_EQ(ZE_RESULT_ERROR_INVALID_ARGUMENT,
//             zetMetricStreamerOpen(lzt::get_default_context(), device,
//                                   test_handle, &streamer_desc0, nullptr,
//                                   &streamer));

//   streamer_desc1.pNext = &streamer_desc0;
//   ASSERT_EQ(ZE_RESULT_ERROR_INVALID_ARGUMENT,
//             zetMetricStreamerOpen(lzt::get_default_context(), device,
//                                   test_handle, &streamer_desc1, nullptr,
//                                   &streamer));
// }

} // namespace
