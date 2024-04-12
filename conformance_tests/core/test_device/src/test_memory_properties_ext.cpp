/*
 *
 * Copyright (C) 2022 Intel Corporation
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

#ifdef ZE_DEVICE_MEMORY_PROPERTIES_EXT_NAME

namespace {

bool check_ext_version() {
  auto ext_props = lzt::get_extension_properties(lzt::get_default_driver());
  uint32_t ext_version = 0;
  for (auto prop : ext_props) {
    if (strncmp(prop.name, ZE_DEVICE_MEMORY_PROPERTIES_EXT_NAME,
                ZE_MAX_EXTENSION_NAME) == 0) {
      ext_version = prop.version;
      break;
    }
  }
  if (ext_version == 0) {
    printf("ZE_DEVICE_MEMORY_PROPERTIES_EXT not found, not running test\n");
    return false;
  } else {
    printf("Extension version %d found\n", ext_version);
    return true;
  }
}

TEST(
    zeDeviceGetMemoryPropertiesTests,
    GivenValidDeviceWhenRetrievingMemoryPropertiesThenValidExtPropertiesAreReturned) {
  if (!check_ext_version())
    GTEST_SKIP();

  auto devices = lzt::get_ze_devices();
  ASSERT_GT(devices.size(), 0);

  for (auto device : devices) {
    auto count = lzt::get_memory_properties_count(device);
    std::vector<ze_device_memory_ext_properties_t> extProperties(count);
    memset(extProperties.data(), 0,
           sizeof(ze_device_memory_ext_properties_t) * count);

    for (auto &prop : extProperties) {
      prop.stype = ZE_STRUCTURE_TYPE_DEVICE_MEMORY_EXT_PROPERTIES;

      // set values to max to ensure they are set to some valid value by
      // get_memory_prop_ext
      prop.type =
          ze_device_memory_ext_type_t::ZE_DEVICE_MEMORY_EXT_TYPE_FORCE_UINT32;
      prop.physicalSize = std::numeric_limits<uint64_t>::max();
      prop.readBandwidth = std::numeric_limits<uint32_t>::max();
      prop.writeBandwidth = std::numeric_limits<uint32_t>::max();
      prop.bandwidthUnit = ze_bandwidth_unit_t::ZE_BANDWIDTH_UNIT_FORCE_UINT32;
    }
    std::vector<ze_device_memory_properties_t> properties =
        lzt::get_memory_properties_ext(device, count, extProperties);

    ASSERT_GT(properties.size(), 0) << "no memory properties found";

    for (uint32_t i = 0; i < properties.size(); ++i) {
      ze_device_memory_ext_properties_t *propExt =
          static_cast<ze_device_memory_ext_properties_t *>(properties[i].pNext);
      ASSERT_NE(propExt, nullptr);
      ASSERT_EQ(propExt->stype, ZE_STRUCTURE_TYPE_DEVICE_MEMORY_EXT_PROPERTIES);

      EXPECT_NE(
          propExt->type,
          ze_device_memory_ext_type_t::ZE_DEVICE_MEMORY_EXT_TYPE_FORCE_UINT32);
      EXPECT_NE(propExt->physicalSize, std::numeric_limits<uint64_t>::max());
      EXPECT_NE(propExt->readBandwidth, std::numeric_limits<uint32_t>::max());
      EXPECT_NE(propExt->writeBandwidth, std::numeric_limits<uint32_t>::max());
      EXPECT_NE(propExt->bandwidthUnit,
                ze_bandwidth_unit_t::ZE_BANDWIDTH_UNIT_FORCE_UINT32);

      LOG_DEBUG << "Memory Prop Ext Index: " << i;
      LOG_DEBUG << "Memory Prop Ext type: " << propExt->type;
      LOG_DEBUG << "Memory Prop Ext physicalSize: " << propExt->physicalSize;
      LOG_DEBUG << "Memory Prop Ext readBandwidth: " << propExt->readBandwidth;
      LOG_DEBUG << "Memory Prop Ext writeBandwidth: "
                << propExt->writeBandwidth;
      LOG_DEBUG << "Memory Prop Ext bandwidthUnit: " << propExt->bandwidthUnit;
    }
  }
}

TEST(
    zeDeviceP2PBandwidthExpProperties,
    GivenMultipleDevicesWhenRetrievingP2PBandwidthPropertiesThenValidPropertiesAreReturned) {
  auto drivers = lzt::get_all_driver_handles();
  ASSERT_GT(drivers.size(), 0);
  std::vector<ze_device_handle_t> devices;
  for (auto driver : drivers) {
    devices = lzt::get_ze_devices();
    if (devices.size() > 1)
      break;
  }

  if (!check_ext_version())
    GTEST_SKIP();

  if (devices.size() <= 1) {
    LOG_INFO << "WARNING: Exiting as multiple devices do not exist";
    GTEST_SKIP();
  }

  ze_device_p2p_bandwidth_exp_properties_t P2PBandwidthProps = {};
  P2PBandwidthProps.stype = ZE_STRUCTURE_TYPE_DEVICE_P2P_BANDWIDTH_EXP_PROPERTIES;
  ze_device_p2p_properties_t P2PProps = {};
  P2PProps.stype = ZE_STRUCTURE_TYPE_DEVICE_P2P_PROPERTIES;
  P2PProps.pNext = &P2PBandwidthProps;

  for (uint32_t dev_1 = 0; dev_1 < devices.size(); ++dev_1) {
    for (uint32_t dev_2 = 0; dev_2 < devices.size(); ++dev_2) {
      ASSERT_EQ(
          zeDeviceGetP2PProperties(devices[dev_1], devices[dev_2], &P2PProps),
          ZE_RESULT_SUCCESS);
      ASSERT_GE((((ze_device_p2p_bandwidth_exp_properties_t *)(P2PProps.pNext))
                     ->logicalBandwidth),
                0);
    }
  }
}

} // namespace

#else
#warning                                                                       \
    "ZE_DEVICE_MEMORY_PROPERTIES_EXT support not found, not building tests for it"
#endif //#ifdef ZE_DEVICE_MEMORY_PROPERTIES_EXT_NAME
