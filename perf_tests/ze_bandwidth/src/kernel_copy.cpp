/*
 *
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <level_zero/ze_api.h>

#include "../../common/include/common.hpp"
#include "ze_app.hpp"
#include "ze_bandwidth.hpp"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>

// Not part of the Level Zero specification: builds a module directly from
// OpenCL C source instead of from SPIR-V.
static const ze_module_format_t module_format_oclc =
    static_cast<ze_module_format_t>(3);

static const char *kernel_source_name = "ze_bandwidth_copy.cl";
static const char *kernel_name_striding = "bandwidth_striding_copy";
static const char *kernel_name_split = "bandwidth_split_copy";

// Elements copied per thread before the first store, and the width in
// elements of one single direction stripe. Both are passed to the compiler so
// that the kernel and the host agree on a single definition.
static const uint32_t pipe_depth = 12;
static const uint32_t stripe_elements = 32;

static const size_t element_size = 16; // sizeof(uint4)

static uint32_t least_common_multiple(uint32_t a, uint32_t b) {
  return (a / std::gcd(a, b)) * b;
}

static std::string read_kernel_source(void) {
  std::ifstream source_file(kernel_source_name, std::ios::in);
  if (!source_file.is_open()) {
    std::cerr << "Could not open " << kernel_source_name
              << ". It is installed next to the ze_bandwidth binary, so run "
                 "the benchmark from its install directory.\n";
    exit(-1);
  }

  std::stringstream buffer;
  buffer << source_file.rdbuf();
  return buffer.str();
}

void ZeBandwidth::kernel_init(void) {
  std::string source = read_kernel_source();
  source.push_back('\0');

  std::stringstream build_flags;
  build_flags << "-D PIPE_DEPTH=" << pipe_depth << "u"
              << " -D STRIPE_ELEMENTS=" << stripe_elements << "u";
  const std::string build_flags_string = build_flags.str();

  module_handle.resize(benchmark->_devices.size());
  kernel_striding.resize(benchmark->_devices.size());
  kernel_split.resize(benchmark->_devices.size());

  for (auto device_id : device_ids) {
    ze_module_desc_t module_description = {};
    module_description.stype = ZE_STRUCTURE_TYPE_MODULE_DESC;
    module_description.pNext = nullptr;
    module_description.format = module_format_oclc;
    module_description.inputSize = source.size();
    module_description.pInputModule =
        reinterpret_cast<const uint8_t *>(source.data());
    module_description.pBuildFlags = build_flags_string.c_str();

    ze_module_build_log_handle_t build_log = nullptr;
    ze_result_t result = zeModuleCreate(
        benchmark->context, benchmark->_devices[device_id], &module_description,
        &module_handle[device_id], &build_log);

    if (build_log != nullptr) {
      size_t log_size = 0;
      if (zeModuleBuildLogGetString(build_log, &log_size, nullptr) ==
              ZE_RESULT_SUCCESS &&
          log_size > 1) {
        std::string log(log_size, '\0');
        zeModuleBuildLogGetString(build_log, &log_size, log.data());
        std::cerr << "Build log for " << kernel_source_name << ":\n"
                  << log << std::endl;
      }
      zeModuleBuildLogDestroy(build_log);
    }

    if (result != ZE_RESULT_SUCCESS) {
      std::cerr << "zeModuleCreate failed with " << result
                << ". The kernel is built from OpenCL C source, which requires "
                   "a driver supporting that module format.\n";
      exit(-1);
    }

    ze_kernel_desc_t kernel_description = {};
    kernel_description.stype = ZE_STRUCTURE_TYPE_KERNEL_DESC;
    kernel_description.pNext = nullptr;
    kernel_description.flags = 0;

    kernel_description.pKernelName = kernel_name_striding;
    SUCCESS_OR_TERMINATE(zeKernelCreate(module_handle[device_id],
                                        &kernel_description,
                                        &kernel_striding[device_id]));

    kernel_description.pKernelName = kernel_name_split;
    SUCCESS_OR_TERMINATE(zeKernelCreate(module_handle[device_id],
                                        &kernel_description,
                                        &kernel_split[device_id]));
  }
}

void ZeBandwidth::kernel_cleanup(void) {
  for (auto device_id : device_ids) {
    if (kernel_striding[device_id]) {
      SUCCESS_OR_TERMINATE(zeKernelDestroy(kernel_striding[device_id]));
    }
    if (kernel_split[device_id]) {
      SUCCESS_OR_TERMINATE(zeKernelDestroy(kernel_split[device_id]));
    }
    if (module_handle[device_id]) {
      SUCCESS_OR_TERMINATE(zeModuleDestroy(module_handle[device_id]));
    }
  }
}

//---------------------------------------------------------------------
// Works out the launch geometry for one transfer. The split kernel alternates
// copy direction across stripes, so it needs a whole number of stripe pairs to
// keep the two directions balanced; the striding kernel only needs a whole
// number of work groups. Either way the transfer is rounded down, and the
// number of bytes that will actually move is reported back.
//---------------------------------------------------------------------
bool ZeBandwidth::kernel_launch_config(uint32_t device_id, bool split_direction,
                                       size_t size, ze_kernel_handle_t &kernel,
                                       ze_group_count_t &group_count,
                                       uint32_t &total_threads,
                                       uint32_t &chunk_elements,
                                       size_t &bytes_moved) {
  kernel =
      split_direction ? kernel_split[device_id] : kernel_striding[device_id];

  bytes_moved = 0;
  total_threads = 0;
  chunk_elements = 1;

  uint32_t num_elements = static_cast<uint32_t>(size / element_size);
  if (num_elements == 0) {
    return false;
  }

  uint32_t group_size_x = 0;
  uint32_t group_size_y = 0;
  uint32_t group_size_z = 0;
  SUCCESS_OR_TERMINATE(zeKernelSuggestGroupSize(
      kernel, num_elements, 1, 1, &group_size_x, &group_size_y, &group_size_z));
  if (group_size_x == 0) {
    return false;
  }

  if (split_direction) {
    const uint32_t granularity =
        least_common_multiple(group_size_x, 2 * stripe_elements);
    num_elements -= num_elements % granularity;
    total_threads = num_elements;
  } else {
    // Aim for one full software pipeline per thread, then let the chunk size
    // absorb whatever is left over.
    total_threads = num_elements / pipe_depth;
    total_threads = (total_threads / group_size_x) * group_size_x;
    if (total_threads == 0) {
      total_threads = std::min(num_elements / group_size_x, 1u) * group_size_x;
    }
    if (total_threads == 0) {
      return false;
    }
    chunk_elements = num_elements / total_threads;
    num_elements = total_threads * chunk_elements;
  }

  if (num_elements == 0 || total_threads == 0) {
    return false;
  }

  SUCCESS_OR_TERMINATE(zeKernelSetGroupSize(kernel, group_size_x, 1, 1));

  group_count = {total_threads / group_size_x, 1, 1};
  bytes_moved = static_cast<size_t>(num_elements) * element_size;
  return true;
}

//---------------------------------------------------------------------
// Appends one kernel driven transfer to a command list, gated on wait_event
// like every other measured operation in this benchmark.
//---------------------------------------------------------------------
size_t ZeBandwidth::append_kernel_copy(ze_command_list_handle_t list,
                                       uint32_t device_id, bool split_direction,
                                       void *buffer_a, void *buffer_b,
                                       size_t size,
                                       ze_event_handle_t signal_event) {
  ze_kernel_handle_t kernel = nullptr;
  ze_group_count_t group_count = {};
  uint32_t total_threads = 0;
  uint32_t chunk_elements = 0;
  size_t bytes_moved = 0;

  if (!kernel_launch_config(device_id, split_direction, size, kernel,
                            group_count, total_threads, chunk_elements,
                            bytes_moved)) {
    return 0;
  }

  SUCCESS_OR_TERMINATE(
      zeKernelSetArgumentValue(kernel, 0, sizeof(buffer_a), &buffer_a));
  SUCCESS_OR_TERMINATE(
      zeKernelSetArgumentValue(kernel, 1, sizeof(buffer_b), &buffer_b));
  if (!split_direction) {
    SUCCESS_OR_TERMINATE(zeKernelSetArgumentValue(
        kernel, 2, sizeof(total_threads), &total_threads));
    SUCCESS_OR_TERMINATE(zeKernelSetArgumentValue(
        kernel, 3, sizeof(chunk_elements), &chunk_elements));
  }

  SUCCESS_OR_TERMINATE(zeCommandListAppendLaunchKernel(
      list, kernel, &group_count, signal_event, 1, &wait_event));

  return bytes_moved;
}

void ZeBandwidth::kernel_transfer_test(size_t size, bool split_direction,
                                       void *buffer_a, void *buffer_b,
                                       uint32_t device_id, bool use_h2d_slot,
                                       size_t &bytes_moved,
                                       long double &device_time_nsec) {
  ze_kernel_handle_t kernel = nullptr;
  ze_group_count_t group_count = {};
  uint32_t total_threads = 0;
  uint32_t chunk_elements = 0;

  bytes_moved = 0;
  device_time_nsec = 0.0L;

  if (!kernel_launch_config(device_id, split_direction, size, kernel,
                            group_count, total_threads, chunk_elements,
                            bytes_moved)) {
    return;
  }

  SUCCESS_OR_TERMINATE(
      zeKernelSetArgumentValue(kernel, 0, sizeof(buffer_a), &buffer_a));
  SUCCESS_OR_TERMINATE(
      zeKernelSetArgumentValue(kernel, 1, sizeof(buffer_b), &buffer_b));
  if (!split_direction) {
    SUCCESS_OR_TERMINATE(zeKernelSetArgumentValue(
        kernel, 2, sizeof(total_threads), &total_threads));
    SUCCESS_OR_TERMINATE(zeKernelSetArgumentValue(
        kernel, 3, sizeof(chunk_elements), &chunk_elements));
  }

  // The measured direction runs on the engine named for it.
  ze_command_list_handle_t list =
      use_h2d_slot ? command_list[device_id] : command_list1[device_id];
  ze_command_queue_handle_t queue =
      use_h2d_slot ? command_queue[device_id] : command_queue1[device_id];
  ze_event_handle_t timed_event =
      use_h2d_slot ? event[device_id] : event1[device_id];

  Timer<std::chrono::nanoseconds::period> timer;

  if (use_immediate_command_list == false) {
    SUCCESS_OR_TERMINATE(zeCommandListAppendLaunchKernel(
        list, kernel, &group_count, use_event_timer ? timed_event : nullptr, 1,
        &wait_event));
    benchmark->commandListClose(list);

    for (uint32_t i = 0; i < warmup_iterations; i++) {
      if (use_event_timer) {
        SUCCESS_OR_TERMINATE(zeEventHostReset(timed_event));
      }
      benchmark->commandQueueExecuteCommandList(queue, 1, &list);
      SUCCESS_OR_TERMINATE(zeEventHostSignal(wait_event));
      benchmark->commandQueueSynchronize(queue);
      SUCCESS_OR_TERMINATE(zeEventHostReset(wait_event));
    }

    for (uint32_t i = 0; i < number_iterations; i++) {
      if (use_event_timer) {
        SUCCESS_OR_TERMINATE(zeEventHostReset(timed_event));
      }
      benchmark->commandQueueExecuteCommandList(queue, 1, &list);

      timer.start();
      SUCCESS_OR_TERMINATE(zeEventHostSignal(wait_event));
      benchmark->commandQueueSynchronize(queue);
      timer.end();

      if (use_event_timer) {
        device_time_nsec += event_time_nsec(timed_event, device_id);
      } else {
        device_time_nsec += timer.period_minus_overhead();
      }

      SUCCESS_OR_TERMINATE(zeEventHostReset(wait_event));
    }

    benchmark->commandListReset(list);
  } else {
    for (uint32_t i = 0; i < warmup_iterations + number_iterations; i++) {
      SUCCESS_OR_TERMINATE(zeEventHostReset(timed_event));
      SUCCESS_OR_TERMINATE(zeCommandListAppendLaunchKernel(
          list, kernel, &group_count, timed_event, 1, &wait_event));

      timer.start();
      SUCCESS_OR_TERMINATE(zeEventHostSignal(wait_event));
      SUCCESS_OR_TERMINATE(zeEventHostSynchronize(timed_event, UINT64_MAX));
      timer.end();

      if (i >= warmup_iterations) {
        if (use_event_timer) {
          device_time_nsec += event_time_nsec(timed_event, device_id);
        } else {
          device_time_nsec += timer.period_minus_overhead();
        }
      }

      SUCCESS_OR_TERMINATE(zeEventHostReset(wait_event));
    }
  }
}

//---------------------------------------------------------------------
// Pattern helpers for the unidirectional kernel tests. Both stage through a
// host allocation and the verification queue, so the same code works whether
// the buffer under test is host or device memory.
//---------------------------------------------------------------------
void ZeBandwidth::kernel_fill_pattern(uint32_t device_id, void *buffer,
                                      size_t bytes) {
  void *staging = nullptr;
  benchmark->memoryAllocHost(bytes, &staging);

  unsigned char *staging_bytes = static_cast<unsigned char *>(staging);
  for (size_t offset = 0; offset < bytes; offset++) {
    staging_bytes[offset] = static_cast<unsigned char>(offset);
  }

  run_verify_copy(device_id, buffer, staging, bytes);

  benchmark->memoryFree(staging);
}

//---------------------------------------------------------------------
// The split kernel needs two distinct patterns so that the stripe parity can
// be told apart afterwards: the host buffer gets a ramp, the device buffer
// its complement.
//---------------------------------------------------------------------
void ZeBandwidth::kernel_fill_split_patterns(uint32_t device_id,
                                             void *host_buffer,
                                             void *device_buffer,
                                             size_t bytes) {
  void *staging = nullptr;
  benchmark->memoryAllocHost(bytes, &staging);

  unsigned char *host_bytes = static_cast<unsigned char *>(host_buffer);
  unsigned char *staging_bytes = static_cast<unsigned char *>(staging);
  for (size_t offset = 0; offset < bytes; offset++) {
    host_bytes[offset] = static_cast<unsigned char>(offset);
    staging_bytes[offset] = static_cast<unsigned char>(~offset);
  }

  run_verify_copy(device_id, device_buffer, staging, bytes);

  benchmark->memoryFree(staging);
}

void ZeBandwidth::kernel_check_pattern(uint32_t device_id, void *buffer,
                                       size_t bytes) {
  void *staging = nullptr;
  uint32_t number_of_errors = 0;

  benchmark->memoryAllocHost(bytes, &staging);

  run_verify_copy(device_id, staging, buffer, bytes);

  const unsigned char *staging_bytes =
      static_cast<const unsigned char *>(staging);
  for (size_t offset = 0; offset < bytes; offset++) {
    if (staging_bytes[offset] != static_cast<unsigned char>(offset)) {
      number_of_errors++;
    }
  }

  benchmark->memoryFree(staging);

  if (number_of_errors > 0) {
    throw std::runtime_error("Kernel copy verification failed ");
  }

  info() << "Verification successful\n";
}

//---------------------------------------------------------------------
// The split kernel is a bandwidth generator, not a copy: odd stripes move
// b into a while even stripes move a into b, so afterwards both buffers hold
// the same value in every stripe and which pattern that is depends on the
// stripe parity. Two distinct fill patterns are needed to tell them apart.
//---------------------------------------------------------------------
void ZeBandwidth::kernel_verify_split(size_t bytes_moved, void *host_buffer,
                                      void *device_buffer, uint32_t device_id) {
  const size_t stripe_bytes = stripe_elements * element_size;
  uint32_t number_of_errors = 0;
  void *readback = nullptr;

  benchmark->memoryAllocHost(bytes_moved, &readback);

  run_verify_copy(device_id, readback, device_buffer, bytes_moved);

  const unsigned char *host_bytes =
      static_cast<const unsigned char *>(host_buffer);
  const unsigned char *device_bytes =
      static_cast<const unsigned char *>(readback);

  for (size_t offset = 0; offset < bytes_moved; offset++) {
    const unsigned char pattern_a = static_cast<unsigned char>(offset);
    const unsigned char pattern_b = static_cast<unsigned char>(~offset);
    const bool odd_stripe = ((offset / stripe_bytes) & 1U) != 0U;
    const unsigned char expected = odd_stripe ? pattern_b : pattern_a;

    if (host_bytes[offset] != expected || device_bytes[offset] != expected) {
      number_of_errors++;
    }
  }

  benchmark->memoryFree(readback);

  if (number_of_errors > 0) {
    throw std::runtime_error("Split kernel verification failed ");
  }

  info() << "Verification successful\n";
}

void ZeBandwidth::test_host2device_kernel(void) {
  long double total_bandwidth = 0.0;
  long double total_latency = 0.0;

  info() << std::endl;
  info() << "HOST-TO-DEVICE BANDWIDTH AND LATENCY VIA COMPUTE KERNEL"
         << std::endl;
  if (csv_output) {
    print_csv_header(nullptr);
  }

  for (auto size : transfer_size) {
    for (auto device_id : device_ids) {
      size_t bytes_moved = 0;
      long double device_time_nsec = 0.0L;

      benchmark->memoryAlloc(device_id, size, &device_buffers[device_id]);
      benchmark->memoryAllocHost(size, &host_buffers[device_id]);

      if (verify) {
        kernel_fill_pattern(device_id, host_buffers[device_id], size);
      }

      kernel_transfer_test(size, false, device_buffers[device_id],
                           host_buffers[device_id], device_id, true,
                           bytes_moved, device_time_nsec);

      if (verify && bytes_moved > 0) {
        kernel_check_pattern(device_id, device_buffers[device_id], bytes_moved);
      }

      benchmark->memoryFree(device_buffers[device_id]);
      benchmark->memoryFree(host_buffers[device_id]);

      if (bytes_moved == 0) {
        std::cerr << "\t[skipped " << size
                  << " bytes: smaller than the kernel granularity]\n";
        continue;
      }

      calculate_metrics(device_time_nsec,
                        static_cast<long double>(bytes_moved) *
                            number_iterations,
                        total_bandwidth, total_latency);
      print_results(bytes_moved, total_bandwidth, total_latency,
                    "\t[Device " + std::to_string(device_id) + " ");
    }
  }
}

void ZeBandwidth::test_device2host_kernel(void) {
  long double total_bandwidth = 0.0;
  long double total_latency = 0.0;

  info() << std::endl;
  info() << "DEVICE-TO-HOST BANDWIDTH AND LATENCY VIA COMPUTE KERNEL"
         << std::endl;
  if (csv_output) {
    print_csv_header(nullptr);
  }

  for (auto size : transfer_size) {
    for (auto device_id : device_ids) {
      size_t bytes_moved = 0;
      long double device_time_nsec = 0.0L;

      benchmark->memoryAlloc(device_id, size, &device_buffers[device_id]);
      benchmark->memoryAllocHost(size, &host_buffers[device_id]);

      if (verify) {
        kernel_fill_pattern(device_id, device_buffers[device_id], size);
      }

      kernel_transfer_test(size, false, host_buffers[device_id],
                           device_buffers[device_id], device_id, false,
                           bytes_moved, device_time_nsec);

      if (verify && bytes_moved > 0) {
        kernel_check_pattern(device_id, host_buffers[device_id], bytes_moved);
      }

      benchmark->memoryFree(device_buffers[device_id]);
      benchmark->memoryFree(host_buffers[device_id]);

      if (bytes_moved == 0) {
        std::cerr << "\t[skipped " << size
                  << " bytes: smaller than the kernel granularity]\n";
        continue;
      }

      calculate_metrics(device_time_nsec,
                        static_cast<long double>(bytes_moved) *
                            number_iterations,
                        total_bandwidth, total_latency);
      print_results(bytes_moved, total_bandwidth, total_latency,
                    "\t[Device " + std::to_string(device_id) + " ");
    }
  }
}

//---------------------------------------------------------------------
// Half of the buffer travels in each direction inside one kernel, so only an
// aggregate figure exists: a single dispatch yields a single timestamp pair
// and the two directions cannot be separated.
//---------------------------------------------------------------------
void ZeBandwidth::test_bidir_kernel(void) {
  long double total_bandwidth = 0.0;
  long double total_latency = 0.0;
  const size_t granularity = 2 * stripe_elements * element_size;

  info() << std::endl;
  info() << "BIDIRECTIONAL HOST-TO-DEVICE/DEVICE-TO-HOST BANDWIDTH AND "
            "LATENCY VIA A SINGLE SPLIT-DIRECTION KERNEL"
         << std::endl;
  info() << "Reported bandwidth is the aggregate of both directions; each "
            "direction carries half of it."
         << std::endl;
  if (csv_output) {
    print_csv_header(nullptr);
  }

  for (auto size : transfer_size) {
    for (auto device_id : device_ids) {
      size_t bytes_moved = 0;
      long double device_time_nsec = 0.0L;

      if (size < granularity) {
        std::cerr << "\t[skipped " << size
                  << " bytes: below the split kernel granularity of "
                  << granularity << " bytes]\n";
        continue;
      }

      benchmark->memoryAlloc(device_id, size, &device_buffers[device_id]);
      benchmark->memoryAllocHost(size, &host_buffers[device_id]);

      if (verify) {
        kernel_fill_split_patterns(device_id, host_buffers[device_id],
                                   device_buffers[device_id], size);
      }

      kernel_transfer_test(size, true, host_buffers[device_id],
                           device_buffers[device_id], device_id, true,
                           bytes_moved, device_time_nsec);

      if (bytes_moved > 0 && verify) {
        kernel_verify_split(bytes_moved, host_buffers[device_id],
                            device_buffers[device_id], device_id);
      }

      benchmark->memoryFree(device_buffers[device_id]);
      benchmark->memoryFree(host_buffers[device_id]);

      if (bytes_moved == 0) {
        std::cerr << "\t[skipped " << size
                  << " bytes: smaller than the kernel granularity]\n";
        continue;
      }

      calculate_metrics(device_time_nsec,
                        static_cast<long double>(bytes_moved) *
                            number_iterations,
                        total_bandwidth, total_latency);
      print_results(bytes_moved, total_bandwidth, total_latency,
                    "\t[Device " + std::to_string(device_id) + " ");
    }
  }
}
