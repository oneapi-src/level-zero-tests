/*
 *
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "gtest/gtest.h"

#include "dx11_interop/dx11_helper.hpp"

struct DX11InteroperabilityTests : ::testing::Test {

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

    if (!devices_matched) {
      GTEST_SKIP() << "Device doesn't support DX11.";
    }

    if (FAILED(D3D11CreateDevice(
            dxgi_adapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr, 0, nullptr, 0,
            D3D11_SDK_VERSION, &dx11_device, nullptr, &dx11_device_context))) {
      throw std::runtime_error("Failed to create DX11 device and context.");
    }

    if (FAILED(dx11_device.As(&dx11_device5))) {
      throw std::runtime_error("Failed to query DX11 device5.");
    }

    if (FAILED(dx11_device_context.As(&dx11_device_context4))) {
      throw std::runtime_error("Failed to query ID3D11DeviceContext4.");
    }
  }

  ze_device_handle_t l0_device;

  ComPtr<IDXGIFactory1> dxgi_adapter_factory;
  ComPtr<IDXGIAdapter1> dxgi_adapter;
  ComPtr<ID3D11Device> dx11_device;
  ComPtr<ID3D11DeviceContext> dx11_device_context;
  ComPtr<ID3D11Device5> dx11_device5;
  ComPtr<ID3D11DeviceContext4> dx11_device_context4;
};
