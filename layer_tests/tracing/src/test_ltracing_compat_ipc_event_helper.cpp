/*
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"
#include "logging/logging.hpp"
#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"
#include "net/test_ipc_comm.hpp"
#include <boost/process.hpp>
#include <level_zero/ze_api.h>
#include <level_zero/layers/zel_tracing_api.h>
#include <level_zero/layers/zel_tracing_register_cb.h>

namespace lzt = level_zero_tests;

#ifdef __linux__

int main(int argc, char **argv) {

  std::string test_type_string = argv[1];

  ze_result_t result;
  if (zeInit(0) != ZE_RESULT_SUCCESS) {
    LOG_DEBUG << "Child exit due to zeInit failure";
    exit(EXIT_FAILURE);
  }

  int ipc_descriptor = lzt::receive_ipc_handle<ze_ipc_event_pool_handle_t>();
  ze_ipc_event_pool_handle_t hIpcEventPool = {};
  memcpy(&(hIpcEventPool), static_cast<void *>(&ipc_descriptor),
         sizeof(ipc_descriptor));

  ze_context_handle_t context = lzt::get_default_context();

  zel_tracer_desc_t tracer_desc = {};
  lzt::test_api_ltracing_user_data user_data = {};
  tracer_desc.pUserData = &user_data;
  zel_tracer_handle_t tracer_handle = lzt::create_ltracer_handle(tracer_desc);

  if (test_type_string == "TEST_OPEN_IPC_EVENT") {
    zelTracerEventPoolOpenIpcHandleRegisterCallback(
        tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
    zelTracerEventPoolOpenIpcHandleRegisterCallback(
        tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);

  } else if (test_type_string == "TEST_CLOSE_IPC_EVENT") {

    zelTracerEventPoolCloseIpcHandleRegisterCallback(
        tracer_handle, ZEL_REGISTER_PROLOGUE, lzt::lprologue_callback);
    zelTracerEventPoolCloseIpcHandleRegisterCallback(
        tracer_handle, ZEL_REGISTER_EPILOGUE, lzt::lepilogue_callback);
  } else {
    exit(EXIT_FAILURE);
  }
  lzt::enable_ltracer(tracer_handle);

  ze_event_pool_handle_t hEventPool = 0;
  lzt::open_ipc_event_handle(lzt::get_default_context(), hIpcEventPool,
                             &hEventPool);
  if (!hEventPool) {
    LOG_DEBUG << "Child exit due to null event pool";
    exit(EXIT_FAILURE);
  }

  lzt::close_ipc_event_handle(hEventPool);

  if ((user_data.prologue_called == 0) || (user_data.epilogue_called == 0))
    exit(EXIT_FAILURE);

  lzt::disable_ltracer(tracer_handle);
  lzt::destroy_ltracer_handle(tracer_handle);

  exit(EXIT_SUCCESS);
}
#else // windows
int main() { exit(EXIT_SUCCESS); }
#endif
