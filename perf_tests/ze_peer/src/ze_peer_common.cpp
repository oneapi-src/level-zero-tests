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

    ze_device_properties_t device_properties = {
        ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES, nullptr};
    SUCCESS_OR_TERMINATE(zeDeviceGetProperties(device, &device_properties));

    uint32_t numQueueGroups = 0;
    benchmark->deviceGetCommandQueueGroupProperties(device_index,
                                                    &numQueueGroups, nullptr);

    if (numQueueGroups == 0) {
      std::cout << " No queue groups found\n" << std::endl;
      continue;
    }

    std::vector<ze_command_queue_group_properties_t> queueProperties(
        numQueueGroups);
    for (auto &prop : queueProperties) {
      prop = {ZE_STRUCTURE_TYPE_COMMAND_QUEUE_GROUP_PROPERTIES, nullptr};
    }
    benchmark->deviceGetCommandQueueGroupProperties(
        device_index, &numQueueGroups, queueProperties.data());

    uint32_t q = 0;
    std::cout << "====================================================\n";
    std::cout << " Device " << device_index << "\n";
    std::cout << " \t" << device_properties.name << "\n";
    std::cout << " \t" << device_properties.coreClockRate << " MHz\n";
    std::cout << "----------------------------------------------------\n";
    std::cout << "| Group  | Number of Queues | Compute | Copy |  -u |\n";
    std::cout << "====================================================\n";
    for (uint32_t i = 0; i < numQueueGroups; i++) {
      bool isCompute = false;
      bool isCopy = false;
      if (queueProperties[i].flags &
          ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE) {
        isCompute = true;
      }
      if ((queueProperties[i].flags &
           ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COPY)) {
        isCopy = true;
      }
      std::cout << "|   " << std::fixed << std::setw(1) << i << "    |        "
                << std::fixed << std::setw(2) << queueProperties[i].numQueues
                << "        |    " << (isCompute ? "X" : " ") << "    |   "
                << (isCopy ? "X" : " ") << "  |     |\n";
      for (uint32_t j = 0; j < queueProperties[i].numQueues; j++) {
        std::cout << "|        |                  |         |      |  "
                  << std::fixed << std::setw(2) << q++ << " |\n";
      }
      std::cout << "----------------------------------------------------\n";
    }
    std::cout << "====================================================\n";
  }
}

void ZePeer::print_results(bool bidirectional, peer_test_t test_type,
                           size_t buffer_size,
                           Timer<std::chrono::microseconds::period> &timer) {
  long double total_time_usec = timer.period_minus_overhead();
  print_results(bidirectional, test_type, buffer_size, total_time_usec);
}

void ZePeer::print_results(bool bidirectional, peer_test_t test_type,
                           size_t buffer_size, long double total_time_us) {
  long double total_time_usec =
      total_time_us / static_cast<long double>(number_iterations);

  long double total_time_s = total_time_usec / 1e6;

  long double total_data_transfer =
      buffer_size / static_cast<long double>(1e9); /* Units in Gigabytes */
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

  if (test_type == PEER_BANDWIDTH) {
    std::cout << "BW [GBPS]: " << std::fixed << std::setw(4)
              << buffer_size_formatted << buffer_format_str << ": "
              << std::fixed << std::setw(8) << std::setprecision(2)
              << total_bandwidth << std::endl;
  } else {
    std::cout << "Latency [us]: " << std::fixed << std::setw(4)
              << buffer_size_formatted << buffer_format_str << ": "
              << std::fixed << std::setw(8) << std::setprecision(2)
              << total_time_usec << std::endl;
  }
}

void ZePeer::set_up(int number_buffer_elements,
                    std::vector<uint32_t> &remote_device_ids,
                    std::vector<uint32_t> &local_device_ids,
                    size_t &buffer_size) {

  size_t element_size = sizeof(char);
  buffer_size = element_size * number_buffer_elements;

  for (auto local_device_id : local_device_ids) {
    void *ze_buffer = nullptr;
    benchmark->memoryAlloc(local_device_id, buffer_size, &ze_buffer);
    ze_src_buffers[local_device_id] = ze_buffer;

    ze_buffer = nullptr;
    benchmark->memoryAlloc(local_device_id, buffer_size, &ze_buffer);
    ze_dst_buffers[local_device_id] = ze_buffer;
  }

  for (auto remote_device_id : remote_device_ids) {
    void *ze_buffer = nullptr;
    benchmark->memoryAlloc(remote_device_id, buffer_size, &ze_buffer);
    ze_src_buffers[remote_device_id] = ze_buffer;

    ze_buffer = nullptr;
    benchmark->memoryAlloc(remote_device_id, buffer_size, &ze_buffer);
    ze_dst_buffers[remote_device_id] = ze_buffer;
  }

  void **host_buffer = reinterpret_cast<void **>(&ze_host_buffer);
  benchmark->memoryAllocHost(buffer_size, host_buffer);
  void **host_validate_buffer =
      reinterpret_cast<void **>(&ze_host_validate_buffer);
  benchmark->memoryAllocHost(buffer_size, host_validate_buffer);
}

void ZePeer::tear_down(std::vector<uint32_t> &remote_device_ids,
                       std::vector<uint32_t> &local_device_ids) {

  for (auto local_device_id : local_device_ids) {
    if (ze_dst_buffers[local_device_id]) {
      benchmark->memoryFree(ze_dst_buffers[local_device_id]);
      ze_dst_buffers[local_device_id] = nullptr;
    }
    if (ze_src_buffers[local_device_id]) {
      benchmark->memoryFree(ze_src_buffers[local_device_id]);
      ze_src_buffers[local_device_id] = nullptr;
    }
  }

  for (auto remote_device_id : remote_device_ids) {
    if (ze_dst_buffers[remote_device_id]) {
      benchmark->memoryFree(ze_dst_buffers[remote_device_id]);
      ze_dst_buffers[remote_device_id] = nullptr;
    }
    if (ze_src_buffers[remote_device_id]) {
      benchmark->memoryFree(ze_src_buffers[remote_device_id]);
      ze_src_buffers[remote_device_id] = nullptr;
    }
  }

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

void ZePeer::initialize_buffers(std::vector<uint32_t> &remote_device_ids,
                                std::vector<uint32_t> &local_device_ids,
                                char *host_buffer, size_t buffer_size) {
  // fibonacci
  host_buffer[0] = 1;
  host_buffer[1] = 2;
  for (size_t k = 2; k < buffer_size; k++) {
    host_buffer[k] = host_buffer[k - 2] + host_buffer[k - 1];
  }

  for (auto remote_device_id : remote_device_ids) {
    ze_command_list_handle_t command_list =
        ze_peer_devices[remote_device_id].engines[0].second;
    ze_command_queue_handle_t command_queue =
        ze_peer_devices[remote_device_id].engines[0].first;
    void *src_buffer = ze_src_buffers[remote_device_id];
    initialize_src_buffer(command_list, command_queue, src_buffer, host_buffer,
                          buffer_size);
  }

  for (auto local_device_id : local_device_ids) {
    ze_command_list_handle_t command_list =
        ze_peer_devices[local_device_id].engines[0].second;
    ze_command_queue_handle_t command_queue =
        ze_peer_devices[local_device_id].engines[0].first;
    void *src_buffer = ze_src_buffers[local_device_id];
    initialize_src_buffer(command_list, command_queue, src_buffer, host_buffer,
                          buffer_size);
  }
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
