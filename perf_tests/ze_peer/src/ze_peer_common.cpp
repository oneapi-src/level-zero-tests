/*
 *
 * Copyright (C) 2019-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "ze_peer.h"

#include <array>

namespace {
// The source pattern x[k] = x[k - 2] + x[k - 1] (char), starting with 1, 2,
// repeats every 384 bytes.
const std::array<char, 384> &pattern_period() {
  static const std::array<char, 384> period = [] {
    std::array<char, 384> p{};
    p[0] = 1;
    p[1] = 2;
    for (size_t k = 2; k < p.size(); k++) {
      p[k] = static_cast<char>(p[k - 2] + p[k - 1]);
    }
    return p;
  }();
  return period;
}

// Writes the pattern bytes [offset, offset + size) to host_buffer.
void fill_pattern(char *host_buffer, size_t offset, size_t size) {
  const auto &period = pattern_period();
  size_t p = offset % period.size();
  for (size_t i = 0; i < size; i++) {
    host_buffer[i] = period[p];
    if (++p == period.size()) {
      p = 0;
    }
  }
}
} // namespace

ze_result_t ZePeer::try_copy(ze_command_list_handle_t command_list,
                             ze_command_queue_handle_t command_queue,
                             void *dst_buffer, void *src_buffer,
                             size_t buffer_size) {
  ze_result_t result = zeCommandListAppendMemoryCopy(
      command_list, dst_buffer, src_buffer, buffer_size, nullptr, 0, nullptr);
  if (result != ZE_RESULT_SUCCESS) {
    return result;
  }

  if (use_immediate_cmdlist) {
    return zeCommandListHostSynchronize(command_list,
                                        std::numeric_limits<uint64_t>::max());
  }

  result = zeCommandListClose(command_list);
  if (result != ZE_RESULT_SUCCESS) {
    return result;
  }
  result = zeCommandQueueExecuteCommandLists(command_queue, 1, &command_list,
                                             nullptr);
  if (result != ZE_RESULT_SUCCESS) {
    return result;
  }
  result = zeCommandQueueSynchronize(command_queue,
                                     std::numeric_limits<uint64_t>::max());
  if (result != ZE_RESULT_SUCCESS) {
    return result;
  }
  return zeCommandListReset(command_list);
}

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
      buffer_size / static_cast<long double>(ONE_GB); /* Units in Gigabytes */
  if (bidirectional) {
    total_data_transfer = 2 * total_data_transfer;
  }
  long double total_bandwidth = total_data_transfer / total_time_s;

  size_t buffer_size_formatted;
  std::string buffer_format_str;

  constexpr size_t one_kb = 1024;
  constexpr size_t one_mb = 1024 * one_kb;
  constexpr size_t one_gb = 1024 * one_mb;
  if (buffer_size % one_gb == 0) {
    buffer_size_formatted = buffer_size / one_gb;
    buffer_format_str = " GB";
  } else if (buffer_size % one_mb == 0) {
    buffer_size_formatted = buffer_size / one_mb;
    buffer_format_str = " MB";
  } else if (buffer_size % one_kb == 0) {
    buffer_size_formatted = buffer_size / one_kb;
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

// Stops the run when the devices cannot hold the buffers of a size, instead
// of failing in the allocator or running a smaller size.
void ZePeer::check_sizes(
    size_t buffer_size,
    const std::unordered_map<uint32_t, size_t> &device_buffers) {
  for (const auto &[device_id, buffers] : device_buffers) {
    ze_device_properties_t properties = {ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES};
    SUCCESS_OR_TERMINATE(
        zeDeviceGetProperties(benchmark->_devices[device_id], &properties));
    if (buffer_size > properties.maxMemAllocSize) {
      std::cerr << "[ERROR] " << buffer_size
                << " B exceeds the single allocation limit of "
                << properties.maxMemAllocSize << " B of device " << device_id
                << "\n";
      exit(1);
    }

    uint32_t count = 0;
    SUCCESS_OR_TERMINATE(zeDeviceGetMemoryProperties(
        benchmark->_devices[device_id], &count, nullptr));
    std::vector<ze_device_memory_properties_t> memories(
        count, {ZE_STRUCTURE_TYPE_DEVICE_MEMORY_PROPERTIES});
    SUCCESS_OR_TERMINATE(zeDeviceGetMemoryProperties(
        benchmark->_devices[device_id], &count, memories.data()));
    uint64_t total = 0;
    for (const auto &memory : memories) {
      total += memory.totalSize;
    }
    constexpr uint64_t headroom_percent = 80;
    const uint64_t budget = total / 100 * headroom_percent;
    const uint64_t needed = static_cast<uint64_t>(buffers) * buffer_size;
    if (needed > budget) {
      std::cerr << "[ERROR] " << buffer_size << " B needs " << needed
                << " B on device " << device_id << ", the budget is " << budget
                << " B of " << total << " B\n";
      exit(1);
    }
  }
}

void ZePeer::set_up(size_t number_buffer_elements,
                    std::vector<uint32_t> &remote_device_ids,
                    std::vector<uint32_t> &local_device_ids,
                    size_t &buffer_size) {

  size_t element_size = sizeof(char);
  buffer_size = element_size * number_buffer_elements;

  std::unordered_map<uint32_t, size_t> device_buffers;
  for (auto device_id : local_device_ids) {
    device_buffers[device_id] += 2;
  }
  for (auto device_id : remote_device_ids) {
    device_buffers[device_id] += 2;
  }
  check_sizes(buffer_size, device_buffers);

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

  const size_t host_size = std::min(buffer_size, host_chunk_size);
  void **host_buffer = reinterpret_cast<void **>(&ze_host_buffer);
  benchmark->memoryAllocHost(host_size, host_buffer);
  void **host_validate_buffer =
      reinterpret_cast<void **>(&ze_host_validate_buffer);
  benchmark->memoryAllocHost(host_size, host_validate_buffer);
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

void ZePeer::copy_and_wait(ze_command_list_handle_t command_list,
                           ze_command_queue_handle_t command_queue, void *dst,
                           const void *src, size_t size) {
  SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
      command_list, dst, src, size, nullptr, 0, nullptr));
  if (use_immediate_cmdlist) {
    SUCCESS_OR_TERMINATE(zeCommandListHostSynchronize(
        command_list, std::numeric_limits<uint64_t>::max()));
    return;
  }
  SUCCESS_OR_TERMINATE(zeCommandListClose(command_list));
  SUCCESS_OR_TERMINATE(zeCommandQueueExecuteCommandLists(
      command_queue, 1, &command_list, nullptr));
  SUCCESS_OR_TERMINATE(zeCommandQueueSynchronize(
      command_queue, std::numeric_limits<uint64_t>::max()));
  SUCCESS_OR_TERMINATE(zeCommandListReset(command_list));
}

void ZePeer::initialize_src_buffer(ze_command_list_handle_t command_list,
                                   ze_command_queue_handle_t command_queue,
                                   void *src_buffer, char *host_buffer,
                                   size_t buffer_size) {
  for (size_t offset = 0; offset < buffer_size; offset += host_chunk_size) {
    const size_t size = std::min(host_chunk_size, buffer_size - offset);
    fill_pattern(host_buffer, offset, size);
    copy_and_wait(command_list, command_queue,
                  static_cast<char *>(src_buffer) + offset, host_buffer, size);
  }
}

void ZePeer::initialize_src_buffer_immediate(
    ze_command_list_handle_t command_list, void *src_buffer, char *host_buffer,
    size_t buffer_size) {
  initialize_src_buffer(command_list, nullptr, src_buffer, host_buffer,
                        buffer_size);
}

void ZePeer::initialize_buffers(ze_command_list_handle_t command_list,
                                ze_command_queue_handle_t command_queue,
                                void *src_buffer, char *host_buffer,
                                size_t buffer_size) {
  if (src_buffer) {
    if (use_immediate_cmdlist) {
      initialize_src_buffer_immediate(command_list, src_buffer, host_buffer,
                                      buffer_size);
    } else {
      initialize_src_buffer(command_list, command_queue, src_buffer,
                            host_buffer, buffer_size);
    }
  }
}

void ZePeer::initialize_buffers(std::vector<uint32_t> &remote_device_ids,
                                std::vector<uint32_t> &local_device_ids,
                                char *host_buffer, size_t buffer_size) {
  for (auto remote_device_id : remote_device_ids) {
    ze_command_list_handle_t command_list =
        ze_peer_devices[remote_device_id].engines[0].second;
    ze_command_queue_handle_t command_queue =
        ze_peer_devices[remote_device_id].engines[0].first;
    void *src_buffer = ze_src_buffers[remote_device_id];
    if (ZePeer::use_immediate_cmdlist) {
      initialize_src_buffer_immediate(command_list, src_buffer, host_buffer,
                                      buffer_size);
    } else {
      initialize_src_buffer(command_list, command_queue, src_buffer,
                            host_buffer, buffer_size);
    }
  }

  for (auto local_device_id : local_device_ids) {
    ze_command_list_handle_t command_list =
        ze_peer_devices[local_device_id].engines[0].second;
    ze_command_queue_handle_t command_queue =
        ze_peer_devices[local_device_id].engines[0].first;
    void *src_buffer = ze_src_buffers[local_device_id];
    if (ZePeer::use_immediate_cmdlist) {
      initialize_src_buffer_immediate(command_list, src_buffer, host_buffer,
                                      buffer_size);
    } else {
      initialize_src_buffer(command_list, command_queue, src_buffer,
                            host_buffer, buffer_size);
    }
  }
}

bool ZePeer::validate_buffer(ze_command_list_handle_t command_list,
                             ze_command_queue_handle_t command_queue,
                             char *validate_buffer, void *dst_buffer,
                             char *host_buffer, size_t buffer_size) {
  for (size_t offset = 0; offset < buffer_size; offset += host_chunk_size) {
    const size_t size = std::min(host_chunk_size, buffer_size - offset);
    copy_and_wait(command_list, command_queue, validate_buffer,
                  static_cast<char *>(dst_buffer) + offset, size);
    fill_pattern(host_buffer, offset, size);
    for (size_t i = 0; i < size; i++) {
      if (validate_buffer[i] != host_buffer[i]) {
        std::cout << "Error at " << offset + i << ": validate_buffer "
                  << static_cast<uint32_t>(validate_buffer[i])
                  << " != host_buffer " << static_cast<uint32_t>(host_buffer[i])
                  << "\n";
        return false;
      }
    }
  }

  return true;
}

bool ZePeer::validate_buffer_immediate(ze_command_list_handle_t command_list,
                                       char *validate_buffer, void *dst_buffer,
                                       char *host_buffer, size_t buffer_size) {
  return ZePeer::validate_buffer(command_list, nullptr, validate_buffer,
                                 dst_buffer, host_buffer, buffer_size);
}
