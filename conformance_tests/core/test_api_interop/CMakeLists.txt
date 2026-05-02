# Copyright (C) 2026 Intel Corporation
# SPDX-License-Identifier: MIT

add_lzt_test(
  NAME test_api_interop
  GROUP "/conformance_tests/core"
  SOURCES
    $<$<BOOL:${WIN32}>:include/dx11_interop/dx11_helper.hpp>
    $<$<BOOL:${WIN32}>:include/dx11_interop/dx11_interop_fixture.hpp>
    $<$<BOOL:${WIN32}>:include/dx12_interop/dx12_helper.hpp>
    $<$<BOOL:${WIN32}>:include/dx12_interop/dx12_interop_fixture.hpp>
    $<$<BOOL:${WIN32}>:include/dx_common.hpp>
    $<$<BOOL:${WIN32}>:src/dx11_interop/dx11_helper.cpp>
    $<$<BOOL:${WIN32}>:src/dx12_interop/dx12_helper.cpp>
    $<$<BOOL:${WIN32}>:src/dx_common.cpp>
    src/dx11_interop/test_dx11_interop.cpp
    src/dx12_interop/test_dx12_interop.cpp
    src/main.cpp
  LINK_LIBRARIES
    level_zero_tests::logging
    level_zero_tests::utils
    level_zero_tests::random
    $<$<BOOL:${WIN32}>:dxgi.lib>
    $<$<BOOL:${WIN32}>:d3d11.lib>
    $<$<BOOL:${WIN32}>:d3d12.lib>
)
