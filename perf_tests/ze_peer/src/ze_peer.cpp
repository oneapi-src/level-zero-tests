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
#include <iomanip>
#include <iostream>

class ZePeer {
public:
  ZePeer() {
    benchmark = new ZeApp("ze_peer_benchmarks.spv");

    driver_count = benchmark->driverCount();
    assert(driver_count > 0);

    /* Retrieve driver number 1 */
    driver_count = 1;
    benchmark->driverGet(&driver_count, &driver);

    /* Create a context for all resources*/
    ze_context_desc_t context_desc = {};
    context_desc.stype = ZE_STRUCTURE_TYPE_CONTEXT_DESC;
    benchmark->contextCreate(driver, context_desc, &context);

    /* Obtain device count */
    device_count = benchmark->deviceCount(driver);
    devices = new std::vector<ze_device_handle_t>(device_count);

    /* Retrieve array of devices in driver */
    benchmark->driverGetDevices(driver, device_count, devices->data());

    device_contexts = new std::vector<device_context_t>(device_count);

    for (uint32_t i = 0; i < device_count; i++) {
      ze_device_handle_t device;
      ze_module_handle_t module;
      ze_command_queue_handle_t command_queue;
      device_context_t *device_context = &device_contexts->at(i);

      device = devices->at(i);
      benchmark->moduleCreate(device, &module);
      benchmark->commandQueueCreate(device, 0 /* command_queue_id */,
                                    &command_queue);
      device_context->device = device;
      device_context->module = module;
      device_context->command_queue = command_queue;
      benchmark->commandListCreate(device, &device_context->command_list);
    }
  }

  ~ZePeer() {
    for (uint32_t i = 0; i < device_count; i++) {
      device_context_t *device_context = &device_contexts->at(i);
      benchmark->moduleDestroy(device_context->module);
      benchmark->commandQueueDestroy(device_context->command_queue);
      benchmark->commandListDestroy(device_context->command_list);
    }
    benchmark->contextDestroy(context);
    delete benchmark;
    delete device_contexts;
    delete devices;
  }

  void bandwidth(bool bidirectional, peer_transfer_t transfer_type);
  void latency(bool bidirectional, peer_transfer_t transfer_type);

private:
  ZeApp *benchmark;
  ze_context_handle_t context;
  ze_driver_handle_t driver;
  uint32_t driver_count;
  uint32_t device_count;
  std::vector<device_context_t> *device_contexts;
  std::vector<ze_device_handle_t> *devices;

  void _copy_function_setup(ze_module_handle_t module,
                            ze_kernel_handle_t &function,
                            const char *function_name, uint32_t globalSizeX,
                            uint32_t globalSizeY, uint32_t globalSizeZ,
                            uint32_t &group_size_x, uint32_t &group_size_y,
                            uint32_t &group_size_z);
  void _copy_function_cleanup(ze_kernel_handle_t function);
};

void ZePeer::_copy_function_setup(ze_module_handle_t module,
                                  ze_kernel_handle_t &function,
                                  const char *function_name,
                                  uint32_t globalSizeX, uint32_t globalSizeY,
                                  uint32_t globalSizeZ, uint32_t &group_size_x,
                                  uint32_t &group_size_y,
                                  uint32_t &group_size_z) {
  group_size_x = 0;
  group_size_y = 0;
  group_size_z = 0;

  benchmark->functionCreate(module, &function, function_name);

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
  std::vector<void *> buffers(device_count, nullptr);
  size_t element_size = sizeof(unsigned long int);
  size_t buffer_size = element_size * number_buffer_elements;

  for (uint32_t i = 0; i < device_count; i++) {
    device_context_t *device_context = &device_contexts->at(i);

    benchmark->memoryAlloc(context, device_context->device, buffer_size,
                           &buffers[i]);
  }

  for (uint32_t i = 0; i < device_count; i++) {
    device_context_t *device_context_i = &device_contexts->at(i);
    ze_kernel_handle_t function_a = nullptr;
    ze_kernel_handle_t function_b = nullptr;
    void *buffer_i = buffers.at(i);
    ze_command_list_handle_t command_list_a = device_context_i->command_list;
    ze_command_queue_handle_t command_queue_a = device_context_i->command_queue;
    ze_group_count_t thread_group_dimensions;
    uint32_t group_size_x;
    uint32_t group_size_y;
    uint32_t group_size_z;

    _copy_function_setup(device_context_i->module, function_a,
                         "single_copy_peer_to_peer", number_buffer_elements, 1,
                         1, group_size_x, group_size_y, group_size_z);
    if (bidirectional) {
      _copy_function_setup(device_context_i->module, function_b,
                           "single_copy_peer_to_peer", number_buffer_elements,
                           1, 1, group_size_x, group_size_y, group_size_z);
    }

    const int group_count_x = number_buffer_elements / group_size_x;
    thread_group_dimensions.groupCountX = group_count_x;
    thread_group_dimensions.groupCountY = 1;
    thread_group_dimensions.groupCountZ = 1;

    for (uint32_t j = 0; j < device_count; j++) {
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
    benchmark->memoryFree(context, buffer);
  }
}

void ZePeer::latency(bool bidirectional, peer_transfer_t transfer_type) {
  int number_iterations = 100;
  int warm_up_iterations = 5;
  int number_buffer_elements = 1;
  std::vector<void *> buffers(device_count, nullptr);
  size_t element_size = sizeof(unsigned long int);
  size_t buffer_size = element_size * number_buffer_elements;

  for (uint32_t i = 0; i < device_count; i++) {
    device_context_t *device_context = &device_contexts->at(i);

    benchmark->memoryAlloc(context, device_context->device, buffer_size,
                           &buffers[i]);
  }

  for (uint32_t i = 0; i < device_count; i++) {
    device_context_t *device_context_i = &device_contexts->at(i);
    ze_kernel_handle_t function_a = nullptr;
    ze_kernel_handle_t function_b = nullptr;
    void *buffer_i = buffers.at(i);
    ze_command_list_handle_t command_list_a = device_context_i->command_list;
    ze_command_queue_handle_t command_queue_a = device_context_i->command_queue;
    ze_group_count_t thread_group_dimensions;
    uint32_t group_size_x;
    uint32_t group_size_y;
    uint32_t group_size_z;

    _copy_function_setup(device_context_i->module, function_a,
                         "single_copy_peer_to_peer", number_buffer_elements, 1,
                         1, group_size_x, group_size_y, group_size_z);
    if (bidirectional) {
      _copy_function_setup(device_context_i->module, function_b,
                           "single_copy_peer_to_peer", number_buffer_elements,
                           1, 1, group_size_x, group_size_y, group_size_z);
    }

    const int group_count_x = number_buffer_elements / group_size_x;
    thread_group_dimensions.groupCountX = group_count_x;
    thread_group_dimensions.groupCountY = 1;
    thread_group_dimensions.groupCountZ = 1;

    for (uint32_t j = 0; j < device_count; j++) {
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
    benchmark->memoryFree(context, buffer);
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

  std::cout << "Unidirectional Bandwidth P2P Write" << std::endl;
  peer.latency(false /* unidirectional */, PEER_WRITE);
  std::cout << std::endl;

  std::cout << "Unidirectional Bandwidth P2P Read" << std::endl;
  peer.latency(false /* unidirectional */, PEER_READ);
  std::cout << std::endl;

  std::cout << "Bidirectional Bandwidth P2P Write" << std::endl;
  peer.latency(true /* bidirectional */, PEER_NONE);
  std::cout << std::endl;

  std::cout << std::flush;

  return 0;
}
