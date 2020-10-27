/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef level_zero_tests_TEST_HARNESS_LTRACING_HPP
#define level_zero_tests_TEST_HARNESS_LTRACING_HPP

#include <level_zero/ze_api.h>
#include <level_zero/layers/zel_tracing_api.h>

namespace level_zero_tests {

typedef struct _test_api_ltracing_user_data {
  bool prologue_called;
  bool epilogue_called;
} test_api_ltracing_user_data;

zel_tracer_handle_t create_ltracer_handle(const zel_tracer_desc_t tracer_desc);

void destroy_ltracer_handle(zel_tracer_handle_t tracer_handle);

void set_ltracer_prologues(const zel_tracer_handle_t tracer_handle);
void set_ltracer_prologues(zel_tracer_handle_t tracer_handle,
                           zet_core_callbacks_t prologues);

void set_ltracer_epilogues(const zel_tracer_handle_t tracer_handle);
void set_ltracer_epilogues(zel_tracer_handle_t tracer_handle,
                           zet_core_callbacks_t epilogues);
void enable_ltracer(const zel_tracer_handle_t tracer_handle);

void disable_ltracer(const zel_tracer_handle_t tracer_handle);

template <typename params_type>
void lprologue_callback(params_type params, ze_result_t result,
                        void *pTracerUserData,
                        void **ppTracerInstanceUserData) {
  test_api_ltracing_user_data *udata =
      (test_api_ltracing_user_data *)pTracerUserData;

  udata->prologue_called = true;
}

template <typename params_type>
void lepilogue_callback(params_type params, ze_result_t result,
                        void *pTracerUserData,
                        void **ppTracerInstanceUserData) {
  test_api_ltracing_user_data *udata =
      (test_api_ltracing_user_data *)pTracerUserData;

  udata->epilogue_called = true;
}

}; // namespace level_zero_tests

#endif
