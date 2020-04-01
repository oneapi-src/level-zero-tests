# Copyright (C) 2019 Intel Corporation
# SPDX-License-Identifier: MIT

set(ze_imagecopy_libraries
Boost::program_options
${OS_SPECIFIC_LIBS}
utils
)



add_lzt_test(
  NAME ze_image_copy
  GROUP "/perf_tests"
  SOURCES
    ../common/src/ze_app.cpp
    src/ze_image_copy.cpp
    src/options.cpp
  LINK_LIBRARIES ${ze_imagecopy_libraries} 
)
