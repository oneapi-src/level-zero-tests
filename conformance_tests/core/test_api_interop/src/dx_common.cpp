/*
 *
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "dx_common.hpp"

#include "test_harness/test_harness_device.hpp"

namespace lzt = level_zero_tests;

namespace dx {

std::string hr_to_string(HRESULT result) {
  char *msg = nullptr;
  FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                     FORMAT_MESSAGE_IGNORE_INSERTS,
                 nullptr, result, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                 (LPSTR)&msg, 0, nullptr);

  std::string str = msg ? msg : "Unknown error";
  if (msg)
    LocalFree(msg);
  return str;
}

ze_external_semaphore_ext_handle_t
import_fence(ze_device_handle_t l0_device, HANDLE shared_handle,
             ze_external_semaphore_ext_flags_t type) {
  ze_external_semaphore_win32_ext_desc_t external_semaphore_win32_desc = {
      .stype = ZE_STRUCTURE_TYPE_EXTERNAL_SEMAPHORE_WIN32_EXT_DESC,
      .handle = shared_handle,
  };

  ze_external_semaphore_ext_desc_t external_semaphore_desc = {
      .stype = ZE_STRUCTURE_TYPE_EXTERNAL_SEMAPHORE_EXT_DESC,
      .pNext = &external_semaphore_win32_desc,
      .flags = type};

  return lzt::import_external_semaphore(l0_device, &external_semaphore_desc);
}

} // namespace dx
