/*
 *
 * Copyright (C) 2019-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */
#include "ze_peer.h"

void ZePeer::bidirectional_append_copies(
    uint32_t remote_device_id, uint32_t local_device_id,
    peer_transfer_t transfer_type, ze_command_list_handle_t local_command_list,
    ze_command_list_handle_t remote_command_list, size_t buffer_size,
    ze_event_handle_t local_timestamp_event,
    ze_event_handle_t remote_timestamp_event) {
  if (transfer_type == PEER_WRITE) {
    append_copy(local_command_list, local_device_id,
                ze_dst_buffers[remote_device_id],
                ze_src_buffers[local_device_id], buffer_size,
                local_timestamp_event, 1, &event);
    append_copy(remote_command_list, remote_device_id,
                ze_dst_buffers[local_device_id],
                ze_src_buffers[remote_device_id], buffer_size,
                remote_timestamp_event, 1, &event);
  } else {
    append_copy(local_command_list, local_device_id,
                ze_dst_buffers[local_device_id],
                ze_src_buffers[remote_device_id], buffer_size,
                local_timestamp_event, 1, &event);
    append_copy(remote_command_list, remote_device_id,
                ze_dst_buffers[remote_device_id],
                ze_src_buffers[local_device_id], buffer_size,
                remote_timestamp_event, 1, &event);
  }
}

void ZePeer::bidirectional_print_results(
    uint32_t remote_device_id, uint32_t local_device_id, peer_test_t test_type,
    peer_transfer_t transfer_type, size_t buffer_size, long double host_time_us,
    long double local_time_us, long double remote_time_us) {
  if (!use_event_timer) {
    print_results(true, test_type, buffer_size, host_time_us);
    return;
  }

  // Labels follow the data: a read copy moves data towards its submitter.
  auto label = [](uint32_t from, uint32_t to) {
    return "Device( " + std::to_string(from) + " )->Device( " +
           std::to_string(to) + " )";
  };
  const bool write = transfer_type == PEER_WRITE;
  const std::string local_label =
      write ? label(local_device_id, remote_device_id)
            : label(remote_device_id, local_device_id);
  const std::string remote_label =
      write ? label(remote_device_id, local_device_id)
            : label(local_device_id, remote_device_id);

  const long double iterations = static_cast<long double>(number_iterations);
  const long double local_us = local_time_us / iterations;
  const long double remote_us = remote_time_us / iterations;
  if (test_type == PEER_BANDWIDTH) {
    const long double local_bw = bandwidth_gbps(buffer_size, local_us);
    const long double remote_bw = bandwidth_gbps(buffer_size, remote_us);
    // Assumes that the copies released by the gate event overlap, so the
    // slower one ends the combined transfer.
    print_value(test_type, buffer_size,
                2 * bandwidth_gbps(buffer_size, std::max(local_us, remote_us)),
                "");
    print_value(test_type, buffer_size, local_bw, local_label);
    print_value(test_type, buffer_size, remote_bw, remote_label);
  } else {
    print_value(test_type, buffer_size, local_us, local_label);
    print_value(test_type, buffer_size, remote_us, remote_label);
  }
}

void ZePeer::bidirectional_perform_copy(
    uint32_t remote_device_id, uint32_t local_device_id, uint32_t queue_index,
    peer_test_t test_type, peer_transfer_t transfer_type, size_t buffer_size) {

  ze_command_list_handle_t local_command_list =
      ze_peer_devices[local_device_id].engines[queue_index].second;
  ze_command_queue_handle_t local_command_queue =
      ze_peer_devices[local_device_id].engines[queue_index].first;
  ze_command_list_handle_t remote_command_list =
      ze_peer_devices[remote_device_id].engines[queue_index].second;
  ze_command_queue_handle_t remote_command_queue =
      ze_peer_devices[remote_device_id].engines[queue_index].first;

  ze_event_pool_handle_t local_timestamp_pool = nullptr;
  ze_event_handle_t local_timestamp_event = nullptr;
  ze_event_pool_handle_t remote_timestamp_pool = nullptr;
  ze_event_handle_t remote_timestamp_event = nullptr;
  if (use_event_timer) {
    create_timestamp_event(local_device_id, local_timestamp_pool,
                           local_timestamp_event);
    create_timestamp_event(remote_device_id, remote_timestamp_pool,
                           remote_timestamp_event);
  }

  bidirectional_append_copies(remote_device_id, local_device_id, transfer_type,
                              local_command_list, remote_command_list,
                              buffer_size, local_timestamp_event,
                              remote_timestamp_event);
  SUCCESS_OR_TERMINATE(zeCommandListClose(local_command_list));
  SUCCESS_OR_TERMINATE(zeCommandListClose(remote_command_list));

  auto reset_timestamp_events = [&]() {
    if (use_event_timer) {
      SUCCESS_OR_TERMINATE(zeEventHostReset(local_timestamp_event));
      SUCCESS_OR_TERMINATE(zeEventHostReset(remote_timestamp_event));
    }
  };

  Timer<std::chrono::microseconds::period> timer;

  /* Warm up */
  for (uint32_t i = 0U; i < warm_up_iterations; i++) {
    SUCCESS_OR_TERMINATE(zeEventHostSignal(event));
    SUCCESS_OR_TERMINATE(zeCommandQueueExecuteCommandLists(
        local_command_queue, 1, &local_command_list, nullptr));
    SUCCESS_OR_TERMINATE(zeCommandQueueExecuteCommandLists(
        remote_command_queue, 1, &remote_command_list, nullptr));
    SUCCESS_OR_TERMINATE(zeCommandQueueSynchronize(
        local_command_queue, std::numeric_limits<uint64_t>::max()));
    SUCCESS_OR_TERMINATE(zeCommandQueueSynchronize(
        remote_command_queue, std::numeric_limits<uint64_t>::max()));
    SUCCESS_OR_TERMINATE(zeEventHostReset(event));
    reset_timestamp_events();
  }

  do {
    long double time_usec = 0;
    long double local_time_usec = 0;
    long double remote_time_usec = 0;
    for (uint32_t i = 0U; i < number_iterations; i++) {
      SUCCESS_OR_TERMINATE(zeCommandQueueExecuteCommandLists(
          local_command_queue, 1, &local_command_list, nullptr));
      SUCCESS_OR_TERMINATE(zeCommandQueueExecuteCommandLists(
          remote_command_queue, 1, &remote_command_list, nullptr));

      timer.start();
      SUCCESS_OR_TERMINATE(zeEventHostSignal(event));
      SUCCESS_OR_TERMINATE(zeCommandQueueSynchronize(
          local_command_queue, std::numeric_limits<uint64_t>::max()));
      SUCCESS_OR_TERMINATE(zeCommandQueueSynchronize(
          remote_command_queue, std::numeric_limits<uint64_t>::max()));
      timer.end();
      time_usec += timer.period_minus_overhead();

      if (use_event_timer) {
        local_time_usec +=
            timestamp_event_time_us(local_timestamp_event, local_device_id);
        remote_time_usec +=
            timestamp_event_time_us(remote_timestamp_event, remote_device_id);
      }
      SUCCESS_OR_TERMINATE(zeEventHostReset(event));
      reset_timestamp_events();
    }
    bidirectional_print_results(remote_device_id, local_device_id, test_type,
                                transfer_type, buffer_size, time_usec,
                                local_time_usec, remote_time_usec);
  } while (run_continuously);

  SUCCESS_OR_TERMINATE(zeCommandListReset(local_command_list));
  SUCCESS_OR_TERMINATE(zeCommandListReset(remote_command_list));

  if (use_event_timer) {
    SUCCESS_OR_TERMINATE(zeEventDestroy(local_timestamp_event));
    SUCCESS_OR_TERMINATE(zeEventPoolDestroy(local_timestamp_pool));
    SUCCESS_OR_TERMINATE(zeEventDestroy(remote_timestamp_event));
    SUCCESS_OR_TERMINATE(zeEventPoolDestroy(remote_timestamp_pool));
  }
}

void ZePeer::bidirectional_perform_copy_immediate(
    uint32_t remote_device_id, uint32_t local_device_id, uint32_t queue_index,
    peer_test_t test_type, peer_transfer_t transfer_type, size_t buffer_size) {
  ze_command_list_handle_t local_command_list =
      ze_peer_devices[local_device_id].engines[queue_index].second;
  ze_command_list_handle_t remote_command_list =
      ze_peer_devices[remote_device_id].engines[queue_index].second;

  ze_event_pool_handle_t local_timestamp_pool = nullptr;
  ze_event_handle_t local_timestamp_event = nullptr;
  ze_event_pool_handle_t remote_timestamp_pool = nullptr;
  ze_event_handle_t remote_timestamp_event = nullptr;
  if (use_event_timer) {
    create_timestamp_event(local_device_id, local_timestamp_pool,
                           local_timestamp_event);
    create_timestamp_event(remote_device_id, remote_timestamp_pool,
                           remote_timestamp_event);
  }

  auto reset_timestamp_events = [&]() {
    if (use_event_timer) {
      SUCCESS_OR_TERMINATE(zeEventHostReset(local_timestamp_event));
      SUCCESS_OR_TERMINATE(zeEventHostReset(remote_timestamp_event));
    }
  };

  Timer<std::chrono::microseconds::period> timer;

  /* Warm up */
  for (uint32_t i = 0U; i < warm_up_iterations; i++) {
    bidirectional_append_copies(remote_device_id, local_device_id,
                                transfer_type, local_command_list,
                                remote_command_list, buffer_size,
                                local_timestamp_event, remote_timestamp_event);
    SUCCESS_OR_TERMINATE(zeEventHostSignal(event));
    SUCCESS_OR_TERMINATE(zeCommandListHostSynchronize(
        local_command_list, std::numeric_limits<uint64_t>::max()));
    SUCCESS_OR_TERMINATE(zeCommandListHostSynchronize(
        remote_command_list, std::numeric_limits<uint64_t>::max()));
    SUCCESS_OR_TERMINATE(zeEventHostReset(event));
    reset_timestamp_events();
  }

  do {
    long double time_usec = 0;
    long double local_time_usec = 0;
    long double remote_time_usec = 0;
    for (uint32_t i = 0U; i < number_iterations; i++) {
      bidirectional_append_copies(
          remote_device_id, local_device_id, transfer_type, local_command_list,
          remote_command_list, buffer_size, local_timestamp_event,
          remote_timestamp_event);

      timer.start();
      SUCCESS_OR_TERMINATE(zeEventHostSignal(event));
      SUCCESS_OR_TERMINATE(zeCommandListHostSynchronize(
          local_command_list, std::numeric_limits<uint64_t>::max()));
      SUCCESS_OR_TERMINATE(zeCommandListHostSynchronize(
          remote_command_list, std::numeric_limits<uint64_t>::max()));
      timer.end();
      time_usec += timer.period_minus_overhead();

      if (use_event_timer) {
        local_time_usec +=
            timestamp_event_time_us(local_timestamp_event, local_device_id);
        remote_time_usec +=
            timestamp_event_time_us(remote_timestamp_event, remote_device_id);
      }
      SUCCESS_OR_TERMINATE(zeEventHostReset(event));
      reset_timestamp_events();
    }
    bidirectional_print_results(remote_device_id, local_device_id, test_type,
                                transfer_type, buffer_size, time_usec,
                                local_time_usec, remote_time_usec);
  } while (run_continuously);

  if (use_event_timer) {
    SUCCESS_OR_TERMINATE(zeEventDestroy(local_timestamp_event));
    SUCCESS_OR_TERMINATE(zeEventPoolDestroy(local_timestamp_pool));
    SUCCESS_OR_TERMINATE(zeEventDestroy(remote_timestamp_event));
    SUCCESS_OR_TERMINATE(zeEventPoolDestroy(remote_timestamp_pool));
  }
}

void ZePeer::bidirectional_bandwidth_latency(peer_test_t test_type,
                                             peer_transfer_t transfer_type,
                                             size_t number_buffer_elements,
                                             uint32_t remote_device_id,
                                             uint32_t local_device_id,
                                             uint32_t queue_index) {

  size_t buffer_size = 0;

  set_up_kernel_copy(local_device_id, queue_index, number_buffer_elements);
  set_up_kernel_copy(remote_device_id, queue_index, number_buffer_elements);

  std::vector<uint32_t> remote_device_ids = {remote_device_id};
  std::vector<uint32_t> local_device_ids = {local_device_id};

  set_up(number_buffer_elements, remote_device_ids, local_device_ids,
         buffer_size);

  initialize_buffers(remote_device_ids, local_device_ids, ze_host_buffer,
                     buffer_size);

  if (ZePeer::use_immediate_cmdlist) {
    bidirectional_perform_copy_immediate(remote_device_id, local_device_id,
                                         queue_index, test_type, transfer_type,
                                         buffer_size);
  } else {
    bidirectional_perform_copy(remote_device_id, local_device_id, queue_index,
                               test_type, transfer_type, buffer_size);
  }

  if (validate_results) {
    if (ZePeer::use_immediate_cmdlist) {
      validate_buffer_immediate(
          ze_peer_devices[remote_device_id].engines[0].second,
          ze_host_validate_buffer, ze_dst_buffers[remote_device_id],
          ze_host_buffer, buffer_size);
    } else {
      validate_buffer(ze_peer_devices[remote_device_id].engines[0].second,
                      ze_peer_devices[remote_device_id].engines[0].first,
                      ze_host_validate_buffer, ze_dst_buffers[remote_device_id],
                      ze_host_buffer, buffer_size);
    }
    if (ZePeer::use_immediate_cmdlist) {
      validate_buffer_immediate(
          ze_peer_devices[local_device_id].engines[0].second,
          ze_host_validate_buffer, ze_dst_buffers[local_device_id],
          ze_host_buffer, buffer_size);
    } else {
      validate_buffer(ze_peer_devices[local_device_id].engines[0].second,
                      ze_peer_devices[local_device_id].engines[0].first,
                      ze_host_validate_buffer, ze_dst_buffers[local_device_id],
                      ze_host_buffer, buffer_size);
    }
  }

  tear_down(remote_device_ids, local_device_ids);
}