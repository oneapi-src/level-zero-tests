/*
 *
 * Copyright (C) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "test_harness/test_harness.hpp"

namespace level_zero_tests {
ze_result_t get_driver_ext_properties(zes_driver_handle_t driver,
                                      uint32_t *count) {
  return zesDriverGetExtensionProperties(driver, count, nullptr);
}

void get_driver_ext_properties(
    zes_driver_handle_t driver, uint32_t *count,
    std::vector<zes_driver_extension_properties_t> &ext_properties) {
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesDriverGetExtensionProperties(
                                   driver, count, ext_properties.data()));
}

void get_driver_ext_function_address(zes_driver_handle_t driver,
                                     const char *function_name) {
  void *func_ptr = nullptr;
  EXPECT_EQ(ZE_RESULT_SUCCESS, zesDriverGetExtensionFunctionAddress(
                                   driver, function_name, &func_ptr));
  EXPECT_NE(func_ptr, nullptr);
}
} // namespace level_zero_tests