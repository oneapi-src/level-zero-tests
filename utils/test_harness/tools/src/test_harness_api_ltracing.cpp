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
#include <level_zero/layers/zel_tracing_api.h>

namespace lzt = level_zero_tests;

namespace level_zero_tests {

zel_tracer_handle_t create_ltracer_handle(const zel_tracer_desc_t tracer_desc) {
  zel_tracer_handle_t tracer_handle;

  EXPECT_ZE_RESULT_SUCCESS(zelTracerCreate(&tracer_desc, &tracer_handle));

  return tracer_handle;
}

void destroy_ltracer_handle(zel_tracer_handle_t tracer_handle) {
  EXPECT_ZE_RESULT_SUCCESS(zelTracerDestroy(tracer_handle));
}

void set_ltracer_prologues(zel_tracer_handle_t tracer_handle,
                           zet_core_callbacks_t prologues) {
  EXPECT_ZE_RESULT_SUCCESS(zelTracerSetPrologues(tracer_handle, &prologues));
}

void set_ltracer_epilogues(const zel_tracer_handle_t tracer_handle,
                           zet_core_callbacks_t epilogues) {
  EXPECT_ZE_RESULT_SUCCESS(zelTracerSetEpilogues(tracer_handle, &epilogues));
}

void enable_ltracer(zel_tracer_handle_t tracer_handle) {
  EXPECT_ZE_RESULT_SUCCESS(zelTracerSetEnabled(tracer_handle, true));
}

void disable_ltracer(zel_tracer_handle_t tracer_handle) {
  EXPECT_ZE_RESULT_SUCCESS(zelTracerSetEnabled(tracer_handle, false));
}

} // namespace level_zero_tests
