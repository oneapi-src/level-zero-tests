/*
 *
 * Copyright (C) 2019-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "ze_peer.h"

#include <array>
#include <fstream>
#include <sstream>

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

static const char *copy_kernel_source_name = "ze_peer_benchmarks.cl";
static const char *copy_kernel_name = "copy_peer";

// 512 work-items per work-group, one work-group per subslice.
static const uint32_t copy_kernel_preferred_group_size = 512;

void ZePeer::create_copy_kernel(uint32_t device_id, size_t buffer_size) {
  if (copy_kernels[device_id]) {
    return;
  }

  std::ifstream source_file(copy_kernel_source_name, std::ios::in);
  if (!source_file.is_open()) {
    std::cerr << "ERROR: could not open " << copy_kernel_source_name
              << ". It is installed next to the ze_peer binary, so run the "
                 "benchmark from its install directory.\n";
    exit(1);
  }
  std::stringstream source_stream;
  source_stream << source_file.rdbuf();
  std::string source = source_stream.str();
  source.push_back('\0');

  ze_module_desc_t module_desc = {ZE_STRUCTURE_TYPE_MODULE_DESC};
  module_desc.format = ZE_MODULE_FORMAT_OCLC;
  module_desc.inputSize = source.size();
  module_desc.pInputModule = reinterpret_cast<const uint8_t *>(source.data());
  module_desc.pBuildFlags = (buffer_size >= (size_t{1} << 32))
                                ? "-ze-opt-greater-than-4GB-buffer-required"
                                : "";

  ze_module_build_log_handle_t build_log = nullptr;
  const ze_result_t result = zeModuleCreate(
      benchmark->deviceContext(device_id), benchmark->_devices[device_id],
      &module_desc, &copy_modules[device_id], &build_log);
  if (build_log != nullptr) {
    size_t log_size = 0;
    if (zeModuleBuildLogGetString(build_log, &log_size, nullptr) ==
            ZE_RESULT_SUCCESS &&
        log_size > 1) {
      std::string log(log_size, '\0');
      zeModuleBuildLogGetString(build_log, &log_size, log.data());
      std::cerr << "Build log for " << copy_kernel_source_name << ":\n"
                << log << std::endl;
    }
    zeModuleBuildLogDestroy(build_log);
  }
  if (result != ZE_RESULT_SUCCESS) {
    std::cerr << "ERROR: zeModuleCreate failed with " << result
              << ". The kernel is built from OpenCL C source, which requires "
                 "a driver supporting that module format.\n";
    exit(1);
  }

  ze_kernel_desc_t kernel_desc = {ZE_STRUCTURE_TYPE_KERNEL_DESC};
  kernel_desc.pKernelName = copy_kernel_name;
  SUCCESS_OR_TERMINATE(zeKernelCreate(copy_modules[device_id], &kernel_desc,
                                      &copy_kernels[device_id]));

  ze_device_properties_t device_properties = {
      ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES};
  SUCCESS_OR_TERMINATE(zeDeviceGetProperties(benchmark->_devices[device_id],
                                             &device_properties));
  ze_device_compute_properties_t compute_properties = {
      ZE_STRUCTURE_TYPE_DEVICE_COMPUTE_PROPERTIES};
  SUCCESS_OR_TERMINATE(zeDeviceGetComputeProperties(
      benchmark->_devices[device_id], &compute_properties));

  uint32_t group_size_x = 0;
  uint32_t group_size_y = 0;
  uint32_t group_size_z = 0;
  SUCCESS_OR_TERMINATE(zeKernelSuggestGroupSize(
      copy_kernels[device_id],
      std::min({copy_kernel_preferred_group_size,
                compute_properties.maxTotalGroupSize,
                compute_properties.maxGroupSizeX}),
      1, 1, &group_size_x, &group_size_y, &group_size_z));
  copy_kernel_group_size[device_id] = std::max(1u, group_size_x);
  copy_kernel_group_count[device_id] = std::max(
      1u, device_properties.numSlices * device_properties.numSubslicesPerSlice);
  SUCCESS_OR_TERMINATE(zeKernelSetGroupSize(
      copy_kernels[device_id], copy_kernel_group_size[device_id], 1, 1));
}

void ZePeer::set_up_kernel_copy(uint32_t device_id, uint32_t queue_index,
                                size_t buffer_size) {
  if (!use_kernel) {
    return;
  }
  if (!ze_peer_devices[device_id].compute_engines[queue_index]) {
    std::cerr << "ERROR: --kernel needs an engine with compute capability, "
              << "engine " << queue_index << " of device " << device_id
              << " has none (see -q)\n";
    exit(1);
  }
  create_copy_kernel(device_id, buffer_size);
}

void ZePeer::append_copy(ze_command_list_handle_t command_list,
                         uint32_t device_id, void *dst_buffer, void *src_buffer,
                         size_t buffer_size, ze_event_handle_t signal_event,
                         uint32_t num_wait_events,
                         ze_event_handle_t *wait_events) {
  if (!use_kernel) {
    SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
        command_list, dst_buffer, src_buffer, buffer_size, signal_event,
        num_wait_events, wait_events));
    return;
  }

  ze_kernel_handle_t kernel = copy_kernels[device_id];
  const uint64_t size = buffer_size;
  SUCCESS_OR_TERMINATE(
      zeKernelSetArgumentValue(kernel, 0, sizeof(src_buffer), &src_buffer));
  SUCCESS_OR_TERMINATE(
      zeKernelSetArgumentValue(kernel, 1, sizeof(dst_buffer), &dst_buffer));
  SUCCESS_OR_TERMINATE(
      zeKernelSetArgumentValue(kernel, 2, sizeof(size), &size));

  const size_t group_size = copy_kernel_group_size[device_id];
  const size_t elements = buffer_size / sizeof(uint32_t[4]);
  const size_t groups_needed =
      std::max<size_t>(1, (elements + group_size - 1) / group_size);
  ze_group_count_t group_count = {
      static_cast<uint32_t>(
          std::min<size_t>(copy_kernel_group_count[device_id], groups_needed)),
      1, 1};
  SUCCESS_OR_TERMINATE(zeCommandListAppendLaunchKernel(
      command_list, kernel, &group_count, signal_event, num_wait_events,
      wait_events));
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

long double ZePeer::bandwidth_gbps(size_t buffer_size, long double time_us) {
  const long double data_gb =
      buffer_size / static_cast<long double>(ONE_GB); /* Units in Gigabytes */
  return data_gb / (time_us / 1e6);
}

void ZePeer::print_results(bool bidirectional, peer_test_t test_type,
                           size_t buffer_size, long double total_time_us) {
  long double total_time_usec =
      total_time_us / static_cast<long double>(number_iterations);

  long double total_bandwidth = bandwidth_gbps(buffer_size, total_time_usec);
  if (bidirectional) {
    total_bandwidth = 2 * total_bandwidth;
  }

  print_value(test_type, buffer_size,
              (test_type == PEER_BANDWIDTH) ? total_bandwidth : total_time_usec,
              "");
}

void ZePeer::print_value(peer_test_t test_type, size_t buffer_size,
                         long double value, const std::string &label) {
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

  std::cout << ((test_type == PEER_BANDWIDTH) ? "BW [GBPS]: "
                                              : "Latency [us]: ")
            << std::fixed << std::setw(4) << buffer_size_formatted
            << buffer_format_str << ": " << std::fixed << std::setw(8)
            << std::setprecision(2) << value;
  if (!label.empty()) {
    std::cout << " " << label;
  }
  std::cout << std::endl;
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
