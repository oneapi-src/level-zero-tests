/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "test_harness/test_harness.hpp"
#include "gtest/gtest.h"
#include "logging/logging.hpp"

#include <level_zero/ze_api.h>

namespace lzt = level_zero_tests;

namespace level_zero_tests {

zet_tracer_handle_t create_tracer_handle(const zet_tracer_desc_t tracer_desc) {
  return create_tracer_handle(zeDevice::get_instance()->get_driver(),
                              tracer_desc);
}

zet_tracer_handle_t create_tracer_handle(const ze_driver_handle_t driver,
                                         const zet_tracer_desc_t tracer_desc) {
  zet_tracer_handle_t tracer_handle;

  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetTracerCreate(driver, &tracer_desc, &tracer_handle));

  return tracer_handle;
}

void destroy_tracer_handle(zet_tracer_handle_t tracer_handle) {
  EXPECT_EQ(ZE_RESULT_SUCCESS, zetTracerDestroy(tracer_handle));
}

void set_tracer_prologues(zet_tracer_handle_t tracer_handle,
                          zet_core_callbacks_t prologues) {
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetTracerSetPrologues(tracer_handle, &prologues));
}

void set_tracer_epilogues(const zet_tracer_handle_t tracer_handle,
                          zet_core_callbacks_t epilogues) {
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetTracerSetEpilogues(tracer_handle, &epilogues));
}

void enable_tracer(zet_tracer_handle_t tracer_handle) {
  EXPECT_EQ(ZE_RESULT_SUCCESS, zetTracerSetEnabled(tracer_handle, true));
}

void disable_tracer(zet_tracer_handle_t tracer_handle) {
  EXPECT_EQ(ZE_RESULT_SUCCESS, zetTracerSetEnabled(tracer_handle, false));
}

} // namespace level_zero_tests
