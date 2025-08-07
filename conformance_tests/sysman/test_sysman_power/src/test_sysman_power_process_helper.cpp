/*
 *
 * Copyright (C) 2020-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"

namespace lzt = level_zero_tests;
namespace bp = boost::process;
namespace bi = boost::interprocess;

struct powerInfo {
  uint32_t interval;
  uint32_t limit;
  bool limitValueLocked;
};

void GetPowerLimitsSetByParentProcess(uint32_t power_handle_size, bool &test_result) {
  //1. read shared object
  bi::shared_memory_object power_limit_shm(bi::open_only, "MultiProcPowerLimitSharedMemory", bi::read_only);
  bi::mapped_region region(power_limit_shm, bi::read_only);
  char* data = static_cast<char*>(region.get_address());
  std::vector<powerInfo> power_limits_set;
  power_limits_set.resize(power_handle_size);
  zes_uuid_t uuid;
  memcpy(uuid.id, data, ZES_MAX_UUID_SIZE);
  memcpy(power_limits_set.data(), data+ZES_MAX_UUID_SIZE, power_handle_size*sizeof(powerInfo));

  //2. read power limits set for the device
  auto driver = lzt::get_default_zes_driver();
  auto dev_count = lzt::get_zes_device_count(driver);
  auto devices = lzt::get_zes_devices(dev_count, driver);
  for (auto device : devices) {
    zes_uuid_t id = lzt::get_sysman_device_uuid(device);
    if (lzt::is_uuid_pair_equal(id.id, uuid.id)) {
      uint32_t count = 0;
      auto p_power_handles = lzt::get_power_handles(device, count);
      if (count == 0) {
        FAIL() << "No handles found: "
              << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
        test_result = false;
      }

      for (size_t p = 0; p < p_power_handles.size(); ++p) {
        EXPECT_NE(nullptr, p_power_handles[p]);
        uint32_t count_power = 0;
        zes_power_limit_ext_desc_t power_peak_get = {};
        std::vector<zes_power_limit_ext_desc_t> power_limits_descriptors;
        auto status = lzt::get_power_limits_ext(
            p_power_handles[p], &count_power,
            power_limits_descriptors); // get power limits for all descriptors
        if (status == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
          continue;
        }
        EXPECT_ZE_RESULT_SUCCESS(status);
        for (const auto &p_power_limits_descriptor : power_limits_descriptors) {
          if (p_power_limits_descriptor.level == ZES_POWER_LEVEL_PEAK) {
            power_peak_get = p_power_limits_descriptor;
          }
        }

        EXPECT_EQ(power_peak_get.limitValueLocked,
                  power_limits_set[p].limitValueLocked);
        EXPECT_EQ(power_peak_get.interval,
                  power_limits_set[p].interval);
        EXPECT_EQ(power_peak_get.limit, power_limits_set[p].limit);

        // Need to notify parent process about failure
        if (power_peak_get.limitValueLocked != power_limits_set[p].limitValueLocked ||
          power_peak_get.interval != power_limits_set[p].interval ||
          power_peak_get.limit != (power_limits_set[p].limit)) {
          test_result = false;
          return;
        }
      }
    }
  }
}

int main(int argc, char **argv) {
  if (argc != 2) {
    LOG_INFO << "Insufficient argument count " <<argc;
    return 1;
  }

  // Parent thread should pass number of power handles as argument
  uint32_t power_handle_size = std::stoi(argv[1]);

  ze_result_t result = zesInit(0);
  if (result != ZE_RESULT_SUCCESS) {
    exit(1);
  }

  bool test_result = true;
  GetPowerLimitsSetByParentProcess(power_handle_size, test_result);
  if (test_result == false) {
    return 1; // test failed, report the same to parent process
  }
  return 0;
}
