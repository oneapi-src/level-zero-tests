/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"
#include "test_harness/test_harness.hpp"

namespace level_zero_tests {

ze_sampler_handle_t create_sampler(ze_sampler_address_mode_t addrmode,
                                   ze_sampler_filter_mode_t filtmode,
                                   ze_bool_t normalized) {
  ze_sampler_desc_t descriptor = {};
  descriptor.version = ZE_SAMPLER_DESC_VERSION_CURRENT;
  descriptor.addressMode = addrmode;
  descriptor.filterMode = filtmode;
  descriptor.isNormalized = normalized;

  ze_sampler_handle_t sampler = nullptr;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeSamplerCreate(zeDevice::get_instance()->get_device(), &descriptor,
                            &sampler));
  EXPECT_NE(nullptr, sampler);
  return sampler;
}

ze_sampler_handle_t create_sampler() {
  return create_sampler(ZE_SAMPLER_ADDRESS_MODE_NONE,
                        ZE_SAMPLER_FILTER_MODE_NEAREST, false);
}

void destroy_sampler(ze_sampler_handle_t sampler) {

  EXPECT_EQ(ZE_RESULT_SUCCESS, zeSamplerDestroy(sampler));
}

}; // namespace level_zero_tests
