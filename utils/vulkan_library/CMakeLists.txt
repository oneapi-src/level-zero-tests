# Copyright (C) 2026 Intel Corporation
# SPDX-License-Identifier: MIT

include(FetchContent)

FetchContent_Declare(
    VulkanHeaders
    GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
    GIT_TAG v1.4.351
)
FetchContent_MakeAvailable(VulkanHeaders)

add_core_library(vulkan_library
  SOURCE
  "include/vulkan_library/vulkan_library.hpp"
  "src/vulkan_library.cpp"
  $<$<BOOL:${UNIX}>:src/vulkan_library_linux.cpp>
  $<$<BOOL:${WIN32}>:src/vulkan_library_windows.cpp>
)
target_link_libraries(vulkan_library
  PUBLIC
  Vulkan::Headers
  PRIVATE
  level_zero_tests::logging
  level_zero_tests::system
  level_zero_tests::utils
)
target_compile_definitions(vulkan_library
  PUBLIC
  VK_NO_PROTOTYPES
  $<$<BOOL:${WIN32}>:VK_USE_PLATFORM_WIN32_KHR>
  $<$<BOOL:${WIN32}>:WIN32_LEAN_AND_MEAN>
  $<$<BOOL:${WIN32}>:NOMINMAX>
)
