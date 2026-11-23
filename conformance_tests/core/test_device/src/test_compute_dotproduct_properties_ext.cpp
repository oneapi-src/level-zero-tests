/*
 *
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <limits>

#include "gtest/gtest.h"

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"

namespace lzt = level_zero_tests;

namespace {

constexpr uint32_t not_written = std::numeric_limits<uint32_t>::max();

constexpr uint32_t dp4a_caps =
    ZE_DEVICE_DP_CAPABILITY_FLAG_DEVICE_NON_SYSTOLIC_DPA4_SIMD_ALL;
constexpr uint32_t dpas_caps =
    ZE_DEVICE_DP_CAPABILITY_FLAG_DEVICE_SYSTOLIC_DPAS_SIMD8 |
    ZE_DEVICE_DP_CAPABILITY_FLAG_DEVICE_SYSTOLIC_DPAS_SIMD16 |
    ZE_DEVICE_DP_CAPABILITY_FLAG_DEVICE_SYSTOLIC_DPAS_DEPTH4 |
    ZE_DEVICE_DP_CAPABILITY_FLAG_DEVICE_SYSTOLIC_DPAS_DEPTH8;
constexpr uint32_t bdpas_caps =
    ZE_DEVICE_DP_CAPABILITY_FLAG_DEVICE_SYSTOLIC_BDPAS_SIMD16 |
    ZE_DEVICE_DP_CAPABILITY_FLAG_DEVICE_SYSTOLIC_BDPAS_DEPTH8;
constexpr uint32_t none_input_type =
    ZE_DEVICE_INPUT_DATA_TYPE_FLAG_DEVICE_INPUT_DATA_TYPE_NONE;

LZT_TEST(
    zeDeviceGetComputePropertiesTests,
    GivenValidDeviceWhenRetrievingComputePropertiesThenValidDotProductExtPropertiesAreReturned) {
  if (!lzt::check_if_extension_supported(
          lzt::get_default_driver(),
          ZE_DEVICE_COMPUTE_DOTPRODUCT_PROPERTIES_EXT_NAME)) {
    GTEST_SKIP() << "Extension "
                 << ZE_DEVICE_COMPUTE_DOTPRODUCT_PROPERTIES_EXT_NAME
                 << " not supported";
  }

  const auto devices = lzt::get_ze_devices();
  ASSERT_GT(devices.size(), 0);

  for (const auto &device : devices) {
    ze_device_compute_dotproduct_ext_properties_t dp_properties = {
        ZE_STRUCTURE_TYPE_DEVICE_COMPUTE_DOTPRODUCT_EXT_PROPERTIES};

    dp_properties.dp_caps = not_written;
    dp_properties.dpv4_input_types = not_written;
    dp_properties.dpv4_output_types = not_written;
    dp_properties.dpas_input_types = not_written;
    dp_properties.dpas_output_types = not_written;
    dp_properties.bdpas_input_types = not_written;
    dp_properties.bdpas_output_types = not_written;

    ze_device_compute_properties_t properties = {
        ZE_STRUCTURE_TYPE_DEVICE_COMPUTE_PROPERTIES, &dp_properties};

    ASSERT_ZE_RESULT_SUCCESS(zeDeviceGetComputeProperties(device, &properties));
    ASSERT_NE(not_written, dp_properties.dp_caps);

    if ((dp_properties.dp_caps & dp4a_caps) != 0) {
      EXPECT_NE(not_written, dp_properties.dpv4_input_types);
      EXPECT_NE(not_written, dp_properties.dpv4_output_types);

      if ((dp_properties.dpv4_input_types & none_input_type) != 0) {
        EXPECT_EQ(none_input_type, dp_properties.dpv4_input_types);
      }
    }
    if ((dp_properties.dp_caps & dpas_caps) != 0) {
      EXPECT_NE(not_written, dp_properties.dpas_input_types);
      EXPECT_NE(not_written, dp_properties.dpas_output_types);

      if ((dp_properties.dpas_input_types & none_input_type) != 0) {
        EXPECT_EQ(none_input_type, dp_properties.dpas_input_types);
      }
    }
    if ((dp_properties.dp_caps & bdpas_caps) != 0) {
      EXPECT_NE(not_written, dp_properties.bdpas_input_types);
      EXPECT_NE(not_written, dp_properties.bdpas_output_types);

      if ((dp_properties.bdpas_input_types & none_input_type) != 0) {
        EXPECT_EQ(none_input_type, dp_properties.bdpas_input_types);
      }
    }
  }
}

} // namespace
