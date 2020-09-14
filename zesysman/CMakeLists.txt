# Copyright (C) 2020 Intel Corporation
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

find_package(Python3 COMPONENTS Interpreter Development)
find_package(SWIG REQUIRED)

include(${SWIG_USE_FILE})
set(UseSWIG_TARGET_NAME_PREFERENCE STANDARD)


include_directories(${LevelZero_INCLUDE_DIRS})
include_directories(${Python3_INCLUDE_DIRS})

swig_add_library(zes_wrap
    TYPE SHARED
    LANGUAGE python
    SOURCES 
        zes_wrap.i)

swig_link_libraries(zes_wrap
        PRIVATE
          LevelZero::LevelZero
     )

set(destination "${CMAKE_INSTALL_PREFIX}/zesysman")


install(
      TARGETS ${SWIG_MODULE_zes_wrap_REAL_NAME}
      DESTINATION ${destination}/zesys
      COMPONENT zesysman
)

install(
      FILES ${CMAKE_CURRENT_BINARY_DIR}/zes_wrap.py
      FILES zesys/logger.py
      FILES zesys/__init__.py
      DESTINATION ${destination}/zesys
      COMPONENT zesysman
)

install(
      FILES zesysman
      DESTINATION ${destination}
      PERMISSIONS OWNER_EXECUTE OWNER_READ OWNER_WRITE
      COMPONENT zesysman
)
