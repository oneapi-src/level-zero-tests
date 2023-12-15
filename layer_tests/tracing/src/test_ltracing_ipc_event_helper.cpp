/*
 *
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "logging/logging.hpp"
#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"
#include "net/test_ipc_comm.hpp"
#include <boost/process.hpp>
#include <level_zero/ze_api.h>
#include <level_zero/layers/zel_tracing_api.h>
#include <level_zero/loader/ze_loader.h>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/process.hpp>

namespace lzt = level_zero_tests;
namespace bipc = boost::interprocess;

#ifdef __linux__

int main(int argc, char **argv) {

  std::string test_type_string = argv[1];

  ze_result_t result;
  if (zeInit(0) != ZE_RESULT_SUCCESS) {
    LOG_DEBUG << "Child exit due to zeInit failure";
    exit(EXIT_FAILURE);
  }
  #ifdef USE_RUNTIME_TRACING
  zelEnableTracingLayer();
  #endif

  ze_ipc_event_pool_handle_t hIpcEventPool{};
  int ipc_descriptor =
      lzt::receive_ipc_handle<ze_ipc_event_pool_handle_t>(hIpcEventPool.data);

  lzt::shared_ipc_event_data_t shared_data;
  bipc::shared_memory_object shm;
  shm = bipc::shared_memory_object(bipc::open_only, "ipc_ltracing_event_test",
                                   bipc::read_write);
  shm.truncate(sizeof(lzt::shared_ipc_event_data_t));
  bipc::mapped_region region(shm, bipc::read_only);
  std::memcpy(&shared_data, region.get_address(),
              sizeof(lzt::shared_ipc_event_data_t));

  memcpy(&(hIpcEventPool), static_cast<void *>(&ipc_descriptor),
         sizeof(ipc_descriptor));

  ze_context_handle_t context = lzt::get_default_context();

  lzt::test_api_ltracing_user_data user_data = {};
  zel_tracer_desc_t tracer_desc = {ZEL_STRUCTURE_TYPE_TRACER_DESC, nullptr,
                                   &user_data};
  zel_tracer_handle_t tracer_handle = lzt::create_ltracer_handle(tracer_desc);

  ze_callbacks_t prologues = {};
  ze_callbacks_t epilogues = {};
  if (test_type_string == "TEST_OPEN_IPC_EVENT") {
    prologues.EventPool.pfnOpenIpcHandleCb = lzt::lprologue_callback;
    epilogues.EventPool.pfnOpenIpcHandleCb = lzt::lepilogue_callback;
  } else if (test_type_string == "TEST_CLOSE_IPC_EVENT") {
    prologues.EventPool.pfnCloseIpcHandleCb = lzt::lprologue_callback;
    epilogues.EventPool.pfnCloseIpcHandleCb = lzt::lepilogue_callback;
  } else {
    exit(EXIT_FAILURE);
  }
  lzt::set_ltracer_prologues(tracer_handle, prologues);
  lzt::set_ltracer_epilogues(tracer_handle, epilogues);
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
  #ifdef USE_RUNTIME_TRACING
  zelDisableTracingLayer();
  #endif

  exit(EXIT_SUCCESS);
}
#else // windows
int main() { exit(EXIT_SUCCESS); }
#endif
