/*
 *
 * Copyright (C) 2019-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */
#include "ze_peer.h"

void ZePeer::create_timestamp_event(uint32_t device_id,
                                    ze_event_pool_handle_t &timestamp_pool,
                                    ze_event_handle_t &timestamp_event) {
  ze_event_pool_desc_t pool_desc = {ZE_STRUCTURE_TYPE_EVENT_POOL_DESC};
  pool_desc.flags =
      ZE_EVENT_POOL_FLAG_HOST_VISIBLE | ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP;
  pool_desc.count = 1;
  SUCCESS_OR_TERMINATE(
      zeEventPoolCreate(benchmark->deviceContext(device_id), &pool_desc, 1,
                        &benchmark->_devices[device_id], &timestamp_pool));

  ze_event_desc_t event_desc = {ZE_STRUCTURE_TYPE_EVENT_DESC};
  event_desc.signal = ZE_EVENT_SCOPE_FLAG_HOST;
  event_desc.wait = ZE_EVENT_SCOPE_FLAG_HOST;
  SUCCESS_OR_TERMINATE(
      zeEventCreate(timestamp_pool, &event_desc, &timestamp_event));
}

// timerResolution is in nanoseconds per tick for
// ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES.
long double ZePeer::timestamp_event_time_us(ze_event_handle_t timestamp_event,
                                            uint32_t device_id) {
  ze_device_properties_t properties = {ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES};
  SUCCESS_OR_TERMINATE(
      zeDeviceGetProperties(benchmark->_devices[device_id], &properties));

  ze_kernel_timestamp_result_t timestamp = {};
  SUCCESS_OR_TERMINATE(
      zeEventQueryKernelTimestamp(timestamp_event, &timestamp));
  const uint64_t start = timestamp.global.kernelStart;
  const uint64_t end = timestamp.global.kernelEnd;
  const uint64_t max_value =
      (properties.kernelTimestampValidBits >= 64)
          ? ~0ULL
          : ~(~0ULL << properties.kernelTimestampValidBits);
  const uint64_t ticks =
      (end >= start) ? (end - start) : ((max_value - start) + end + 1);

  return static_cast<long double>(ticks) *
         static_cast<long double>(properties.timerResolution) / 1e3L;
}

void ZePeer::perform_copy(peer_test_t test_type,
                          ze_command_list_handle_t command_list,
                          ze_command_queue_handle_t command_queue,
                          void *dst_buffer, void *src_buffer,
                          size_t buffer_size, uint32_t device_id) {
  ze_event_pool_handle_t timestamp_pool = nullptr;
  ze_event_handle_t timestamp_event = nullptr;
  if (use_event_timer) {
    create_timestamp_event(device_id, timestamp_pool, timestamp_event);
  }

  append_copy(command_list, device_id, dst_buffer, src_buffer, buffer_size,
              timestamp_event, 0, nullptr);
  SUCCESS_OR_TERMINATE(zeCommandListClose(command_list));

  Timer<std::chrono::microseconds::period> timer;

  /* Warm up */
  for (uint32_t i = 0U; i < warm_up_iterations; i++) {
    SUCCESS_OR_TERMINATE(zeCommandQueueExecuteCommandLists(
        command_queue, 1, &command_list, nullptr));
    SUCCESS_OR_TERMINATE(zeCommandQueueSynchronize(
        command_queue, std::numeric_limits<uint64_t>::max()));
    if (timestamp_event) {
      SUCCESS_OR_TERMINATE(zeEventHostReset(timestamp_event));
    }
  }

  do {
    long double device_time_us = 0;
    timer.start();
    for (uint32_t i = 0U; i < number_iterations; i++) {
      SUCCESS_OR_TERMINATE(zeCommandQueueExecuteCommandLists(
          command_queue, 1, &command_list, nullptr));
      SUCCESS_OR_TERMINATE(zeCommandQueueSynchronize(
          command_queue, std::numeric_limits<uint64_t>::max()));
      if (timestamp_event) {
        device_time_us += timestamp_event_time_us(timestamp_event, device_id);
        SUCCESS_OR_TERMINATE(zeEventHostReset(timestamp_event));
      }
    }
    timer.end();

    if (timestamp_event) {
      print_results(false, test_type, buffer_size, device_time_us);
    } else {
      print_results(false, test_type, buffer_size, timer);
    }
  } while (run_continuously);

  SUCCESS_OR_TERMINATE(zeCommandListReset(command_list));

  if (timestamp_event) {
    SUCCESS_OR_TERMINATE(zeEventDestroy(timestamp_event));
    SUCCESS_OR_TERMINATE(zeEventPoolDestroy(timestamp_pool));
  }
}

void ZePeer::perform_copy_immediate(peer_test_t test_type,
                                    ze_command_list_handle_t command_list,
                                    void *dst_buffer, void *src_buffer,
                                    size_t buffer_size, uint32_t device_id) {
  ze_event_pool_handle_t timestamp_pool = nullptr;
  ze_event_handle_t timestamp_event = nullptr;
  if (use_event_timer) {
    create_timestamp_event(device_id, timestamp_pool, timestamp_event);
  }

  Timer<std::chrono::microseconds::period> timer;

  /* Warm up */
  for (uint32_t i = 0U; i < warm_up_iterations; i++) {
    append_copy(command_list, device_id, dst_buffer, src_buffer, buffer_size,
                timestamp_event, 0, nullptr);
    SUCCESS_OR_TERMINATE(zeCommandListHostSynchronize(
        command_list, std::numeric_limits<uint64_t>::max()));
    if (timestamp_event) {
      SUCCESS_OR_TERMINATE(zeEventHostReset(timestamp_event));
    }
  }

  do {
    long double device_time_us = 0;
    timer.start();
    for (uint32_t i = 0U; i < number_iterations; i++) {
      append_copy(command_list, device_id, dst_buffer, src_buffer, buffer_size,
                  timestamp_event, 0, nullptr);
      SUCCESS_OR_TERMINATE(zeCommandListHostSynchronize(
          command_list, std::numeric_limits<uint64_t>::max()));
      if (timestamp_event) {
        device_time_us += timestamp_event_time_us(timestamp_event, device_id);
        SUCCESS_OR_TERMINATE(zeEventHostReset(timestamp_event));
      }
    }
    timer.end();

    if (timestamp_event) {
      print_results(false, test_type, buffer_size, device_time_us);
    } else {
      print_results(false, test_type, buffer_size, timer);
    }
  } while (run_continuously);

  SUCCESS_OR_TERMINATE(zeCommandListReset(command_list));

  if (timestamp_event) {
    SUCCESS_OR_TERMINATE(zeEventDestroy(timestamp_event));
    SUCCESS_OR_TERMINATE(zeEventPoolDestroy(timestamp_pool));
  }
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

  set_up_kernel_copy(local_device_id, queue_index, number_buffer_elements);

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

  if (ZePeer::use_immediate_cmdlist) {
    perform_copy_immediate(test_type, command_list, dst_buffer, src_buffer,
                           buffer_size, local_device_id);
  } else {
    perform_copy(test_type, command_list, command_queue, dst_buffer, src_buffer,
                 buffer_size, local_device_id);
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
