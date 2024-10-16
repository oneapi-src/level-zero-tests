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

void validate_vf_capabilities(zes_vf_exp_capabilities_t &vf_capabilities,
                              uint32_t &vf_count) {
  EXPECT_EQ(vf_capabilities.stype, ZES_STRUCTURE_TYPE_VF_EXP_CAPABILITIES);
  EXPECT_EQ(vf_capabilities.pNext, nullptr);
  EXPECT_GE(vf_capabilities.address.domain, 0);
  EXPECT_LE(vf_capabilities.address.domain, MAX_DOMAINs);
  EXPECT_GE(vf_capabilities.address.bus, 0);
  EXPECT_LE(vf_capabilities.address.bus, MAX_BUSES_PER_DOMAIN);
  EXPECT_GE(vf_capabilities.address.device, 0);
  EXPECT_LE(vf_capabilities.address.device, MAX_DEVICES_PER_BUS);
  EXPECT_GE(vf_capabilities.address.function, 0);
  EXPECT_LE(vf_capabilities.address.function, MAX_FUNCTIONS_PER_DEVICE);
  EXPECT_GT(vf_capabilities.vfDeviceMemSize, 0);
  EXPECT_GT(vf_capabilities.vfID, 0);
  EXPECT_LE(vf_capabilities.vfID, vf_count);
}

void compare_vf_capabilities(zes_vf_exp_capabilities_t &vf_capability_initial,
                             zes_vf_exp_capabilities_t &vf_capability_later) {
  EXPECT_EQ(vf_capability_initial.stype, vf_capability_later.stype);
  EXPECT_EQ(vf_capability_initial.pNext, vf_capability_later.pNext);
  EXPECT_EQ(vf_capability_initial.address.domain,
            vf_capability_later.address.domain);
  EXPECT_EQ(vf_capability_initial.address.bus, vf_capability_later.address.bus);
  EXPECT_EQ(vf_capability_initial.address.device,
            vf_capability_later.address.device);
  EXPECT_EQ(vf_capability_initial.address.function,
            vf_capability_later.address.function);
  EXPECT_EQ(vf_capability_initial.vfDeviceMemSize,
            vf_capability_later.vfDeviceMemSize);
  EXPECT_EQ(vf_capability_initial.vfID, vf_capability_later.vfID);
}

TEST_F(
    VF_MANAGEMENT_TEST,
    GivenValidDeviceWhenCallingEnumEnabledVFExpThenValidVFHandlesAreReturned) {
  for (auto device : devices) {
    uint32_t count = lzt::get_vf_handles_count(device);
    if (count > 0) {
      LOG_INFO << "VF is enabled on this device!!";
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
      LOG_INFO << "VF is enabled on this device!!";
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
      LOG_INFO << "VF is enabled on this device!!";
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
      LOG_INFO << "VF is enabled on this device!!";
      uint32_t test_count = count - 1;
      auto vf_handles = lzt::get_vf_handles(device, test_count);
      EXPECT_EQ(count - 1, test_count);

      for (const auto &vf_handle : vf_handles) {
        EXPECT_NE(vf_handle, nullptr);
      }
    } else if (count == 1) {
      LOG_INFO << "Insufficient number of VF handles to validate this test";
    } else {
      LOG_INFO << "No VF handles found for this device!!";
    }
  }
}

TEST_F(
    VF_MANAGEMENT_TEST,
    GivenValidDeviceWhenRetrievingVfCapabilitiesThenValidCapabilitiesAreReturned) {
  for (auto device : devices) {
    uint32_t count = lzt::get_vf_handles_count(device);
    if (count > 0) {
      LOG_INFO << "VF is enabled on this device!!";
      auto vf_handles = lzt::get_vf_handles(device, count);

      for (const auto &vf_handle : vf_handles) {
        auto vf_capabilities = lzt::get_vf_capabilities(vf_handle);
        validate_vf_capabilities(vf_capabilities, count);
      }
    } else {
      LOG_INFO << "No VF handles found for this device!!";
    }
  }
}

TEST_F(
    VF_MANAGEMENT_TEST,
    GivenValidDeviceWhenRetrievingVfCapabilitiesTwiceThenSimilarCapabilitiesAreReturned) {
  for (auto device : devices) {
    uint32_t count = lzt::get_vf_handles_count(device);
    if (count > 0) {
      LOG_INFO << "VF is enabled on this device!!";
      auto vf_handles = lzt::get_vf_handles(device, count);

      for (const auto &vf_handle : vf_handles) {
        auto vf_capabilities_initial = lzt::get_vf_capabilities(vf_handle);
        auto vf_capabilities_later = lzt::get_vf_capabilities(vf_handle);
        compare_vf_capabilities(vf_capabilities_initial, vf_capabilities_later);
      }
    } else {
      LOG_INFO << "No VF handles found for this device!!";
    }
  }
}

TEST_F(VF_MANAGEMENT_TEST,
       GivenValidDeviceWhenRetrievingVFCapabilitiesThenExpectVfIdIsUnique) {
  for (auto device : devices) {
    uint32_t count = lzt::get_vf_handles_count(device);
    if (count > 1) {
      std::vector<uint32_t> vf_ids{};
      LOG_INFO << "VF is enabled on this device!!";
      auto vf_handles = lzt::get_vf_handles(device, count);

      for (const auto &vf_handle : vf_handles) {
        auto vf_capabilities = lzt::get_vf_capabilities(vf_handle);
        vf_ids.push_back(vf_capabilities.vfID);
      }

      std::set<uint32_t> unique_vf_ids(vf_ids.begin(), vf_ids.end());
      EXPECT_EQ(vf_ids.size(), unique_vf_ids.size());
    } else if (count == 1) {
      LOG_INFO << "Insufficient number of VF handles to validate this test";
    } else {
      LOG_INFO << "No VF handles found for this device!!";
    }
  }
}

} // namespace