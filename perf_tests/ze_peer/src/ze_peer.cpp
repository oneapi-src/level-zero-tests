/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <level_zero/ze_api.h>

#include "common.hpp"
#include "ze_app.hpp"
#include "ze_peer.h"

#include <assert.h>
#include <stdlib.h>
#include <iomanip>
#include <iostream>
#include <vector>

class ZePeer {
public:
  ZePeer() {

    benchmark = new ZeApp("ze_peer_benchmarks.spv");

    uint32_t device_count = benchmark->allDevicesInit();
    for (uint32_t i = 0;
         (i < device_count - 1) && (device_environs.size() == 0); i++) {
      for (uint32_t j = i + 1; j < device_count; j++) {
        if (benchmark->canAccessPeer(i, j)) {
          if (device_environs.size() == 0) {
            device_environs.emplace_back(i);
          }
          device_environs.emplace_back(j);
        }
      }
    }

    if (device_environs.size() == 0) {
      std::cerr << "ERROR: there are no peer devices among " << device_count
                << " devices found" << std::endl;
      std::terminate();
    }

    std::cout << "running tests with " << device_environs.size()
              << " peer devices out of " << device_count << " devices found"
              << std::endl;

    for (uint32_t i = 0; i < device_environs.size(); i++) {
      benchmark->commandQueueCreate(device_environs[i].device_index,
                                    0 /* command queue group ordinal */,
                                    &device_environs[i].command_queue);
      benchmark->commandListCreate(device_environs[i].device_index,
                                   &device_environs[i].command_list);
    }
  }

  ~ZePeer() {
    for (uint32_t i = 0; i < device_environs.size(); i++) {
      benchmark->commandQueueDestroy(device_environs[i].command_queue);
      benchmark->commandListDestroy(device_environs[i].command_list);
    }
    benchmark->allDevicesCleanup();
    delete benchmark;
  }

  void bandwidth(bool bidirectional, peer_transfer_t transfer_type);
  void latency(bool bidirectional, peer_transfer_t transfer_type);

private:
  ZeApp *benchmark;
  std::vector<device_environ_t> device_environs;

  void _copy_function_setup(const uint32_t device_index,
                            ze_kernel_handle_t &function,
                            const char *function_name, uint32_t globalSizeX,
                            uint32_t globalSizeY, uint32_t globalSizeZ,
                            uint32_t &group_size_x, uint32_t &group_size_y,
                            uint32_t &group_size_z);
  void _copy_function_cleanup(ze_kernel_handle_t function);
};

void ZePeer::_copy_function_setup(const uint32_t device_index,
                                  ze_kernel_handle_t &function,
                                  const char *function_name,
                                  uint32_t globalSizeX, uint32_t globalSizeY,
                                  uint32_t globalSizeZ, uint32_t &group_size_x,
                                  uint32_t &group_size_y,
                                  uint32_t &group_size_z) {
  group_size_x = 0;
  group_size_y = 0;
  group_size_z = 0;

  benchmark->functionCreate(device_index, &function, function_name);

  SUCCESS_OR_TERMINATE(
      zeKernelSuggestGroupSize(function, globalSizeX, globalSizeY, globalSizeZ,
                               &group_size_x, &group_size_y, &group_size_z));
  SUCCESS_OR_TERMINATE(
      zeKernelSetGroupSize(function, group_size_x, group_size_y, group_size_z));
}

void ZePeer::_copy_function_cleanup(ze_kernel_handle_t function) {
  benchmark->functionDestroy(function);
}

void ZePeer::bandwidth(bool bidirectional, peer_transfer_t transfer_type) {
  int number_iterations = 5;
  int warm_up_iterations = 5;
  int number_buffer_elements = 10000000;
  std::vector<void *> buffers(device_environs.size(), nullptr);
  size_t element_size = sizeof(unsigned long int);
  size_t buffer_size = element_size * number_buffer_elements;

  for (uint32_t i = 0; i < device_environs.size(); i++) {
    benchmark->memoryAlloc(device_environs[i].device_index, buffer_size,
                           &buffers[i]);
  }

  for (uint32_t i = 0; i < device_environs.size(); i++) {
    ze_kernel_handle_t function_a = nullptr;
    ze_kernel_handle_t function_b = nullptr;
    void *buffer_i = buffers.at(i);
    ze_command_list_handle_t command_list_a = device_environs[i].command_list;
    ze_command_queue_handle_t command_queue_a =
        device_environs[i].command_queue;
    ze_group_count_t thread_group_dimensions;
    uint32_t group_size_x;
    uint32_t group_size_y;
    uint32_t group_size_z;

    _copy_function_setup(device_environs[i].device_index, function_a,
                         "single_copy_peer_to_peer", number_buffer_elements, 1,
                         1, group_size_x, group_size_y, group_size_z);
    if (bidirectional) {
      _copy_function_setup(device_environs[i].device_index, function_b,
                           "single_copy_peer_to_peer", number_buffer_elements,
                           1, 1, group_size_x, group_size_y, group_size_z);
    }

    const int group_count_x = number_buffer_elements / group_size_x;
    thread_group_dimensions.groupCountX = group_count_x;
    thread_group_dimensions.groupCountY = 1;
    thread_group_dimensions.groupCountZ = 1;

    for (uint32_t j = 0; j < device_environs.size(); j++) {
      long double total_time_usec;
      long double total_time_s;
      long double total_data_transfer;
      long double total_bandwidth;
      Timer<std::chrono::microseconds::period> timer;
      void *buffer_j = buffers.at(j);

      if (bidirectional) {
        /* PEER_WRITE */
        SUCCESS_OR_TERMINATE(
            zeKernelSetArgumentValue(function_a, 0, /* Destination buffer*/
                                     sizeof(buffer_j), &buffer_j));
        SUCCESS_OR_TERMINATE(
            zeKernelSetArgumentValue(function_a, 1, /* Source buffer */
                                     sizeof(buffer_i), &buffer_i));
        /* PEER_READ */
        SUCCESS_OR_TERMINATE(
            zeKernelSetArgumentValue(function_b, 0, /* Destination buffer*/
                                     sizeof(buffer_i), &buffer_i));
        SUCCESS_OR_TERMINATE(
            zeKernelSetArgumentValue(function_b, 1, /* Source buffer */
                                     sizeof(buffer_j), &buffer_j));

        zeCommandListAppendLaunchKernel(command_list_a, function_a,
                                        &thread_group_dimensions, nullptr, 0,
                                        nullptr);
        zeCommandListAppendLaunchKernel(command_list_a, function_b,
                                        &thread_group_dimensions, nullptr, 0,
                                        nullptr);
      } else { /* unidirectional */
        if (transfer_type == PEER_WRITE) {
          SUCCESS_OR_TERMINATE(
              zeKernelSetArgumentValue(function_a, 0, /* Destination buffer*/
                                       sizeof(buffer_j), &buffer_j));
          SUCCESS_OR_TERMINATE(
              zeKernelSetArgumentValue(function_a, 1, /* Source buffer */
                                       sizeof(buffer_i), &buffer_i));
        } else if (transfer_type == PEER_READ) {
          SUCCESS_OR_TERMINATE(
              zeKernelSetArgumentValue(function_a, 0, /* Destination buffer*/
                                       sizeof(buffer_i), &buffer_i));
          SUCCESS_OR_TERMINATE(
              zeKernelSetArgumentValue(function_a, 1, /* Source buffer */
                                       sizeof(buffer_j), &buffer_j));
        } else {
          std::cerr
              << "ERROR: Bandwidth test - transfer type parameter is invalid"
              << std::endl;
          std::terminate();
        }

        zeCommandListAppendLaunchKernel(command_list_a, function_a,
                                        &thread_group_dimensions, nullptr, 0,
                                        nullptr);
      }
      benchmark->commandListClose(command_list_a);

      /* Warm up */
      for (int i = 0; i < warm_up_iterations; i++) {
        benchmark->commandQueueExecuteCommandList(command_queue_a, 1,
                                                  &command_list_a);
        benchmark->commandQueueSynchronize(command_queue_a);
      }

      timer.start();
      for (int i = 0; i < number_iterations; i++) {
        benchmark->commandQueueExecuteCommandList(command_queue_a, 1,
                                                  &command_list_a);
        benchmark->commandQueueSynchronize(command_queue_a);
      }
      timer.end();

      total_time_usec = timer.period_minus_overhead();
      total_time_s = total_time_usec / 1e6;

      total_data_transfer =
          (buffer_size * number_iterations) /
          static_cast<long double>(1e9); /* Units in Gigabytes */
      if (bidirectional) {
        total_data_transfer = 2 * total_data_transfer;
      }
      total_bandwidth = total_data_transfer / total_time_s;

      if (bidirectional) {
        std::cout << std::setprecision(11) << std::setw(8) << " Device(" << i
                  << ")<->Device(" << j << "): "
                  << " GBPS " << total_bandwidth << std::endl;
      } else {
        if (transfer_type == PEER_WRITE) {
          std::cout << std::setprecision(11) << std::setw(8) << " Device(" << i
                    << ")->Device(" << j << "): "
                    << " GBPS " << total_bandwidth << std::endl;
        } else {
          std::cout << std::setprecision(11) << std::setw(8) << " Device(" << i
                    << ")<-Device(" << j << "): "
                    << " GBPS " << total_bandwidth << std::endl;
        }
      }
      benchmark->commandListReset(command_list_a);
    }
    _copy_function_cleanup(function_a);
    if (bidirectional) {
      _copy_function_cleanup(function_b);
    }
  }

  for (void *buffer : buffers) {
    benchmark->memoryFree(buffer);
  }
}

void ZePeer::latency(bool bidirectional, peer_transfer_t transfer_type) {
  int number_iterations = 100;
  int warm_up_iterations = 5;
  int number_buffer_elements = 1;
  std::vector<void *> buffers(device_environs.size(), nullptr);
  size_t element_size = sizeof(unsigned long int);
  size_t buffer_size = element_size * number_buffer_elements;

  for (uint32_t i = 0; i < device_environs.size(); i++) {
    benchmark->memoryAlloc(device_environs[i].device_index, buffer_size,
                           &buffers[i]);
  }

  for (uint32_t i = 0; i < device_environs.size(); i++) {
    ze_kernel_handle_t function_a = nullptr;
    ze_kernel_handle_t function_b = nullptr;
    void *buffer_i = buffers.at(i);
    ze_command_list_handle_t command_list_a = device_environs[i].command_list;
    ze_command_queue_handle_t command_queue_a =
        device_environs[i].command_queue;
    ze_group_count_t thread_group_dimensions;
    uint32_t group_size_x;
    uint32_t group_size_y;
    uint32_t group_size_z;

    _copy_function_setup(device_environs[i].device_index, function_a,
                         "single_copy_peer_to_peer", number_buffer_elements, 1,
                         1, group_size_x, group_size_y, group_size_z);
    if (bidirectional) {
      _copy_function_setup(device_environs[i].device_index, function_b,
                           "single_copy_peer_to_peer", number_buffer_elements,
                           1, 1, group_size_x, group_size_y, group_size_z);
    }

    const int group_count_x = number_buffer_elements / group_size_x;
    thread_group_dimensions.groupCountX = group_count_x;
    thread_group_dimensions.groupCountY = 1;
    thread_group_dimensions.groupCountZ = 1;

    for (uint32_t j = 0; j < device_environs.size(); j++) {
      long double total_time_usec;
      Timer<std::chrono::microseconds::period> timer;
      void *buffer_j = buffers.at(j);

      if (bidirectional) {
        /* PEER_WRITE */
        SUCCESS_OR_TERMINATE(
            zeKernelSetArgumentValue(function_a, 0, /* Destination buffer*/
                                     sizeof(buffer_j), &buffer_j));
        SUCCESS_OR_TERMINATE(
            zeKernelSetArgumentValue(function_a, 1, /* Source buffer */
                                     sizeof(buffer_i), &buffer_i));
        /* PEER_READ */
        SUCCESS_OR_TERMINATE(
            zeKernelSetArgumentValue(function_b, 0, /* Destination buffer*/
                                     sizeof(buffer_i), &buffer_i));
        SUCCESS_OR_TERMINATE(
            zeKernelSetArgumentValue(function_b, 1, /* Source buffer */
                                     sizeof(buffer_j), &buffer_j));

        zeCommandListAppendLaunchKernel(command_list_a, function_a,
                                        &thread_group_dimensions, nullptr, 0,
                                        nullptr);
        zeCommandListAppendLaunchKernel(command_list_a, function_b,
                                        &thread_group_dimensions, nullptr, 0,
                                        nullptr);
      } else { /* unidirectional */
        if (transfer_type == PEER_WRITE) {
          SUCCESS_OR_TERMINATE(
              zeKernelSetArgumentValue(function_a, 0, /* Destination buffer*/
                                       sizeof(buffer_j), &buffer_j));
          SUCCESS_OR_TERMINATE(
              zeKernelSetArgumentValue(function_a, 1, /* Source buffer */
                                       sizeof(buffer_i), &buffer_i));
        } else if (transfer_type == PEER_READ) {
          SUCCESS_OR_TERMINATE(
              zeKernelSetArgumentValue(function_a, 0, /* Destination buffer*/
                                       sizeof(buffer_i), &buffer_i));
          SUCCESS_OR_TERMINATE(
              zeKernelSetArgumentValue(function_a, 1, /* Source buffer */
                                       sizeof(buffer_j), &buffer_j));
        } else {
          std::cerr
              << "ERROR: Latency test - transfer type parameter is invalid"
              << std::endl;
          std::terminate();
        }

        zeCommandListAppendLaunchKernel(command_list_a, function_a,
                                        &thread_group_dimensions, nullptr, 0,
                                        nullptr);
      }
      benchmark->commandListClose(command_list_a);

      /* Warm up */
      for (int i = 0; i < warm_up_iterations; i++) {
        benchmark->commandQueueExecuteCommandList(command_queue_a, 1,
                                                  &command_list_a);
        benchmark->commandQueueSynchronize(command_queue_a);
      }

      timer.start();
      for (int i = 0; i < number_iterations; i++) {
        benchmark->commandQueueExecuteCommandList(command_queue_a, 1,
                                                  &command_list_a);
        benchmark->commandQueueSynchronize(command_queue_a);
      }
      timer.end();

      total_time_usec = timer.period_minus_overhead() /
                        static_cast<long double>(number_iterations);

      if (bidirectional) {
        std::cout << std::setprecision(11) << std::setw(8) << " Device(" << i
                  << ")<->Device(" << j << "): " << total_time_usec << " uS"
                  << std::endl;
      } else {
        if (transfer_type == PEER_WRITE) {
          std::cout << std::setprecision(11) << std::setw(8) << " Device(" << i
                    << ")->Device(" << j << "): " << total_time_usec << " uS"
                    << std::endl;
        } else {
          std::cout << std::setprecision(11) << std::setw(8) << " Device(" << i
                    << ")<-Device(" << j << "): " << total_time_usec << " uS"
                    << std::endl;
        }
      }
      benchmark->commandListReset(command_list_a);
    }
    _copy_function_cleanup(function_a);
    if (bidirectional) {
      _copy_function_cleanup(function_b);
    }
  }

  for (void *buffer : buffers) {
    benchmark->memoryFree(buffer);
  }
}

int main(int argc, char **argv) {
  ZePeer peer;

  std::cout << "Unidirectional Bandwidth P2P Write" << std::endl;
  peer.bandwidth(false /* unidirectional */, PEER_WRITE);
  std::cout << std::endl;

  std::cout << "Unidirectional Bandwidth P2P Read" << std::endl;
  peer.bandwidth(false /* unidirectional */, PEER_READ);
  std::cout << std::endl;

  std::cout << "Bidirectional Bandwidth P2P Write" << std::endl;
  peer.bandwidth(true /* bidirectional */, PEER_NONE);
  std::cout << std::endl;

  std::cout << "Unidirectional Latency P2P Write" << std::endl;
  peer.latency(false /* unidirectional */, PEER_WRITE);
  std::cout << std::endl;

  std::cout << "Unidirectional Latency P2P Read" << std::endl;
  peer.latency(false /* unidirectional */, PEER_READ);
  std::cout << std::endl;

  std::cout << "Bidirectional Latency P2P Write" << std::endl;
  peer.latency(true /* bidirectional */, PEER_NONE);
  std::cout << std::endl;

  std::cout << std::flush;

  return 0;
}
