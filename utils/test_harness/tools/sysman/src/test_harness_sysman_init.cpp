/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "test_harness/test_harness.hpp"

#include <level_zero/ze_api.h>
#include "utils/utils.hpp"

namespace lzt = level_zero_tests;

namespace level_zero_tests {

zet_sysman_handle_t get_sysman_handle(ze_device_handle_t device) {
  zet_sysman_handle_t hSysmanDevice;
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetSysmanGet(device, ZET_SYSMAN_VERSION_CURRENT, &hSysmanDevice));
  return hSysmanDevice;
}

}; // namespace level_zero_tests
