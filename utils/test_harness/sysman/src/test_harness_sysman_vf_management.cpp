/*
 *
 * Copyright (C) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "test_harness/test_harness.hpp"

#include <level_zero/zes_api.h>
#include "utils/utils.hpp"

namespace level_zero_tests {

uint32_t get_vf_handles_count(zes_device_handle_t device) {
  uint32_t count = 0;
  EXPECT_ZE_RESULT_SUCCESS(zesDeviceEnumEnabledVFExp(device, &count, nullptr));
  return count;
}

std::vector<zes_vf_handle_t> get_vf_handles(zes_device_handle_t device,
                                            uint32_t &count) {
  if (count == 0) {
    count = get_vf_handles_count(device);
  }
  std::vector<zes_vf_handle_t> vf_handles(count);
  EXPECT_ZE_RESULT_SUCCESS(
      zesDeviceEnumEnabledVFExp(device, &count, vf_handles.data()));
  return vf_handles;
}

zes_vf_exp2_capabilities_t get_vf_capabilities(zes_vf_handle_t vf_handle) {
  zes_vf_exp2_capabilities_t vf_capabilities = {
      ZES_STRUCTURE_TYPE_VF_EXP2_CAPABILITIES};
  EXPECT_ZE_RESULT_SUCCESS(
      zesVFManagementGetVFCapabilitiesExp2(vf_handle, &vf_capabilities));
  return vf_capabilities;
}

uint32_t get_vf_mem_util_count(zes_vf_handle_t vf_handle) {
  uint32_t count = 0;
  EXPECT_ZE_RESULT_SUCCESS(
      zesVFManagementGetVFMemoryUtilizationExp2(vf_handle, &count, nullptr));
  return count;
}

std::vector<zes_vf_util_mem_exp2_t> get_vf_mem_util(zes_vf_handle_t vf_handle,
                                                    uint32_t &count) {
  if (count == 0) {
    count = get_vf_mem_util_count(vf_handle);
  }
  std::vector<zes_vf_util_mem_exp2_t> vf_util_mem_exp(count);
  for (uint32_t i = 0; i < count; i++) {
    vf_util_mem_exp[i].stype = ZES_STRUCTURE_TYPE_VF_UTIL_MEM_EXP2;
  }
  EXPECT_ZE_RESULT_SUCCESS(zesVFManagementGetVFMemoryUtilizationExp2(
      vf_handle, &count, vf_util_mem_exp.data()));
  return vf_util_mem_exp;
}

uint32_t get_vf_engine_util_count(zes_vf_handle_t vf_handle) {
  uint32_t count = 0;
  EXPECT_ZE_RESULT_SUCCESS(
      zesVFManagementGetVFEngineUtilizationExp2(vf_handle, &count, nullptr));
  return count;
}

std::vector<zes_vf_util_engine_exp2_t>
get_vf_engine_util(zes_vf_handle_t vf_handle, uint32_t &count) {
  if (count == 0) {
    count = get_vf_engine_util_count(vf_handle);
  }
  std::vector<zes_vf_util_engine_exp2_t> vf_util_engine_exp(count);
  for (uint32_t i = 0; i < count; i++) {
    vf_util_engine_exp[i].stype = ZES_STRUCTURE_TYPE_VF_UTIL_ENGINE_EXP2;
  }
  EXPECT_ZE_RESULT_SUCCESS(zesVFManagementGetVFEngineUtilizationExp2(
      vf_handle, &count, vf_util_engine_exp.data()));
  return vf_util_engine_exp;
}

} // namespace level_zero_tests
