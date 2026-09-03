#
# Copyright (C) 2026 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

add_core_library(system
  SOURCE
  "include/system/shared_library.hpp"
  $<$<BOOL:${UNIX}>:src/shared_library_linux.cpp>
  $<$<BOOL:${WIN32}>:src/shared_library_windows.cpp>
)

target_link_libraries(system PRIVATE
  $<$<BOOL:${UNIX}>:${CMAKE_DL_LIBS}>
)
