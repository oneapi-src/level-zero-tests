/*
 *
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"

namespace lzt = level_zero_tests;

#ifdef USE_ZESINIT
#define DEVICE_HANDLE_TYPE zes_device_handle_t
#define GET_DEVICE_FN lzt::get_zes_devices
#else // USE_ZESINIT
#define DEVICE_HANDLE_TYPE ze_device_handle_t
#define GET_DEVICE_FN lzt::get_ze_devices
#endif // USE_ZESINIT

void get_sysman_devices(const std::string set_device_hierarchy,
                        std::vector<DEVICE_HANDLE_TYPE> &devices) {
  std::string env_str = "ZE_FLAT_DEVICE_HIERARCHY=" + set_device_hierarchy;
  char *sys_env = &env_str[0];
  putenv(sys_env);
  char *get_device_hierarchy = getenv("ZE_FLAT_DEVICE_HIERARCHY");
  EXPECT_NE(get_device_hierarchy, nullptr);
  LOG_INFO << "Child Process : Device Hierarchy = " << get_device_hierarchy
      ? get_device_hierarchy
      : "NULL";
  devices = GET_DEVICE_FN();
}

void compare_devices_in_flat_and_composite(
    std::vector<DEVICE_HANDLE_TYPE> flat_mode_sysman_devices,
    std::vector<DEVICE_HANDLE_TYPE> composite_mode_sysman_devices) {
#ifdef USE_ZESINIT
  ASSERT_EQ(flat_mode_sysman_devices.size(),
            composite_mode_sysman_devices.size());
  for (uint32_t i = 0; i < flat_mode_sysman_devices.size(); i++) {
    EXPECT_EQ(flat_mode_sysman_devices[i], composite_mode_sysman_devices[i]);
  }
#else  // USE_ZESINIT
  if (flat_mode_sysman_devices.size() == composite_mode_sysman_devices.size()) {
    ASSERT_EQ(flat_mode_sysman_devices.size(),
              composite_mode_sysman_devices.size());
    for (uint32_t i = 0; i < flat_mode_sysman_devices.size(); i++) {
      EXPECT_EQ(flat_mode_sysman_devices[i], composite_mode_sysman_devices[i]);
    }
  } else {
    EXPECT_NE(flat_mode_sysman_devices.size(),
              composite_mode_sysman_devices.size());
  }
#endif // USE_ZESINIT
}

int main() {

#ifdef USE_ZESINIT
  ze_result_t result = zesInit(0);
  if (result) {
    throw std::runtime_error("Child Process : zesInit failed: " +
                             level_zero_tests::to_string(result));
    exit(1);
  }
  LOG_INFO << "Child Process : Sysman initialized";
#else  // USE_ZESINIT
  static char sys_env[] = "ZES_ENABLE_SYSMAN=1";
  putenv(sys_env);
  auto is_sysman_enabled = getenv("ZES_ENABLE_SYSMAN");
  if (is_sysman_enabled == nullptr) {
    LOG_INFO << "Child Process : Sysman is not Enabled";
    exit(1);
  } else {
    auto is_sysman_enabled_int = atoi(is_sysman_enabled);
    if (is_sysman_enabled_int == 1) {
      ze_result_t result = zeInit(2);
      if (result) {
        throw std::runtime_error("zeInit failed: " +
                                 level_zero_tests::to_string(result));
        exit(1);
      }
      LOG_INFO << "Child Process : Driver initialized";
    } else {
      LOG_INFO << "Child Process : Sysman is not Enabled";
      exit(1);
    }
  }
#endif // USE_ZESINIT

  std::vector<DEVICE_HANDLE_TYPE> flat_mode_sysman_devices;
  get_sysman_devices("FLAT", flat_mode_sysman_devices);

  std::vector<DEVICE_HANDLE_TYPE> composite_mode_sysman_devices;
  get_sysman_devices("COMPOSITE", composite_mode_sysman_devices);

  compare_devices_in_flat_and_composite(flat_mode_sysman_devices,
                                        composite_mode_sysman_devices);

  exit(0);
}