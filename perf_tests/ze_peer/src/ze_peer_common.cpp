/*
 *
 * Copyright (C) 2019-2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "ze_peer.h"

void ZePeer::query_engines() {
  for (uint32_t device_index = 0;
       device_index < static_cast<uint32_t>(benchmark->_devices.size());
       device_index++) {
    auto device = benchmark->_devices[device_index];

    std::cout << "Device " << device_index << ":" << std::endl;

    uint32_t numQueueGroups = 0;
    benchmark->deviceGetCommandQueueGroupProperties(device_index,
                                                    &numQueueGroups, nullptr);

    if (numQueueGroups == 0) {
      std::cout << " No queue groups found\n" << std::endl;
      continue;
    }

    std::vector<ze_command_queue_group_properties_t> queueProperties(
        numQueueGroups);
    benchmark->deviceGetCommandQueueGroupProperties(
        device_index, &numQueueGroups, queueProperties.data());

    for (uint32_t i = 0; i < numQueueGroups; i++) {
      if (queueProperties[i].flags &
          ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE) {
        std::cout << " Group " << i
                  << " (compute): " << queueProperties[i].numQueues
                  << " queues\n";
      } else if ((queueProperties[i].flags &
                  ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE) == 0 &&
                 (queueProperties[i].flags &
                  ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COPY)) {
        std::cout << " Group " << i
                  << " (copy): " << queueProperties[i].numQueues << " queues\n";
      }
    }
    std::cout << std::endl;
  }
}

void ZePeer::print_results(bool bidirectional, peer_test_t test_type,
                           size_t buffer_size,
                           Timer<std::chrono::microseconds::period> &timer) {
  long double total_time_usec = timer.period_minus_overhead();
  if (test_type == PEER_LATENCY) {
    total_time_usec = timer.period_minus_overhead() /
                      static_cast<long double>(number_iterations);
  }
  long double total_time_s = total_time_usec / 1e6;

  long double total_data_transfer =
      (buffer_size * number_iterations) /
      static_cast<long double>(1e9); /* Units in Gigabytes */
  if (bidirectional) {
    total_data_transfer = 2 * total_data_transfer;
  }
  long double total_bandwidth = total_data_transfer / total_time_s;

  int buffer_size_formatted;
  std::string buffer_format_str;

  if (buffer_size >= (1024 * 1024)) {
    buffer_size_formatted =
        buffer_size / (1024 * 1024); /* Units in Megabytes */
    buffer_format_str = " MB";
  } else if (buffer_size >= 1024) {
    buffer_size_formatted = buffer_size / 1024; /* Units in Kilobytes */
    buffer_format_str = " KB";
  } else {
    buffer_size_formatted = buffer_size; /* Units in Bytes */
    buffer_format_str = "  B";
  }

  std::cout << std::fixed << std::setw(4) << buffer_size_formatted
            << buffer_format_str << ": " << std::fixed << std::setw(8)
            << std::setprecision(4);
  if (test_type == PEER_BANDWIDTH) {
    std::cout << total_bandwidth << std::endl;
  } else {
    std::cout << total_time_usec << std::endl;
  }
}

void ZePeer::set_up(int number_buffer_elements, uint32_t remote_device_id,
                    uint32_t local_device_id, size_t &buffer_size,
                    bool validate) {

  if (validate) {
    warm_up_iterations = 0;
    number_iterations = 1;
  }

  size_t element_size = sizeof(char);
  buffer_size = element_size * number_buffer_elements;

  void *ze_buffer = nullptr;
  benchmark->memoryAlloc(remote_device_id, buffer_size, &ze_buffer);
  ze_src_buffers[remote_device_id] = ze_buffer;

  ze_buffer = nullptr;
  benchmark->memoryAlloc(local_device_id, buffer_size, &ze_buffer);
  ze_src_buffers[local_device_id] = ze_buffer;

  ze_buffer = nullptr;
  benchmark->memoryAlloc(remote_device_id, buffer_size, &ze_buffer);
  ze_dst_buffers[remote_device_id] = ze_buffer;

  ze_buffer = nullptr;
  benchmark->memoryAlloc(local_device_id, buffer_size, &ze_buffer);
  ze_dst_buffers[local_device_id] = ze_buffer;

  void **host_buffer = reinterpret_cast<void **>(&ze_host_buffer);
  benchmark->memoryAllocHost(buffer_size, host_buffer);
  void **host_validate_buffer =
      reinterpret_cast<void **>(&ze_host_validate_buffer);
  benchmark->memoryAllocHost(buffer_size, host_validate_buffer);
}

void ZePeer::tear_down(uint32_t remote_device_id, uint32_t local_device_id) {

  benchmark->memoryFree(ze_dst_buffers[local_device_id]);
  benchmark->memoryFree(ze_dst_buffers[remote_device_id]);
  benchmark->memoryFree(ze_src_buffers[local_device_id]);
  benchmark->memoryFree(ze_src_buffers[remote_device_id]);

  benchmark->memoryFree(ze_host_buffer);
  benchmark->memoryFree(ze_host_validate_buffer);
}

void ZePeer::initialize_src_buffer(ze_command_list_handle_t command_list,
                                   ze_command_queue_handle_t command_queue,
                                   void *src_buffer, char *host_buffer,
                                   size_t buffer_size) {
  SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
      command_list, src_buffer, host_buffer, buffer_size, nullptr, 0, nullptr));
  SUCCESS_OR_TERMINATE(zeCommandListClose(command_list));
  SUCCESS_OR_TERMINATE(zeCommandQueueExecuteCommandLists(
      command_queue, 1, &command_list, nullptr));
  SUCCESS_OR_TERMINATE(zeCommandQueueSynchronize(
      command_queue, std::numeric_limits<uint64_t>::max()));
  SUCCESS_OR_TERMINATE(zeCommandListReset(command_list));
}

void ZePeer::initialize_buffers(ze_command_list_handle_t command_list,
                                ze_command_queue_handle_t command_queue,
                                void *src_buffer, char *host_buffer,
                                size_t buffer_size) {
  // fibonacci
  host_buffer[0] = 1;
  host_buffer[1] = 2;
  for (size_t k = 2; k < buffer_size; k++) {
    host_buffer[k] = host_buffer[k - 2] + host_buffer[k - 1];
    ze_host_validate_buffer[k] = 0;
  }

  if (src_buffer) {
    initialize_src_buffer(command_list, command_queue, src_buffer, host_buffer,
                          buffer_size);
  }
}

void ZePeer::initialize_buffers(uint32_t remote_device_id,
                                uint32_t local_device_id, char *host_buffer,
                                size_t buffer_size) {
  // fibonacci
  host_buffer[0] = 1;
  host_buffer[1] = 2;
  for (size_t k = 2; k < buffer_size; k++) {
    host_buffer[k] = host_buffer[k - 2] + host_buffer[k - 1];
  }

  ze_command_list_handle_t command_list =
      ze_peer_devices[remote_device_id].command_lists[0];
  ze_command_queue_handle_t command_queue =
      ze_peer_devices[remote_device_id].command_queues[0];
  void *src_buffer = ze_src_buffers[remote_device_id];
  initialize_src_buffer(command_list, command_queue, src_buffer, host_buffer,
                        buffer_size);

  command_list = ze_peer_devices[local_device_id].command_lists[0];
  command_queue = ze_peer_devices[local_device_id].command_queues[0];
  src_buffer = ze_src_buffers[local_device_id];
  initialize_src_buffer(command_list, command_queue, src_buffer, host_buffer,
                        buffer_size);
}

void ZePeer::validate_buffer(ze_command_list_handle_t command_list,
                             ze_command_queue_handle_t command_queue,
                             char *validate_buffer, void *dst_buffer,
                             char *host_buffer, size_t buffer_size) {
  SUCCESS_OR_TERMINATE(
      zeCommandListAppendMemoryCopy(command_list, validate_buffer, dst_buffer,
                                    buffer_size, nullptr, 0, nullptr));
  SUCCESS_OR_TERMINATE(zeCommandListClose(command_list));
  SUCCESS_OR_TERMINATE(zeCommandQueueExecuteCommandLists(
      command_queue, 1, &command_list, nullptr));
  SUCCESS_OR_TERMINATE(zeCommandQueueSynchronize(
      command_queue, std::numeric_limits<uint64_t>::max()));
  SUCCESS_OR_TERMINATE(zeCommandListReset(command_list));

  for (size_t i = 0; i < buffer_size; i++) {
    if (validate_buffer[i] != host_buffer[i]) {
      std::cout << "Error at " << i << ": validate_buffer "
                << static_cast<uint32_t>(validate_buffer[i])
                << " != host_buffer " << static_cast<uint32_t>(host_buffer[i])
                << "\n";
      break;
    }
  }
}

void ZePeer::perform_copy(peer_test_t test_type,
                          ze_command_list_handle_t command_list,
                          ze_command_queue_handle_t command_queue,
                          void *dst_buffer, void *src_buffer,
                          size_t buffer_size) {

  SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
      command_list, dst_buffer, src_buffer, buffer_size, nullptr, 0, nullptr));
  SUCCESS_OR_TERMINATE(zeCommandListClose(command_list));

  Timer<std::chrono::microseconds::period> timer;

  /* Warm up */
  for (int i = 0; i < warm_up_iterations; i++) {
    SUCCESS_OR_TERMINATE(zeCommandQueueExecuteCommandLists(
        command_queue, 1, &command_list, nullptr));
    SUCCESS_OR_TERMINATE(zeCommandQueueSynchronize(
        command_queue, std::numeric_limits<uint64_t>::max()));
  }

  timer.start();
  for (int i = 0; i < number_iterations; i++) {
    SUCCESS_OR_TERMINATE(zeCommandQueueExecuteCommandLists(
        command_queue, 1, &command_list, nullptr));
    SUCCESS_OR_TERMINATE(zeCommandQueueSynchronize(
        command_queue, std::numeric_limits<uint64_t>::max()));
  }
  timer.end();

  print_results(false, test_type, buffer_size, timer);

  SUCCESS_OR_TERMINATE(zeCommandListReset(command_list));
}

void ZePeer::perform_copy_all_engines(peer_test_t test_type,
                                      uint32_t local_device_id,
                                      void *dst_buffer, void *src_buffer,
                                      size_t buffer_size) {

  ze_event_pool_handle_t event_pool = {};
  ze_event_pool_desc_t event_pool_desc = {ZE_STRUCTURE_TYPE_EVENT_POOL_DESC};
  event_pool_desc.count = 1;
  event_pool_desc.flags = {};
  SUCCESS_OR_TERMINATE(
      zeEventPoolCreate(benchmark->context, &event_pool_desc, 1,
                        &benchmark->_devices[local_device_id], &event_pool));

  ze_event_handle_t event;
  ze_event_desc_t event_desc = {ZE_STRUCTURE_TYPE_EVENT_DESC};
  event_desc.signal = ZE_EVENT_SCOPE_FLAG_HOST;
  SUCCESS_OR_TERMINATE(zeEventCreate(event_pool, &event_desc, &event));

  size_t num_engines = ze_peer_devices[local_device_id].command_queues.size();
  size_t chunk = buffer_size / num_engines;
  for (size_t e = 0; e < num_engines; e++) {
    SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
        ze_peer_devices[local_device_id].command_lists[e],
        reinterpret_cast<void *>(reinterpret_cast<uint64_t>(dst_buffer) +
                                 e * chunk),
        reinterpret_cast<void *>(reinterpret_cast<uint64_t>(src_buffer) +
                                 e * chunk),
        chunk, nullptr, 1, &event));
    SUCCESS_OR_TERMINATE(
        zeCommandListClose(ze_peer_devices[local_device_id].command_lists[e]));
  }

  Timer<std::chrono::microseconds::period> timer;

  /* Warm up */
  for (int i = 0; i < warm_up_iterations; i++) {
    SUCCESS_OR_TERMINATE(zeEventHostReset(event));

    for (size_t e = 0; e < num_engines; e++) {
      SUCCESS_OR_TERMINATE(zeCommandQueueExecuteCommandLists(
          ze_peer_devices[local_device_id].command_queues[e], 1,
          &ze_peer_devices[local_device_id].command_lists[e], nullptr));
    }

    SUCCESS_OR_TERMINATE(zeEventHostSignal(event));

    for (size_t e = 0; e < num_engines; e++) {
      SUCCESS_OR_TERMINATE(zeCommandQueueSynchronize(
          ze_peer_devices[local_device_id].command_queues[e],
          std::numeric_limits<uint64_t>::max()));
    }
  }

  timer.start();
  for (int i = 0; i < number_iterations; i++) {
    SUCCESS_OR_TERMINATE(zeEventHostReset(event));

    for (size_t e = 0; e < num_engines; e++) {
      SUCCESS_OR_TERMINATE(zeCommandQueueExecuteCommandLists(
          ze_peer_devices[local_device_id].command_queues[e], 1,
          &ze_peer_devices[local_device_id].command_lists[e], nullptr));
    }

    SUCCESS_OR_TERMINATE(zeEventHostSignal(event));

    for (size_t e = 0; e < num_engines; e++) {
      SUCCESS_OR_TERMINATE(zeCommandQueueSynchronize(
          ze_peer_devices[local_device_id].command_queues[e],
          std::numeric_limits<uint64_t>::max()));
    }
  }
  timer.end();

  print_results(false, test_type, buffer_size, timer);

  for (size_t e = 0; e < num_engines; e++) {
    SUCCESS_OR_TERMINATE(
        zeCommandListReset(ze_peer_devices[local_device_id].command_lists[e]));
  }

  SUCCESS_OR_TERMINATE(zeEventDestroy(event));
  SUCCESS_OR_TERMINATE(zeEventPoolDestroy(event_pool));
}
