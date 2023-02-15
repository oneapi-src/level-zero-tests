/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <level_zero/ze_api.h>

#include "../../common/include/common.hpp"
#include "ze_app.hpp"
#include "ze_bandwidth.hpp"

#include <assert.h>
#include <iomanip>
#include <iostream>

ZeBandwidth::ZeBandwidth() {
  benchmark = new ZeApp();

  benchmark->singleDeviceInit();
}

ZeBandwidth::~ZeBandwidth() {
  if (!query_engines) {
    if (command_list_verify) {
      benchmark->commandListDestroy(command_list_verify);
    }
    if (command_list) {
      benchmark->commandListDestroy(command_list);
    }
    if (command_list1) {
      benchmark->commandListDestroy(command_list1);
    }
    if (command_queue_verify) {
      benchmark->commandQueueDestroy(command_queue_verify);
    }
    if (command_queue) {
      benchmark->commandQueueDestroy(command_queue);
    }
    if (command_queue1) {
      benchmark->commandQueueDestroy(command_queue1);
    }

    SUCCESS_OR_TERMINATE(zeEventDestroy(event));
    SUCCESS_OR_TERMINATE(zeEventDestroy(event1));
    SUCCESS_OR_TERMINATE(zeEventDestroy(wait_event));
    SUCCESS_OR_TERMINATE(zeEventPoolDestroy(event_pool));
  }

  benchmark->singleDeviceCleanup();

  delete benchmark;
}

void ZeBandwidth::calculate_metrics(
    long double total_time_nsec,     /* Units in nanoseconds */
    long double total_data_transfer, /* Units in bytes */
    long double &total_bandwidth, long double &total_latency) {
  long double total_time_s;

  total_time_s = total_time_nsec / 1e9;
  total_bandwidth = (total_data_transfer / total_time_s) / ONE_GB;
  total_latency = total_time_nsec / (1e3 * number_iterations);
}

void ZeBandwidth::print_results(size_t buffer_size, long double total_bandwidth,
                                long double total_latency,
                                std::string direction_string) {
  if (csv_output) {
    std::cout << buffer_size << "," << std::setprecision(6) << total_bandwidth
              << "," << std::setprecision(2) << total_latency << std::endl;
  } else {
    std::cout << direction_string << std::fixed << std::setw(10) << buffer_size
              << "]:  BW = " << std::setw(9) << std::setprecision(6)
              << total_bandwidth << " GBPS  Latency = " << std::setw(9)
              << std::setprecision(2) << total_latency << " usec" << std::endl;
  }
}

void ZeBandwidth::transfer_size_test(size_t size, void *destination_buffer,
                                     void *source_buffer,
                                     long double &total_time_nsec) {
  size_t element_size = sizeof(uint8_t);
  size_t buffer_size = element_size * size;
  double total_time_s;
  double total_data_transfer;

  if (verify) {
    benchmark->memoryAllocHost(buffer_size, &host_buffer_verify1);
    char *host_buffer_verify_char1 = static_cast<char *>(host_buffer_verify1);
    for (uint32_t i = 0; i < buffer_size; i++) {
      host_buffer_verify_char1[i] = i;
    }
    SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
        command_list_verify, source_buffer, host_buffer_verify1, buffer_size,
        nullptr, 0, nullptr));
    benchmark->commandListClose(command_list_verify);
    benchmark->commandQueueExecuteCommandList(command_queue_verify, 1,
                                              &command_list_verify);
    benchmark->commandQueueSynchronize(command_queue_verify);
    benchmark->commandListReset(command_list_verify);
  }

  if (use_immediate_command_list == false) {
    SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
        command_list, destination_buffer, source_buffer, buffer_size, nullptr,
        0, nullptr));
    benchmark->commandListClose(command_list);

    Timer<std::chrono::nanoseconds::period> timer;

    // warm-up
    for (uint32_t i = 0; i < warmup_iterations; i++) {
      benchmark->commandQueueExecuteCommandList(command_queue, 1,
                                                &command_list);
      benchmark->commandQueueSynchronize(command_queue);
    }

    timer.start();
    for (uint32_t i = 0; i < number_iterations; i++) {
      benchmark->commandQueueExecuteCommandList(command_queue, 1,
                                                &command_list);
      benchmark->commandQueueSynchronize(command_queue);
    }
    timer.end();

    total_time_nsec = timer.period_minus_overhead();

    benchmark->commandListReset(command_list);
  } else {
    Timer<std::chrono::nanoseconds::period> timer;

    // warm-up
    for (uint32_t i = 0; i < warmup_iterations; i++) {
      SUCCESS_OR_TERMINATE(zeEventHostReset(event));
      SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
          command_list, destination_buffer, source_buffer, buffer_size, event,
          0, nullptr));
      SUCCESS_OR_TERMINATE(zeEventHostSynchronize(event, UINT64_MAX));
    }

    total_time_nsec = 0;
    for (uint32_t i = 0; i < number_iterations; i++) {
      SUCCESS_OR_TERMINATE(zeEventHostReset(event));
      timer.start();
      SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
          command_list, destination_buffer, source_buffer, buffer_size, event,
          0, nullptr));
      SUCCESS_OR_TERMINATE(zeEventHostSynchronize(event, UINT64_MAX));
      timer.end();
      total_time_nsec += timer.period_minus_overhead();
    }
  }

  if (verify) {
    benchmark->memoryAllocHost(buffer_size, &host_buffer_verify);

    SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
        command_list_verify, host_buffer_verify, destination_buffer,
        buffer_size, nullptr, 0, nullptr));
    benchmark->commandListClose(command_list_verify);
    benchmark->commandQueueExecuteCommandList(command_queue_verify, 1,
                                              &command_list_verify);
    benchmark->commandQueueSynchronize(command_queue_verify);

    uint32_t number_of_errors = 0;
    char *host_buffer_verify_char = static_cast<char *>(host_buffer_verify);
    char *host_buffer_verify_char1 = static_cast<char *>(host_buffer_verify1);
    for (uint32_t i = 0; i < buffer_size; i++) {
      if (host_buffer_verify_char[i] != host_buffer_verify_char1[i]) {
        number_of_errors++;
      }
    }
    benchmark->memoryFree(host_buffer_verify1);
    benchmark->memoryFree(host_buffer_verify);
    benchmark->commandListReset(command_list_verify);

    if (number_of_errors > 0) {
      throw std::runtime_error("Host memory verification failed ");
    } else {
      std::cout << "Verification successful\n";
    }
  }
}

void ZeBandwidth::transfer_bidir_size_test(size_t size,
                                           void *destination_buffer,
                                           void *source_buffer,
                                           void *destination_buffer1,
                                           void *source_buffer1,
                                           long double &total_time_nsec) {
  size_t element_size = sizeof(uint8_t);
  size_t buffer_size = element_size * size;
  long double total_time_s;
  long double total_data_transfer;

  if (use_immediate_command_list == false) {
    SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
        command_list, destination_buffer, source_buffer, buffer_size, nullptr,
        1, &wait_event));
    benchmark->commandListClose(command_list);

    SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
        command_list1, destination_buffer1, source_buffer1, buffer_size,
        nullptr, 1, &wait_event));

    benchmark->commandListClose(command_list1);

    Timer<std::chrono::nanoseconds::period> timer;

    // warm-up
    for (uint32_t i = 0; i < warmup_iterations; i++) {
      benchmark->commandQueueExecuteCommandList(command_queue, 1,
                                                &command_list);
      benchmark->commandQueueExecuteCommandList(command_queue1, 1,
                                                &command_list1);

      SUCCESS_OR_TERMINATE(zeEventHostSignal(wait_event));

      benchmark->commandQueueSynchronize(command_queue);
      benchmark->commandQueueSynchronize(command_queue1);

      SUCCESS_OR_TERMINATE(zeEventHostReset(wait_event));
    }

    total_time_nsec = 0;
    for (uint32_t i = 0; i < number_iterations; i++) {
      benchmark->commandQueueExecuteCommandList(command_queue1, 1,
                                                &command_list1);
      benchmark->commandQueueExecuteCommandList(command_queue, 1,
                                                &command_list);

      timer.start();
      SUCCESS_OR_TERMINATE(zeEventHostSignal(wait_event));

      benchmark->commandQueueSynchronize(command_queue1);
      benchmark->commandQueueSynchronize(command_queue);
      timer.end();

      total_time_nsec += timer.period_minus_overhead();

      SUCCESS_OR_TERMINATE(zeEventHostReset(wait_event));
    }

    benchmark->commandListReset(command_list);
    benchmark->commandListReset(command_list1);
  } else {
    Timer<std::chrono::nanoseconds::period> timer;

    // warm-up
    for (uint32_t i = 0; i < warmup_iterations; i++) {
      SUCCESS_OR_TERMINATE(zeEventHostReset(event));
      SUCCESS_OR_TERMINATE(zeEventHostReset(event1));
      SUCCESS_OR_TERMINATE(zeEventHostReset(wait_event));
      SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
          command_list, destination_buffer, source_buffer, buffer_size, event,
          1, &wait_event));
      SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
          command_list1, destination_buffer1, source_buffer1, buffer_size,
          event1, 1, &wait_event));
      SUCCESS_OR_TERMINATE(zeEventHostSignal(wait_event));
      SUCCESS_OR_TERMINATE(zeEventHostSynchronize(event, UINT64_MAX));
      SUCCESS_OR_TERMINATE(zeEventHostSynchronize(event1, UINT64_MAX));
    }

    total_time_nsec = 0;
    for (uint32_t i = 0; i < number_iterations; i++) {
      SUCCESS_OR_TERMINATE(zeEventHostReset(event));
      SUCCESS_OR_TERMINATE(zeEventHostReset(event1));
      SUCCESS_OR_TERMINATE(zeEventHostReset(wait_event));
      SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
          command_list, destination_buffer, source_buffer, buffer_size, event,
          1, &wait_event));
      SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
          command_list1, destination_buffer1, source_buffer1, buffer_size,
          event1, 1, &wait_event));
      timer.start();
      SUCCESS_OR_TERMINATE(zeEventHostSignal(wait_event));
      SUCCESS_OR_TERMINATE(zeEventHostSynchronize(event, UINT64_MAX));
      SUCCESS_OR_TERMINATE(zeEventHostSynchronize(event1, UINT64_MAX));
      timer.end();

      total_time_nsec += timer.period_minus_overhead();
    }
  }
}

void ZeBandwidth::test_host2device(void) {
  long double total_bandwidth = 0.0;
  long double total_latency = 0.0;

  std::cout << std::endl;
  std::cout << "HOST-TO-DEVICE BANDWIDTH AND LATENCY" << std::endl;
  if (csv_output) {
    std::cout << "Transfer_size,Bandwidth_(GBPS),Latency_(usec)" << std::endl;
  }
  for (auto size : transfer_size) {
    long double total_time_nsec = 0;

    benchmark->memoryAlloc(size, &device_buffer);
    benchmark->memoryAllocHost(size, &host_buffer);

    transfer_size_test(size, device_buffer, host_buffer, total_time_nsec);

    benchmark->memoryFree(device_buffer);
    benchmark->memoryFree(host_buffer);

    calculate_metrics(total_time_nsec,
                      static_cast<long double>(size * number_iterations),
                      total_bandwidth, total_latency);
    print_results(size, total_bandwidth, total_latency, "Host->Device");
  }
}

void ZeBandwidth::test_device2host(void) {
  long double total_bandwidth;
  long double total_latency;

  std::cout << std::endl;
  std::cout << "DEVICE-TO-HOST BANDWIDTH AND LATENCY" << std::endl;
  if (csv_output) {
    std::cout << "Transfer_size,Bandwidth_(GBPS),Latency_(usec)" << std::endl;
  }
  for (auto size : transfer_size) {
    long double total_time_nsec = 0;

    benchmark->memoryAlloc(size, &device_buffer);
    benchmark->memoryAllocHost(size, &host_buffer);

    transfer_size_test(size, host_buffer, device_buffer, total_time_nsec);

    benchmark->memoryFree(device_buffer);
    benchmark->memoryFree(host_buffer);

    calculate_metrics(total_time_nsec,
                      static_cast<long double>(size * number_iterations),
                      total_bandwidth, total_latency);
    print_results(size, total_bandwidth, total_latency, "Device->Host");
  }
}

void ZeBandwidth::test_bidir(void) {
  long double total_bandwidth;
  long double total_latency;

  std::cout << std::endl;
  std::cout
      << "BIDIRECTIONAL HOST-TO-DEVICE/DEVICE-TO-HOST BANDWIDTH AND LATENCY"
      << std::endl;
  if (csv_output) {
    std::cout << "Transfer_size,Bandwidth_(GBPS),Latency_(usec)" << std::endl;
  }
  for (auto size : transfer_size) {
    long double total_time_nsec = 0;

    benchmark->memoryAlloc(size, &device_buffer);
    benchmark->memoryAllocHost(size, &host_buffer);
    benchmark->memoryAlloc(size, &device_buffer1);
    benchmark->memoryAllocHost(size, &host_buffer1);

    transfer_bidir_size_test(size, device_buffer, host_buffer, host_buffer1,
                             device_buffer1, total_time_nsec);

    benchmark->memoryFree(device_buffer);
    benchmark->memoryFree(host_buffer);
    benchmark->memoryFree(device_buffer1);
    benchmark->memoryFree(host_buffer1);

    calculate_metrics(total_time_nsec,
                      static_cast<long double>(2 * size * number_iterations),
                      total_bandwidth, total_latency);
    print_results(size, total_bandwidth, total_latency, "Host<->Device");
  }
}

//---------------------------------------------------------------------
// Utility function to query queue group properties
//---------------------------------------------------------------------
void ZeBandwidth::ze_bandwidth_query_engines() {

  uint32_t numQueueGroups = 0;
  benchmark->deviceGetCommandQueueGroupProperties(0, &numQueueGroups, nullptr);

  if (numQueueGroups == 0) {
    std::cout << " No queue groups found\n" << std::endl;
    exit(0);
  }

  queueProperties.resize(numQueueGroups);
  for (auto queueProperty : queueProperties) {
    queueProperty = {ZE_STRUCTURE_TYPE_COMMAND_QUEUE_GROUP_PROPERTIES, nullptr};
  }
  benchmark->deviceGetCommandQueueGroupProperties(0, &numQueueGroups,
                                                  queueProperties.data());

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
      std::cout << " Group " << i << " (copy): " << queueProperties[i].numQueues
                << " queues\n";
    }
  }

  if (command_queue_group_ordinal >= numQueueGroups) {
    command_queue_group_ordinal = 0;
    std::cout << "Specified command queue group " << command_queue_group_ordinal
              << " is not valid, defaulting to first group" << std::endl;
  }

  if (command_queue_group_ordinal1 >= numQueueGroups) {
    command_queue_group_ordinal1 = 0;
    std::cout << "Specified command queue group "
              << command_queue_group_ordinal1
              << " is not valid, defaulting to first group" << std::endl;
  }

  if (command_queue_index >=
      queueProperties[command_queue_group_ordinal].numQueues) {
    command_queue_index = 0;
    std::cout << "Specified command queue index " << command_queue_group_ordinal
              << " is not valid for group " << command_queue_group_ordinal
              << ", defaulting to first index" << std::endl;
  }

  if (command_queue_index1 >=
      queueProperties[command_queue_group_ordinal1].numQueues) {
    command_queue_index1 = 0;
    std::cout << "Specified command queue index "
              << command_queue_group_ordinal1 << " is not valid for group "
              << command_queue_group_ordinal1 << ", defaulting to first index"
              << std::endl;
  }

  if (run_host2dev) {
    std::cout << "Running H2D with " << command_queue_group_ordinal << ","
              << command_queue_index << "\n";
  }
  if (run_dev2host) {
    std::cout << "Running D2H with " << command_queue_group_ordinal << ","
              << command_queue_index << "\n";
  }
  if (run_bidirectional) {
    std::cout << "Running H2D with " << command_queue_group_ordinal << ","
              << command_queue_index << " and "
              << "D2H with " << command_queue_group_ordinal1 << ","
              << command_queue_index1 << "\n";
  }

  if (use_immediate_command_list == false) {
    benchmark->commandQueueCreate(0, command_queue_group_ordinal,
                                  command_queue_index, &command_queue);
    benchmark->commandListCreate(0, command_queue_group_ordinal, &command_list);

    benchmark->commandQueueCreate(0, command_queue_group_ordinal1,
                                  command_queue_index1, &command_queue1);
    benchmark->commandListCreate(0, command_queue_group_ordinal1,
                                 &command_list1);
  } else {
    ze_command_queue_desc_t command_queue_description{};
    command_queue_description.stype = ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC;
    command_queue_description.pNext = nullptr;
    command_queue_description.ordinal = command_queue_group_ordinal;
    command_queue_description.index = command_queue_index;
    command_queue_description.mode = ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS;

    SUCCESS_OR_TERMINATE(zeCommandListCreateImmediate(
        benchmark->context, benchmark->_devices[0], &command_queue_description,
        &command_list));

    command_queue_description.ordinal = command_queue_group_ordinal1;
    command_queue_description.index = command_queue_index1;
    SUCCESS_OR_TERMINATE(zeCommandListCreateImmediate(
        benchmark->context, benchmark->_devices[0], &command_queue_description,
        &command_list1));
  }

  if (verify) {
    benchmark->commandQueueCreate(0, 0, 0, &command_queue_verify);
    benchmark->commandListCreate(0, 0, &command_list_verify);
  }

  ze_event_pool_desc_t event_pool_desc = {ZE_STRUCTURE_TYPE_EVENT_POOL_DESC};
  event_pool_desc.count = 3;
  event_pool_desc.flags = ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
  SUCCESS_OR_TERMINATE(zeEventPoolCreate(benchmark->context, &event_pool_desc,
                                         1, &benchmark->_devices[0],
                                         &event_pool));

  ze_event_desc_t event_desc = {ZE_STRUCTURE_TYPE_EVENT_DESC};
  event_desc.signal = ZE_EVENT_SCOPE_FLAG_DEVICE;
  event_desc.wait = ZE_EVENT_SCOPE_FLAG_HOST;
  event_desc.index = 0;
  SUCCESS_OR_TERMINATE(zeEventCreate(event_pool, &event_desc, &event));
  event_desc.index = 1;
  SUCCESS_OR_TERMINATE(zeEventCreate(event_pool, &event_desc, &event1));
  event_desc.index = 2;
  event_desc.signal = ZE_EVENT_SCOPE_FLAG_HOST;
  event_desc.wait = ZE_EVENT_SCOPE_FLAG_DEVICE;
  SUCCESS_OR_TERMINATE(zeEventCreate(event_pool, &event_desc, &wait_event));
}

int main(int argc, char **argv) {
  ZeBandwidth bw;
  size_t default_size;
  srand(1);

  bw.parse_arguments(argc, argv);

  bw.ze_bandwidth_query_engines();

  if (!bw.query_engines) {
    default_size = bw.transfer_lower_limit;
    while (default_size < bw.transfer_upper_limit) {
      bw.transfer_size.push_back(default_size);
      default_size <<= 1;
    }
    bw.transfer_size.push_back(bw.transfer_upper_limit);

    std::cout << std::endl
              << "Iterations per transfer size = " << bw.number_iterations
              << std::endl;

    if (bw.verify) {
      bw.number_iterations = 1;
      bw.warmup_iterations = 0;
    }

    if (bw.run_host2dev) {
      bw.test_host2device();
    }

    if (bw.run_dev2host) {
      bw.test_device2host();
    }

    if (bw.run_bidirectional) {
      bw.test_bidir();
    }

    std::cout << std::endl;

    std::cout << std::flush;
  }
  return 0;
}
