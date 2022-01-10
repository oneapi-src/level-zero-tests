/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "test_harness/test_harness_driver_info.hpp"
#include <level_zero/ze_api.h>
#include "logging/logging.hpp"
#include "gtest/gtest.h"

namespace level_zero_tests {

driverInfo_t *collect_driver_info(uint32_t &driverInfoCount_) {

  uint32_t driver_count = 0;

  LOG_INFO << "collect driver information";

  ze_driver_handle_t *driver_handles;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeDriverGet(&driver_count, NULL));
  EXPECT_NE(0, driver_count);

  driver_handles = new ze_driver_handle_t[driver_count];
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeDriverGet(&driver_count, driver_handles));

  LOG_INFO << "number of drivers " << driver_count;

  driverInfo_t *driver_info;
  driver_info = new driverInfo_t[driver_count];

  for (uint32_t i = 0; i < driver_count; i++) {

    driver_info[i].driver_handle = driver_handles[i];

    uint32_t device_count = 0;
    EXPECT_EQ(ZE_RESULT_SUCCESS,
              zeDeviceGet(driver_handles[i], &device_count, NULL));
    EXPECT_NE(0, device_count);
    driver_info[i].device_handles = new ze_device_handle_t[device_count];
    EXPECT_EQ(ZE_RESULT_SUCCESS, zeDeviceGet(driver_handles[i], &device_count,
                                             driver_info[i].device_handles));
    driver_info[i].number_device_handles = device_count;

    LOG_INFO << "there are " << device_count << " devices in driver " << i;

    /* one ze_device_properties_t for each device in the driver */
    driver_info[i].device_properties = new ze_device_properties_t[device_count];
    for (uint32_t j = 0; j < device_count; j++) {
      driver_info[i].device_properties[j] = {};
      driver_info[i].device_properties[j].stype =
          ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES;
    }

    for (uint32_t j = 0; j < device_count; j++) {
      EXPECT_EQ(ZE_RESULT_SUCCESS,
                zeDeviceGetProperties(driver_info[i].device_handles[j],
                                      &driver_info[i].device_properties[j]));
      LOG_INFO << "zeDeviceGetProperties device " << j;
      LOG_INFO << " name " << driver_info[i].device_properties[j].name;
    }

    /* one ze_device_memory_properties_t per device in the driver */
    uint32_t device_memory_properties_count = 1;
    driver_info[i].device_memory_properties =
        new ze_device_memory_properties_t[device_memory_properties_count *
                                          device_count];
    for (uint32_t j = 0; j < device_count; j++) {
      driver_info[i].device_memory_properties[j].stype =
          ZE_STRUCTURE_TYPE_DEVICE_MEMORY_PROPERTIES;
      EXPECT_EQ(ZE_RESULT_SUCCESS,
                zeDeviceGetMemoryProperties(
                    driver_info[i].device_handles[j],
                    &device_memory_properties_count,
                    &driver_info[i].device_memory_properties[j]));
      LOG_INFO << "ze_device_memory_properties_t device " << j;
      LOG_INFO << "totalSize "
               << driver_info[i].device_memory_properties[j].totalSize;
    }
  }

  driverInfoCount_ = driver_count;
  delete[] driver_handles;
  return driver_info;
}

void free_driver_info(driverInfo_t *driverInfo_, uint32_t driverInfoCount_) {

  LOG_INFO << "free driver information";

  for (uint32_t i = 0; i < driverInfoCount_; i++) {
    delete[] driverInfo_[i].device_handles;
    delete[] driverInfo_[i].device_properties;
    delete[] driverInfo_[i].device_memory_properties;
  }
  delete[] driverInfo_;
  driverInfo_ = NULL;
}

}; // namespace level_zero_tests
