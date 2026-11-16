/*
 *
 * Copyright (C) 2019-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */
#include "ze_peer.h"

void ZePeer::perform_copy(peer_test_t test_type,
                          ze_command_list_handle_t command_list,
                          ze_command_queue_handle_t command_queue,
                          void *dst_buffer, void *src_buffer,
                          size_t buffer_size,
                          std::optional<ze_peer_engine_t> remote_engine) {
  const ze_event_handle_t signal_event =
      remote_engine ? remote_wait_event : nullptr;

  SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(command_list, dst_buffer,
                                                     src_buffer, buffer_size,
                                                     signal_event, 0, nullptr));
  SUCCESS_OR_TERMINATE(zeCommandListClose(command_list));
  if (remote_engine) {
    SUCCESS_OR_TERMINATE(zeCommandListAppendWaitOnEvents(
        remote_engine->second, 1, &remote_wait_event));
    SUCCESS_OR_TERMINATE(zeCommandListAppendEventReset(remote_engine->second,
                                                       remote_wait_event));
    SUCCESS_OR_TERMINATE(zeCommandListClose(remote_engine->second));
  }

  const auto execute_and_synchronize = [&]() {
    SUCCESS_OR_TERMINATE(zeCommandQueueExecuteCommandLists(
        command_queue, 1, &command_list, nullptr));
    if (remote_engine) {
      SUCCESS_OR_TERMINATE(zeCommandQueueExecuteCommandLists(
          remote_engine->first, 1, &remote_engine->second, nullptr));
    }
    SUCCESS_OR_TERMINATE(zeCommandQueueSynchronize(
        command_queue, std::numeric_limits<uint64_t>::max()));
    if (remote_engine) {
      SUCCESS_OR_TERMINATE(zeCommandQueueSynchronize(
          remote_engine->first, std::numeric_limits<uint64_t>::max()));
    }
  };

  Timer<std::chrono::microseconds::period> timer;

  /* Warm up */
  for (uint32_t i = 0U; i < warm_up_iterations; i++) {
    execute_and_synchronize();
  }

  do {
    timer.start();
    for (uint32_t i = 0U; i < number_iterations; i++) {
      execute_and_synchronize();
    }
    timer.end();

    print_results(false, test_type, buffer_size, timer);
  } while (run_continuously);

  SUCCESS_OR_TERMINATE(zeCommandListReset(command_list));
  if (remote_engine) {
    SUCCESS_OR_TERMINATE(zeCommandListReset(remote_engine->second));
  }
}

void ZePeer::perform_copy_immediate(
    peer_test_t test_type, ze_command_list_handle_t command_list,
    void *dst_buffer, void *src_buffer, size_t buffer_size,
    std::optional<ze_peer_engine_t> remote_engine) {
  const ze_event_handle_t signal_event =
      remote_engine ? remote_wait_event : nullptr;

  const auto copy_and_synchronize = [&]() {
    SUCCESS_OR_TERMINATE(
        zeCommandListAppendMemoryCopy(command_list, dst_buffer, src_buffer,
                                      buffer_size, signal_event, 0, nullptr));
    if (remote_engine) {
      SUCCESS_OR_TERMINATE(zeCommandListAppendWaitOnEvents(
          remote_engine->second, 1, &remote_wait_event));
      SUCCESS_OR_TERMINATE(zeCommandListAppendEventReset(remote_engine->second,
                                                         remote_wait_event));
    }
    SUCCESS_OR_TERMINATE(zeCommandListHostSynchronize(
        command_list, std::numeric_limits<uint64_t>::max()));
    if (remote_engine) {
      SUCCESS_OR_TERMINATE(zeCommandListHostSynchronize(
          remote_engine->second, std::numeric_limits<uint64_t>::max()));
    }
  };

  Timer<std::chrono::microseconds::period> timer;

  /* Warm up */
  for (uint32_t i = 0U; i < warm_up_iterations; i++) {
    copy_and_synchronize();
  }

  do {
    timer.start();
    for (uint32_t i = 0U; i < number_iterations; i++) {
      copy_and_synchronize();
    }
    timer.end();

    print_results(false, test_type, buffer_size, timer);
  } while (run_continuously);

  SUCCESS_OR_TERMINATE(zeCommandListReset(command_list));
}

void ZePeer::bandwidth_latency(peer_test_t test_type,
                               peer_transfer_t transfer_type,
                               size_t number_buffer_elements,
                               uint32_t remote_device_id,
                               uint32_t local_device_id, uint32_t queue_index) {

  size_t buffer_size = 0;

  ze_command_queue_handle_t command_queue =
      ze_peer_devices[local_device_id].engines[queue_index].first;
  ze_command_list_handle_t command_list =
      ze_peer_devices[local_device_id].engines[queue_index].second;

  std::vector<uint32_t> remote_device_ids = {remote_device_id};
  std::vector<uint32_t> local_device_ids = {local_device_id};
  set_up(number_buffer_elements, remote_device_ids, local_device_ids,
         buffer_size);

  void *dst_buffer = ze_dst_buffers[remote_device_id];
  void *src_buffer = ze_src_buffers[local_device_id];
  if (transfer_type == PEER_READ) {
    dst_buffer = ze_dst_buffers[local_device_id];
    src_buffer = ze_src_buffers[remote_device_id];
  }

  initialize_buffers(remote_device_ids, local_device_ids, ze_host_buffer,
                     buffer_size);

  // Engine 0 always exists; the power effect of a wait is device specific.
  const std::optional<ze_peer_engine_t> remote_engine =
      ZePeer::remote_wait
          ? std::make_optional(ze_peer_devices[remote_device_id].engines[0])
          : std::nullopt;

  if (ZePeer::use_immediate_cmdlist) {
    perform_copy_immediate(test_type, command_list, dst_buffer, src_buffer,
                           buffer_size, remote_engine);
  } else {
    perform_copy(test_type, command_list, command_queue, dst_buffer, src_buffer,
                 buffer_size, remote_engine);
  }

  if (validate_results) {
    if (ZePeer::use_immediate_cmdlist) {
      validate_buffer_immediate(command_list, ze_host_validate_buffer,
                                dst_buffer, ze_host_buffer, buffer_size);
    } else {
      validate_buffer(command_list, command_queue, ze_host_validate_buffer,
                      dst_buffer, ze_host_buffer, buffer_size);
    }
  }

  tear_down(remote_device_ids, local_device_ids);
}
