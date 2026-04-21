/*
 *
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "test_harness/test_harness.hpp"
#include "utils/utils.hpp"

#ifdef __linux__
struct DX11InteroperabilityTests : ::testing::Test {
  void SetUp() override { GTEST_SKIP() << "Not supported on Linux"; }
};
#else
#include "dx11_interop_fixture.hpp"
#endif

LZT_TEST_F(DX11InteroperabilityTests,
           GivenDX11SharedFenceWhenImportingExternalSemaphoreThenIsSuccess) {
#ifndef __linux__
  ComPtr<ID3D11Device5> dx11_device5;
  if (FAILED(dx11_device.As(&dx11_device5))) {
    throw std::runtime_error("Failed to query DX11 device5.");
  }

  ComPtr<ID3D11Fence> fence;
  if (FAILED(dx11_device5->CreateFence(0, D3D11_FENCE_FLAG_SHARED,
                                       IID_PPV_ARGS(&fence)))) {
    throw std::runtime_error("Failed to create DX11 fence.");
  }

  HANDLE handle;
  if (FAILED(
          fence->CreateSharedHandle(nullptr, GENERIC_ALL, nullptr, &handle))) {
    throw std::runtime_error(
        "Failed to create shared handle for a DX11 fence.");
  }

  ze_external_semaphore_win32_ext_desc_t external_semaphore_win32_desc = {};
  external_semaphore_win32_desc.stype =
      ZE_STRUCTURE_TYPE_EXTERNAL_SEMAPHORE_WIN32_EXT_DESC;
  external_semaphore_win32_desc.handle = handle;

  ze_external_semaphore_ext_desc_t external_semaphore_desc = {};
  external_semaphore_desc.stype = ZE_STRUCTURE_TYPE_EXTERNAL_SEMAPHORE_EXT_DESC;
  external_semaphore_desc.pNext = &external_semaphore_win32_desc;
  external_semaphore_desc.flags = ZE_EXTERNAL_SEMAPHORE_EXT_FLAG_D3D11_FENCE;

  ze_external_semaphore_ext_handle_t external_semaphore_handle = nullptr;
  EXPECT_ZE_RESULT_SUCCESS(zeDeviceImportExternalSemaphoreExt(
      l0_device, &external_semaphore_desc, &external_semaphore_handle));

  EXPECT_NE(external_semaphore_handle, nullptr);

  EXPECT_ZE_RESULT_SUCCESS(
      zeDeviceReleaseExternalSemaphoreExt(external_semaphore_handle));
#endif
}
