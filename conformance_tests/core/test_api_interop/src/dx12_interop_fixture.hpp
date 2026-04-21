/*
 *
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "gtest/gtest.h"

#include "dx12_helper.hpp"

struct DX12InteroperabilityTests : ::testing::Test {

  void SetUp() override {
    l0_device = lzt::zeDevice::get_instance()->get_device();
    ze_device_properties_t l0_device_props =
        lzt::get_device_properties(l0_device);

    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&dxgi_adapter_factory)))) {
      throw std::runtime_error("Failed to create DXGI adapter factory.");
    }

    bool devices_matched = false;
    for (UINT i = 0; dxgi_adapter_factory->EnumAdapters1(i, &dxgi_adapter) !=
                     DXGI_ERROR_NOT_FOUND;
         ++i) {
      DXGI_ADAPTER_DESC1 desc;
      dxgi_adapter->GetDesc1(&desc);
      if (desc.DeviceId == l0_device_props.deviceId) {
        devices_matched = true;
        break;
      }
    }

    EXPECT_TRUE(devices_matched) << "Device doesn't support DX12.";

    if (FAILED(D3D12CreateDevice(dxgi_adapter.Get(), D3D_FEATURE_LEVEL_11_0,
                                 IID_PPV_ARGS(&dx12_device)))) {
      throw std::runtime_error("Failed to create DX12 device.");
    }
  }

  ze_device_handle_t l0_device;

  ComPtr<IDXGIFactory1> dxgi_adapter_factory;
  ComPtr<IDXGIAdapter1> dxgi_adapter;
  ComPtr<ID3D12Device> dx12_device;
};
