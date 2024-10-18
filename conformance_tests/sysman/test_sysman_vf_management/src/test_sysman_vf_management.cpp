/*
 *
 * Copyright (C) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "test_harness/test_harness.hpp"

namespace lzt = level_zero_tests;

namespace {

#ifdef USE_ZESINIT
class VfManagementZesTest : public lzt::ZesSysmanCtsClass {};
#define VF_MANAGEMENT_TEST VfManagementZesTest
#else // USE_ZESINIT
class VfManagementTest : public lzt::SysmanCtsClass {};
#define VF_MANAGEMENT_TEST VfManagementTest
#endif // USE_ZESINIT

TEST_F(
    VF_MANAGEMENT_TEST,
    GivenValidDeviceWhenCallingEnumEnabledVFExpThenValidVFHandlesAreReturned) {
  for (auto device : devices) {
    uint32_t count = lzt::get_vf_handles_count(device);
    if (count > 0) {
      auto vf_handles = lzt::get_vf_handles(device, count);

      for (const auto &vf_handle : vf_handles) {
        EXPECT_NE(vf_handle, nullptr);
      }
    } else {
      LOG_INFO << "No VF handles found for this device!";
    }
  }
}

TEST_F(
    VF_MANAGEMENT_TEST,
    GivenValidDeviceWhenRetrievingVFHandlesTwiceThenSimilarHandlesAreReturned) {
  for (auto device : devices) {
    std::vector<zes_vf_handle_t> vf_handles_initial;
    std::vector<zes_vf_handle_t> vf_handles_later;

    uint32_t count_initial = lzt::get_vf_handles_count(device);
    if (count_initial > 0) {
      vf_handles_initial = lzt::get_vf_handles(device, count_initial);
    }

    uint32_t count_later = lzt::get_vf_handles_count(device);
    if (count_later > 0) {
      vf_handles_later = lzt::get_vf_handles(device, count_later);
    }

    if (count_initial > 0 && count_later > 0) {
      EXPECT_EQ(count_initial, count_later);
      EXPECT_EQ(vf_handles_initial, vf_handles_later);
    } else {
      LOG_INFO << "No VF handles found for this device!";
    }
  }
}

TEST_F(
    VF_MANAGEMENT_TEST,
    GivenValidDeviceWhenRetrievingVFHandlesWithCountGreaterThanActualThenExpectActualCountAndHandlesAreReturned) {
  for (auto device : devices) {
    uint32_t count = lzt::get_vf_handles_count(device);
    if (count > 0) {
      uint32_t test_count = count + 1;
      auto vf_handles = lzt::get_vf_handles(device, test_count);
      EXPECT_EQ(count, test_count);

      for (int i = 0; i < count; i++) {
        EXPECT_NE(vf_handles[i], nullptr);
      }
      EXPECT_EQ(vf_handles[count], nullptr);
    } else {
      LOG_INFO << "No VF handles found for this device!";
    }
  }
}

TEST_F(
    VF_MANAGEMENT_TEST,
    GivenValidDeviceWhenRetrievingVFHandlesWithCountLessThanActualThenExpectReducedCountAndHandlesAreReturned) {
  for (auto device : devices) {
    uint32_t count = lzt::get_vf_handles_count(device);
    if (count > 1) {
      uint32_t test_count = count - 1;
      auto vf_handles = lzt::get_vf_handles(device, test_count);
      EXPECT_EQ(count - 1, test_count);

      for (const auto &vf_handle : vf_handles) {
        EXPECT_NE(vf_handle, nullptr);
      }
    } else {
      LOG_INFO << "Not enough VF handles to validate this test";
    }
  }
}

} // namespace