# Copyright (C) 2020 Intel Corporation
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

find_package(Python3 COMPONENTS Interpreter Development)
find_package(SWIG REQUIRED)

include(${SWIG_USE_FILE})
set(UseSWIG_TARGET_NAME_PREFERENCE STANDARD)


include_directories(${LevelZero_INCLUDE_DIRS})
include_directories(${Python3_INCLUDE_DIRS})

swig_add_library(zesysman
    TYPE SHARED
    LANGUAGE python
    SOURCES 
        zesysman.i)

swig_link_libraries(zesysman
        PRIVATE
          LevelZero::LevelZero
     )

set(destination "${CMAKE_INSTALL_PREFIX}/zesysman")


install(
      TARGETS ${SWIG_MODULE_zesysman_REAL_NAME}
      DESTINATION ${destination}
      COMPONENT zesysman
)

install(
      FILES ${CMAKE_CURRENT_BINARY_DIR}/zesysman.py
      DESTINATION ${destination}
      COMPONENT zesysman
)

install(
      FILES zesysman
      DESTINATION ${destination}
      PERMISSIONS OWNER_EXECUTE OWNER_READ OWNER_WRITE
      COMPONENT zesysman
)