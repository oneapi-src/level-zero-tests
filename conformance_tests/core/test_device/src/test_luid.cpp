/*
 *
 * Copyright (C) 2019 Intel Corporation
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

#ifdef ZE_DEVICE_LUID_EXT_NAME

namespace {

bool check_ext_version() {
  auto ext_props = lzt::get_extension_properties(lzt::get_default_driver());
  uint32_t ext_version = 0;
  for (auto prop : ext_props) {
    if (strncmp(prop.name, ZE_DEVICE_LUID_EXT_NAME, ZE_MAX_EXTENSION_NAME) ==
        0) {
      ext_version = prop.version;
      break;
    }
  }
  if (ext_version == 0) {
    printf("ZE_DEVICE_LUID_EXT not found, not running test\n");
    return false;
  } else {
    printf("Extension version %d found\n", ext_version);
    return true;
  }
}

bool luid_equal(
    const std::array<uint8_t, ZE_MAX_DEVICE_LUID_SIZE_EXT> &first,
    const std::array<uint8_t, ZE_MAX_DEVICE_LUID_SIZE_EXT> &second) {
  for (int index = 0; (index < ZE_MAX_DEVICE_LUID_SIZE_EXT); index++) {
    if (first[index] != second[index])
      return false;
  }
  return true;
}

TEST(zeLuidTests, GivenValidDevicesWhenRetrievingLuidThenValidValuesReturned) {
  if (!check_ext_version())
    GTEST_SKIP();

  auto devices = lzt::get_ze_devices();
  ASSERT_GT(devices.size(), 0);

  // keep a list of each luid to confirm they are unique in cases with multiple
  // devices
  std::vector<std::array<uint8_t, ZE_MAX_DEVICE_LUID_SIZE_EXT>> uniqueLuidList;
  for (auto device : devices) {
    ze_device_properties_t properties = {ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES,
                                         nullptr};
    ze_device_luid_ext_properties_t extProperties = {};
    extProperties.stype = ZE_STRUCTURE_TYPE_DEVICE_LUID_EXT_PROPERTIES;

    properties.pNext = &extProperties;
    EXPECT_EQ(ZE_RESULT_SUCCESS, zeDeviceGetProperties(device, &properties));

    ze_device_luid_ext_properties_t *propExt =
        static_cast<ze_device_luid_ext_properties_t *>(properties.pNext);
    ASSERT_NE(propExt, nullptr);
    ASSERT_EQ(propExt->stype, ZE_STRUCTURE_TYPE_DEVICE_LUID_EXT_PROPERTIES);

    // confirm luid is non-zero
    std::array<uint8_t, ZE_MAX_DEVICE_LUID_SIZE_EXT> currentLuid;
    std::copy(std::begin(propExt->luid.id), std::end(propExt->luid.id),
              currentLuid.begin());
    std::array<uint8_t, ZE_MAX_DEVICE_LUID_SIZE_EXT> zeroLuid = {};
    EXPECT_FALSE(luid_equal(currentLuid, zeroLuid));

    // confirm luid is unique in cases with multiple devices
    for (auto compareLuid : uniqueLuidList)
      ASSERT_FALSE(luid_equal(currentLuid, compareLuid));
    uniqueLuidList.push_back(currentLuid);

    // confirm nodeMask is non-zero
    EXPECT_GT(propExt->nodeMask, 0);
  }
}

} // namespace

#else
#warning "ZE_DEVICE_LUID_EXT support not found, not building tests for it"
#endif // #ifdef ZE_DEVICE_LUID_EXT_NAME
