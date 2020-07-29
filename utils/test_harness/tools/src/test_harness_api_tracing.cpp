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

zet_tracer_exp_handle_t
create_tracer_handle(const zet_tracer_exp_desc_t tracer_desc) {
  return create_tracer_handle(lzt::get_default_context(), tracer_desc);
}

zet_tracer_exp_handle_t
create_tracer_handle(const zet_context_handle_t context,
                     const zet_tracer_exp_desc_t tracer_desc) {
  zet_tracer_exp_handle_t tracer_handle;

  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetTracerExpCreate(context, &tracer_desc, &tracer_handle));

  return tracer_handle;
}

void destroy_tracer_handle(zet_tracer_exp_handle_t tracer_handle) {
  EXPECT_EQ(ZE_RESULT_SUCCESS, zetTracerExpDestroy(tracer_handle));
}

void set_tracer_prologues(zet_tracer_exp_handle_t tracer_handle,
                          zet_core_callbacks_t prologues) {
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetTracerExpSetPrologues(tracer_handle, &prologues));
}

void set_tracer_epilogues(const zet_tracer_exp_handle_t tracer_handle,
                          zet_core_callbacks_t epilogues) {
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zetTracerExpSetEpilogues(tracer_handle, &epilogues));
}

void enable_tracer(zet_tracer_exp_handle_t tracer_handle) {
  EXPECT_EQ(ZE_RESULT_SUCCESS, zetTracerExpSetEnabled(tracer_handle, true));
}

void disable_tracer(zet_tracer_exp_handle_t tracer_handle) {
  EXPECT_EQ(ZE_RESULT_SUCCESS, zetTracerExpSetEnabled(tracer_handle, false));
}

} // namespace level_zero_tests
