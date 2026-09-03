# Copyright (C) 2026 Intel Corporation
# SPDX-License-Identifier: MIT

include(FetchContent)

FetchContent_Declare(
    VulkanHeaders
    GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
    GIT_TAG v1.4.351
)
FetchContent_MakeAvailable(VulkanHeaders)

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
    $<$<BOOL:${WIN32}>:src/vulkan_interop/vulkan_interop_helper_windows.cpp>
    $<$<BOOL:${WIN32}>:src/vulkan_interop/vulkan_library_windows.cpp>
    $<$<BOOL:${WIN32}>:src/dx_common.cpp>

    $<$<BOOL:${UNIX}>:src/vulkan_interop/vulkan_interop_helper_linux.cpp>
    $<$<BOOL:${UNIX}>:src/vulkan_interop/vulkan_library_linux.cpp>

    include/vulkan_interop/vulkan_interop_fixture.hpp
    include/vulkan_interop/vulkan_interop_helper.hpp
    include/vulkan_interop/vulkan_library.hpp
    src/dx11_interop/test_dx11_interop.cpp
    src/dx12_interop/test_dx12_interop.cpp
    src/vulkan_interop/test_vulkan_interop.cpp
    src/vulkan_interop/vulkan_library.cpp
    src/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../test_image/src/utils.cpp
  INCLUDE_DIRECTORIES
    ${CMAKE_CURRENT_SOURCE_DIR}/../test_image/include
  LINK_LIBRARIES
    $<$<BOOL:${WIN32}>:dxgi.lib>
    $<$<BOOL:${WIN32}>:d3d11.lib>
    $<$<BOOL:${WIN32}>:d3d12.lib>
    
    level_zero_tests::logging
    level_zero_tests::random
    level_zero_tests::system
    level_zero_tests::utils
    Vulkan::Headers
  KERNELS
    interop_double
  DEFINES
    $<$<BOOL:${WIN32}>:VK_USE_PLATFORM_WIN32_KHR>
    $<$<BOOL:${WIN32}>:WIN32_LEAN_AND_MEAN>
    $<$<BOOL:${WIN32}>:NOMINMAX>
)
