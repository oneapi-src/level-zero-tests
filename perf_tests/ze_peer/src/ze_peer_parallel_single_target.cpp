/*
 *
 * Copyright (C) 2019-2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */
#include "ze_peer.h"

void ZePeer::perform_bidirectional_parallel_copy_to_single_target(
    peer_test_t test_type, peer_transfer_t transfer_type,
    uint32_t remote_device_id, uint32_t local_device_id, size_t buffer_size) {

  size_t num_engines = queues.size();
  size_t chunk = buffer_size / num_engines;

  for (size_t e = 0; e < num_engines; e++) {
    if (transfer_type == PEER_WRITE) {
      SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
          ze_peer_devices[local_device_id].engines[queues[e]].second,
          reinterpret_cast<void *>(
              reinterpret_cast<uint64_t>(ze_dst_buffers[remote_device_id]) +
              e * chunk),
          reinterpret_cast<void *>(
              reinterpret_cast<uint64_t>(ze_src_buffers[local_device_id]) +
              e * chunk),
          chunk, nullptr, 1, &event));
      SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
          ze_peer_devices[remote_device_id].engines[queues[e]].second,
          reinterpret_cast<void *>(
              reinterpret_cast<uint64_t>(ze_dst_buffers[local_device_id]) +
              e * chunk),
          reinterpret_cast<void *>(
              reinterpret_cast<uint64_t>(ze_src_buffers[remote_device_id]) +
              e * chunk),
          chunk, nullptr, 1, &event));
    } else {
      SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
          ze_peer_devices[local_device_id].engines[queues[e]].second,
          reinterpret_cast<void *>(
              reinterpret_cast<uint64_t>(ze_dst_buffers[local_device_id]) +
              e * chunk),
          reinterpret_cast<void *>(
              reinterpret_cast<uint64_t>(ze_src_buffers[remote_device_id]) +
              e * chunk),
          chunk, nullptr, 1, &event));
      SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
          ze_peer_devices[remote_device_id].engines[queues[e]].second,
          reinterpret_cast<void *>(
              reinterpret_cast<uint64_t>(ze_dst_buffers[remote_device_id]) +
              e * chunk),
          reinterpret_cast<void *>(
              reinterpret_cast<uint64_t>(ze_src_buffers[local_device_id]) +
              e * chunk),
          chunk, nullptr, 1, &event));
    }

    SUCCESS_OR_TERMINATE(zeCommandListClose(
        ze_peer_devices[local_device_id].engines[queues[e]].second));
    SUCCESS_OR_TERMINATE(zeCommandListClose(
        ze_peer_devices[remote_device_id].engines[queues[e]].second));
  }

  Timer<std::chrono::microseconds::period> timer;

  /* Warm up */
  for (uint32_t i = 0U; i < warm_up_iterations; i++) {
    SUCCESS_OR_TERMINATE(zeEventHostSignal(event));
    for (size_t e = 0; e < num_engines; e++) {
      SUCCESS_OR_TERMINATE(zeCommandQueueExecuteCommandLists(
          ze_peer_devices[local_device_id].engines[queues[e]].first, 1,
          &ze_peer_devices[local_device_id].engines[queues[e]].second,
          nullptr));
      SUCCESS_OR_TERMINATE(zeCommandQueueExecuteCommandLists(
          ze_peer_devices[remote_device_id].engines[queues[e]].first, 1,
          &ze_peer_devices[remote_device_id].engines[queues[e]].second,
          nullptr));

      SUCCESS_OR_TERMINATE(zeCommandQueueSynchronize(
          ze_peer_devices[local_device_id].engines[queues[e]].first,
          std::numeric_limits<uint64_t>::max()));
      SUCCESS_OR_TERMINATE(zeCommandQueueSynchronize(
          ze_peer_devices[remote_device_id].engines[queues[e]].first,
          std::numeric_limits<uint64_t>::max()));
    }
    SUCCESS_OR_TERMINATE(zeEventHostReset(event));
  }

  do {
    timer.start();
    for (uint32_t i = 0U; i < number_iterations; i++) {
      for (size_t e = 0; e < num_engines; e++) {
        SUCCESS_OR_TERMINATE(zeCommandQueueExecuteCommandLists(
            ze_peer_devices[local_device_id].engines[queues[e]].first, 1,
            &ze_peer_devices[local_device_id].engines[queues[e]].second,
            nullptr));
        SUCCESS_OR_TERMINATE(zeCommandQueueExecuteCommandLists(
            ze_peer_devices[remote_device_id].engines[queues[e]].first, 1,
            &ze_peer_devices[remote_device_id].engines[queues[e]].second,
            nullptr));
      }

      SUCCESS_OR_TERMINATE(zeEventHostSignal(event));

      for (size_t e = 0; e < num_engines; e++) {
        SUCCESS_OR_TERMINATE(zeCommandQueueSynchronize(
            ze_peer_devices[local_device_id].engines[queues[e]].first,
            std::numeric_limits<uint64_t>::max()));
        SUCCESS_OR_TERMINATE(zeCommandQueueSynchronize(
            ze_peer_devices[remote_device_id].engines[queues[e]].first,
            std::numeric_limits<uint64_t>::max()));
      }
    }
    timer.end();

    print_results(true, test_type, buffer_size, timer);
  } while (run_continuously);

  for (size_t e = 0; e < num_engines; e++) {
    SUCCESS_OR_TERMINATE(zeCommandListReset(
        ze_peer_devices[local_device_id].engines[queues[e]].second));
    SUCCESS_OR_TERMINATE(zeCommandListReset(
        ze_peer_devices[remote_device_id].engines[queues[e]].second));
  }
}

void ZePeer::perform_parallel_copy_to_single_target(
    peer_test_t test_type, peer_transfer_t transfer_type,
    uint32_t remote_device_id, uint32_t local_device_id, size_t buffer_size) {

  uint32_t queue_device_id = local_device_id;
  if (use_queue_in_destination) {
    queue_device_id = remote_device_id;
  }

  void *dst_buffer = ze_dst_buffers[remote_device_id];
  void *src_buffer = ze_src_buffers[local_device_id];
  if (transfer_type == PEER_READ) {
    dst_buffer = ze_dst_buffers[local_device_id];
    src_buffer = ze_src_buffers[remote_device_id];
  }

  size_t num_engines = queues.size();
  size_t chunk = buffer_size / num_engines;
  for (size_t e = 0; e < num_engines; e++) {
    SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
        ze_peer_devices[queue_device_id].engines[queues[e]].second,
        reinterpret_cast<void *>(reinterpret_cast<uint64_t>(dst_buffer) +
                                 e * chunk),
        reinterpret_cast<void *>(reinterpret_cast<uint64_t>(src_buffer) +
                                 e * chunk),
        chunk, nullptr, 1, &event));
    SUCCESS_OR_TERMINATE(zeCommandListClose(
        ze_peer_devices[queue_device_id].engines[queues[e]].second));
  }

  Timer<std::chrono::microseconds::period> timer;

  /* Warm up */
  for (uint32_t i = 0U; i < warm_up_iterations; i++) {
    for (size_t e = 0; e < num_engines; e++) {
      SUCCESS_OR_TERMINATE(zeCommandQueueExecuteCommandLists(
          ze_peer_devices[queue_device_id].engines[queues[e]].first, 1,
          &ze_peer_devices[queue_device_id].engines[queues[e]].second,
          nullptr));
    }

    SUCCESS_OR_TERMINATE(zeEventHostSignal(event));

    for (size_t e = 0; e < num_engines; e++) {
      SUCCESS_OR_TERMINATE(zeCommandQueueSynchronize(
          ze_peer_devices[queue_device_id].engines[queues[e]].first,
          std::numeric_limits<uint64_t>::max()));
    }

    SUCCESS_OR_TERMINATE(zeEventHostReset(event));
  }

  do {
    long double time_usec = 0;
    for (uint32_t i = 0U; i < number_iterations; i++) {
      for (size_t e = 0; e < num_engines; e++) {
        SUCCESS_OR_TERMINATE(zeCommandQueueExecuteCommandLists(
            ze_peer_devices[queue_device_id].engines[queues[e]].first, 1,
            &ze_peer_devices[queue_device_id].engines[queues[e]].second,
            nullptr));
      }

      timer.start();
      SUCCESS_OR_TERMINATE(zeEventHostSignal(event));

      for (size_t e = 0; e < num_engines; e++) {
        SUCCESS_OR_TERMINATE(zeCommandQueueSynchronize(
            ze_peer_devices[queue_device_id].engines[queues[e]].first,
            std::numeric_limits<uint64_t>::max()));
      }
      timer.end();
      time_usec += timer.period_minus_overhead();

      SUCCESS_OR_TERMINATE(zeEventHostReset(event));
    }

    print_results(false, test_type, buffer_size, time_usec);
  } while (run_continuously);

  for (size_t e = 0; e < num_engines; e++) {
    SUCCESS_OR_TERMINATE(zeCommandListReset(
        ze_peer_devices[queue_device_id].engines[queues[e]].second));
  }
}

void ZePeer::bandwidth_latency_parallel_to_single_target(
    peer_test_t test_type, peer_transfer_t transfer_type,
    int number_buffer_elements, uint32_t remote_device_id,
    uint32_t local_device_id) {

  size_t buffer_size = 0;
  std::vector<uint32_t> remote_device_ids = {remote_device_id};
  std::vector<uint32_t> local_device_ids = {local_device_id};
  set_up(number_buffer_elements, remote_device_ids, local_device_ids,
         buffer_size);

  initialize_buffers(remote_device_ids, local_device_ids, ze_host_buffer,
                     buffer_size);

  if (bidirectional) {
    perform_bidirectional_parallel_copy_to_single_target(
        test_type, transfer_type, remote_device_id, local_device_id,
        buffer_size);
  } else {
    perform_parallel_copy_to_single_target(test_type, transfer_type,
                                           remote_device_id, local_device_id,
                                           buffer_size);
  }

  if (validate_results) {
    ze_command_queue_handle_t command_queue =
        ze_peer_devices[local_device_id].engines[0].first;
    ze_command_list_handle_t command_list =
        ze_peer_devices[local_device_id].engines[0].second;

    void *dst_buffer = ze_dst_buffers[remote_device_id];
    if (transfer_type == PEER_READ) {
      dst_buffer = ze_dst_buffers[local_device_id];
    }

    validate_buffer(command_list, command_queue, ze_host_validate_buffer,
                    dst_buffer, ze_host_buffer, buffer_size);
  }

  tear_down(remote_device_ids, local_device_ids);
}
