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
#include <level_zero/zet_api.h>
#include <string>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/process.hpp>

namespace lzt = level_zero_tests;
namespace bipc = boost::interprocess;

#ifdef __linux__

int main(int argc, char **argv) {

  std::string test_type_string = argv[1];

  ze_result_t result;
  if (zeInit(1) != ZE_RESULT_SUCCESS) {
    LOG_DEBUG << "Child exit due to zeInit failure";
    exit(EXIT_FAILURE);
  }

  ze_ipc_event_pool_handle_t hIpcEventPool{};
  int ipc_descriptor =
      lzt::receive_ipc_handle<ze_ipc_event_pool_handle_t>(hIpcEventPool.data);

  lzt::shared_ipc_event_data_t shared_data;
  bipc::shared_memory_object shm;
  shm = bipc::shared_memory_object(bipc::open_only, "ipc_tracing_event_test",
                                   bipc::read_write);
  shm.truncate(sizeof(lzt::shared_ipc_event_data_t));
  bipc::mapped_region region(shm, bipc::read_only);
  std::memcpy(&shared_data, region.get_address(),
              sizeof(lzt::shared_ipc_event_data_t));

  memcpy(&(hIpcEventPool), static_cast<void *>(&ipc_descriptor),
         sizeof(ipc_descriptor));

  ze_context_handle_t context = lzt::get_default_context();

  lzt::test_api_tracing_user_data user_data = {};
  zet_tracer_exp_desc_t tracer_desc = {ZET_STRUCTURE_TYPE_TRACER_EXP_DESC,
                                       nullptr, &user_data};
  zet_tracer_exp_handle_t tracer_handle =
      lzt::create_tracer_handle(context, tracer_desc);

  ze_callbacks_t prologues = {};
  ze_callbacks_t epilogues = {};
  if (test_type_string == "TEST_OPEN_IPC_EVENT") {
    prologues.EventPool.pfnOpenIpcHandleCb = lzt::prologue_callback;
    epilogues.EventPool.pfnOpenIpcHandleCb = lzt::epilogue_callback;
  } else if (test_type_string == "TEST_CLOSE_IPC_EVENT") {
    prologues.EventPool.pfnCloseIpcHandleCb = lzt::prologue_callback;
    epilogues.EventPool.pfnCloseIpcHandleCb = lzt::epilogue_callback;
  } else {
    exit(EXIT_FAILURE);
  }
  lzt::set_tracer_prologues(tracer_handle, prologues);
  lzt::set_tracer_epilogues(tracer_handle, epilogues);
  lzt::enable_tracer(tracer_handle);

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

  lzt::disable_tracer(tracer_handle);
  lzt::destroy_tracer_handle(tracer_handle);

  exit(EXIT_SUCCESS);
}
#else // windows
int main() { exit(EXIT_SUCCESS); }
#endif
