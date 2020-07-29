/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef level_zero_tests_TEST_HARNESS_TRACING_HPP
#define level_zero_tests_TEST_HARNESS_TRACING_HPP

#include <level_zero/ze_api.h>

namespace level_zero_tests {

typedef struct _test_api_tracing_user_data {
  bool prologue_called;
  bool epilogue_called;
} test_api_tracing_user_data;

zet_tracer_exp_handle_t
create_tracer_handle(const zet_tracer_exp_desc_t tracer_desc);
zet_tracer_exp_handle_t
create_tracer_handle(const zet_context_handle_t context,
                     const zet_tracer_exp_desc_t tracer_desc);

void destroy_tracer_handle(zet_tracer_exp_handle_t tracer_handle);

void set_tracer_prologues(const zet_tracer_exp_handle_t tracer_handle);
void set_tracer_prologues(zet_tracer_exp_handle_t tracer_handle,
                          zet_core_callbacks_t prologues);

void set_tracer_epilogues(const zet_tracer_exp_handle_t tracer_handle);
void set_tracer_epilogues(zet_tracer_exp_handle_t tracer_handle,
                          zet_core_callbacks_t epilogues);
void enable_tracer(const zet_tracer_exp_handle_t tracer_handle);

void disable_tracer(const zet_tracer_exp_handle_t tracer_handle);

template <typename params_type>
void prologue_callback(params_type params, ze_result_t result,
                       void *pTracerUserData, void **ppTracerInstanceUserData) {
  test_api_tracing_user_data *udata =
      (test_api_tracing_user_data *)pTracerUserData;

  udata->prologue_called = true;
}

template <typename params_type>
void epilogue_callback(params_type params, ze_result_t result,
                       void *pTracerUserData, void **ppTracerInstanceUserData) {
  test_api_tracing_user_data *udata =
      (test_api_tracing_user_data *)pTracerUserData;

  udata->epilogue_called = true;
}

}; // namespace level_zero_tests

#endif
