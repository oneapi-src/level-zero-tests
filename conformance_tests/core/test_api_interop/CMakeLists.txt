# Copyright (C) 2026 Intel Corporation
# SPDX-License-Identifier: MIT

add_lzt_test(
  NAME test_api_interop
  GROUP "/conformance_tests/core"
  SOURCES
    src/dx11_interop_fixture.hpp
    $<$<BOOL:${WIN32}>:src/dx12_helper.cpp>
    src/dx12_helper.hpp
    src/dx12_interop_fixture.hpp
    src/main.cpp
    src/test_dx11_interop.cpp
    src/test_dx12_interop.cpp
  LINK_LIBRARIES
    level_zero_tests::logging
    level_zero_tests::utils
    level_zero_tests::random
    $<$<BOOL:${WIN32}>:dxgi.lib>
    $<$<BOOL:${WIN32}>:d3d11.lib>
    $<$<BOOL:${WIN32}>:d3d12.lib>
)
