/*
 *
 * Copyright (C) 2019-2023 Intel Corporation
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

  benchmark->allDevicesInit();
}

ZeBandwidth::~ZeBandwidth() {
  if (!query_engines) {
    if (command_list_verify) {
      benchmark->commandListDestroy(command_list_verify);
    }
    if (command_queue_verify) {
      benchmark->commandQueueDestroy(command_queue_verify);
    }
    for (auto device_id : device_ids) {
      if (command_list[device_id]) {
        benchmark->commandListDestroy(command_list[device_id]);
      }
      if (command_list1[device_id]) {
        benchmark->commandListDestroy(command_list1[device_id]);
      }
      if (command_queue[device_id]) {
        benchmark->commandQueueDestroy(command_queue[device_id]);
      }
      if (command_queue1[device_id]) {
        benchmark->commandQueueDestroy(command_queue1[device_id]);
      }

      SUCCESS_OR_TERMINATE(zeEventDestroy(event[device_id]));
      SUCCESS_OR_TERMINATE(zeEventDestroy(event1[device_id]));
    }

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

void ZeBandwidth::transfer_size_test(
    size_t size, std::vector<void *> &destination_buffer,
    std::vector<void *> &source_buffer,
    std::vector<long double> &device_times_nsec, long double &total_time_nsec) {
  size_t element_size = sizeof(uint8_t);
  size_t buffer_size = element_size * size;

  if (verify) {
    benchmark->memoryAllocHost(buffer_size, &host_buffer_verify1);
    char *host_buffer_verify_char1 = static_cast<char *>(host_buffer_verify1);
    for (uint32_t i = 0; i < buffer_size; i++) {
      host_buffer_verify_char1[i] = i;
    }

    for (auto device_id : device_ids) {
      SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
          command_list_verify, source_buffer[device_id], host_buffer_verify1,
          buffer_size, nullptr, 0, nullptr));
      benchmark->commandListClose(command_list_verify);
      benchmark->commandQueueExecuteCommandList(command_queue_verify, 1,
                                                &command_list_verify);
      benchmark->commandQueueSynchronize(command_queue_verify);
      benchmark->commandListReset(command_list_verify);
    }
  }

  if (use_immediate_command_list == false) {
    for (auto device_id : device_ids) {
      SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
          command_list[device_id], destination_buffer[device_id],
          source_buffer[device_id], buffer_size, nullptr, 1, &wait_event));
    }

    for (auto device_id : device_ids) {
      benchmark->commandListClose(command_list[device_id]);
    }

    // warm-up
    for (uint32_t i = 0; i < warmup_iterations; i++) {
      for (auto device_id : device_ids) {
        benchmark->commandQueueExecuteCommandList(command_queue[device_id], 1,
                                                  &command_list[device_id]);
      }

      SUCCESS_OR_TERMINATE(zeEventHostSignal(wait_event));

      for (auto device_id : device_ids) {
        benchmark->commandQueueSynchronize(command_queue[device_id]);
      }

      SUCCESS_OR_TERMINATE(zeEventHostReset(wait_event));
    }

    Timer<std::chrono::nanoseconds::period> timer;
    std::vector<Timer<std::chrono::nanoseconds::period>> timers(
        benchmark->_devices.size());

    timer.start();
    for (uint32_t i = 0; i < number_iterations; i++) {
      for (auto device_id : device_ids) {
        benchmark->commandQueueExecuteCommandList(command_queue[device_id], 1,
                                                  &command_list[device_id]);
      }

      for (auto device_id : device_ids) {
        timers[device_id].start();
      }

      SUCCESS_OR_TERMINATE(zeEventHostSignal(wait_event));

      for (auto device_id : device_ids) {
        benchmark->commandQueueSynchronize(command_queue[device_id]);

        timers[device_id].end();
        device_times_nsec[device_id] +=
            timers[device_id].period_minus_overhead();
      }

      SUCCESS_OR_TERMINATE(zeEventHostReset(wait_event));
    }
    timer.end();

    total_time_nsec = timer.period_minus_overhead();

    for (auto device_id : device_ids) {
      benchmark->commandListReset(command_list[device_id]);
    }
  } else {
    // warm-up
    for (uint32_t i = 0; i < warmup_iterations; i++) {
      for (auto device_id : device_ids) {
        SUCCESS_OR_TERMINATE(zeEventHostReset(event[device_id]));
        SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
            command_list[device_id], destination_buffer[device_id],
            source_buffer[device_id], buffer_size, event[device_id], 1,
            &wait_event));
      }

      SUCCESS_OR_TERMINATE(zeEventHostSignal(wait_event));

      for (auto device_id : device_ids) {
        SUCCESS_OR_TERMINATE(
            zeEventHostSynchronize(event[device_id], UINT64_MAX));
      }

      SUCCESS_OR_TERMINATE(zeEventHostReset(wait_event));
    }

    Timer<std::chrono::nanoseconds::period> timer;
    std::vector<Timer<std::chrono::nanoseconds::period>> timers(
        benchmark->_devices.size());

    timer.start();
    for (uint32_t i = 0; i < number_iterations; i++) {
      for (auto device_id : device_ids) {
        SUCCESS_OR_TERMINATE(zeEventHostReset(event[device_id]));
        SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
            command_list[device_id], destination_buffer[device_id],
            source_buffer[device_id], buffer_size, event[device_id], 1,
            &wait_event));
      }

      for (auto device_id : device_ids) {
        timers[device_id].start();
      }

      SUCCESS_OR_TERMINATE(zeEventHostSignal(wait_event));

      for (auto device_id : device_ids) {
        SUCCESS_OR_TERMINATE(
            zeEventHostSynchronize(event[device_id], UINT64_MAX));

        timers[device_id].end();
        device_times_nsec[device_id] +=
            timers[device_id].period_minus_overhead();
      }

      SUCCESS_OR_TERMINATE(zeEventHostReset(wait_event));
    }
    timer.end();

    total_time_nsec = timer.period_minus_overhead();
  }

  if (verify) {
    uint32_t number_of_errors = 0;
    benchmark->memoryAllocHost(buffer_size, &host_buffer_verify);

    for (auto device_id : device_ids) {
      SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
          command_list_verify, host_buffer_verify,
          destination_buffer[device_id], buffer_size, nullptr, 0, nullptr));
      benchmark->commandListClose(command_list_verify);
      benchmark->commandQueueExecuteCommandList(command_queue_verify, 1,
                                                &command_list_verify);
      benchmark->commandQueueSynchronize(command_queue_verify);
      benchmark->commandListReset(command_list_verify);

      char *host_buffer_verify_char = static_cast<char *>(host_buffer_verify);
      char *host_buffer_verify_char1 = static_cast<char *>(host_buffer_verify1);
      for (uint32_t i = 0; i < buffer_size; i++) {
        if (host_buffer_verify_char[i] != host_buffer_verify_char1[i]) {
          number_of_errors++;
        }
      }
    }

    benchmark->memoryFree(host_buffer_verify1);
    benchmark->memoryFree(host_buffer_verify);

    if (number_of_errors > 0) {
      throw std::runtime_error("Host memory verification failed ");
    } else {
      std::cout << "Verification successful\n";
    }
  }
}

void ZeBandwidth::transfer_bidir_size_test(
    size_t size, std::vector<void *> &destination_buffer,
    std::vector<void *> &source_buffer,
    std::vector<void *> &destination_buffer1,
    std::vector<void *> &source_buffer1,
    std::vector<long double> &device_times_nsec, long double &total_time_nsec) {
  size_t element_size = sizeof(uint8_t);
  size_t buffer_size = element_size * size;
  long double total_time_s;
  long double total_data_transfer;

  if (use_immediate_command_list == false) {
    for (auto device_id : device_ids) {
      SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
          command_list[device_id], destination_buffer[device_id],
          source_buffer[device_id], buffer_size, nullptr, 1, &wait_event));
    }

    for (auto device_id : device_ids) {
      benchmark->commandListClose(command_list[device_id]);
    }

    for (auto device_id : device_ids) {
      SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
          command_list1[device_id], destination_buffer1[device_id],
          source_buffer1[device_id], buffer_size, nullptr, 1, &wait_event));
    }

    for (auto device_id : device_ids) {
      benchmark->commandListClose(command_list1[device_id]);
    }

    // warm-up
    for (uint32_t i = 0; i < warmup_iterations; i++) {
      for (auto device_id : device_ids) {
        benchmark->commandQueueExecuteCommandList(command_queue[device_id], 1,
                                                  &command_list[device_id]);
        benchmark->commandQueueExecuteCommandList(command_queue1[device_id], 1,
                                                  &command_list1[device_id]);
      }

      SUCCESS_OR_TERMINATE(zeEventHostSignal(wait_event));

      for (auto device_id : device_ids) {
        benchmark->commandQueueSynchronize(command_queue[device_id]);
        benchmark->commandQueueSynchronize(command_queue1[device_id]);
      }

      SUCCESS_OR_TERMINATE(zeEventHostReset(wait_event));
    }

    Timer<std::chrono::nanoseconds::period> timer;
    std::vector<Timer<std::chrono::nanoseconds::period>> timers(
        benchmark->_devices.size());

    timer.start();
    for (uint32_t i = 0; i < number_iterations; i++) {
      for (auto device_id : device_ids) {
        benchmark->commandQueueExecuteCommandList(command_queue1[device_id], 1,
                                                  &command_list1[device_id]);
        benchmark->commandQueueExecuteCommandList(command_queue[device_id], 1,
                                                  &command_list[device_id]);
      }

      for (auto device_id : device_ids) {
        timers[device_id].start();
      }

      SUCCESS_OR_TERMINATE(zeEventHostSignal(wait_event));

      for (auto device_id : device_ids) {
        benchmark->commandQueueSynchronize(command_queue1[device_id]);
        benchmark->commandQueueSynchronize(command_queue[device_id]);

        timers[device_id].end();
        device_times_nsec[device_id] +=
            timers[device_id].period_minus_overhead();
      }

      SUCCESS_OR_TERMINATE(zeEventHostReset(wait_event));
    }
    timer.end();

    total_time_nsec = timer.period_minus_overhead();

    for (auto device_id : device_ids) {
      benchmark->commandListReset(command_list[device_id]);
      benchmark->commandListReset(command_list1[device_id]);
    }
  } else {
    // warm-up
    for (uint32_t i = 0; i < warmup_iterations; i++) {
      for (auto device_id : device_ids) {
        SUCCESS_OR_TERMINATE(zeEventHostReset(event[device_id]));
        SUCCESS_OR_TERMINATE(zeEventHostReset(event1[device_id]));
        SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
            command_list[device_id], destination_buffer[device_id],
            source_buffer[device_id], buffer_size, event[device_id], 1,
            &wait_event));
        SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
            command_list1[device_id], destination_buffer1[device_id],
            source_buffer1[device_id], buffer_size, event1[device_id], 1,
            &wait_event));
      }

      SUCCESS_OR_TERMINATE(zeEventHostSignal(wait_event));

      for (auto device_id : device_ids) {
        SUCCESS_OR_TERMINATE(
            zeEventHostSynchronize(event[device_id], UINT64_MAX));
        SUCCESS_OR_TERMINATE(
            zeEventHostSynchronize(event1[device_id], UINT64_MAX));
      }

      SUCCESS_OR_TERMINATE(zeEventHostReset(wait_event));
    }

    Timer<std::chrono::nanoseconds::period> timer;
    std::vector<Timer<std::chrono::nanoseconds::period>> timers(
        benchmark->_devices.size());

    timer.start();
    for (uint32_t i = 0; i < number_iterations; i++) {
      for (auto device_id : device_ids) {
        SUCCESS_OR_TERMINATE(zeEventHostReset(event[device_id]));
        SUCCESS_OR_TERMINATE(zeEventHostReset(event1[device_id]));
        SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
            command_list[device_id], destination_buffer[device_id],
            source_buffer[device_id], buffer_size, event[device_id], 1,
            &wait_event));
        SUCCESS_OR_TERMINATE(zeCommandListAppendMemoryCopy(
            command_list1[device_id], destination_buffer1[device_id],
            source_buffer1[device_id], buffer_size, event1[device_id], 1,
            &wait_event));
      }

      for (auto device_id : device_ids) {
        timers[device_id].start();
      }

      SUCCESS_OR_TERMINATE(zeEventHostSignal(wait_event));

      for (auto device_id : device_ids) {
        SUCCESS_OR_TERMINATE(
            zeEventHostSynchronize(event[device_id], UINT64_MAX));
        SUCCESS_OR_TERMINATE(
            zeEventHostSynchronize(event1[device_id], UINT64_MAX));

        timers[device_id].end();
        device_times_nsec[device_id] +=
            timers[device_id].period_minus_overhead();
      }

      SUCCESS_OR_TERMINATE(zeEventHostReset(wait_event));
    }
    timer.end();

    total_time_nsec = timer.period_minus_overhead();
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
    std::vector<long double> device_times_nsec(benchmark->_devices.size());
    for (int i = 0; i < device_times_nsec.size(); i++) {
      device_times_nsec[i] = 0;
    }

    for (auto device_id : device_ids) {
      benchmark->memoryAlloc(device_id, size, &device_buffers[device_id]);
      benchmark->memoryAllocHost(size, &host_buffers[device_id]);
    }

    transfer_size_test(size, device_buffers, host_buffers, device_times_nsec,
                       total_time_nsec);

    std::cout << "-----------------------------------------------------"
                 "---------------------------\n";
    std::cout << "Host->Device\n";
    for (auto device_id : device_ids) {
      benchmark->memoryFree(device_buffers[device_id]);
      benchmark->memoryFree(host_buffers[device_id]);

      calculate_metrics(device_times_nsec[device_id],
                        static_cast<long double>(size * number_iterations),
                        total_bandwidth, total_latency);
      print_results(size, total_bandwidth, total_latency,
                    "\t[Device " + std::to_string(device_id) + " ");
    }

    calculate_metrics(
        total_time_nsec,
        static_cast<long double>(device_ids.size() * size * number_iterations),
        total_bandwidth, total_latency);
    print_results(size * device_ids.size(), total_bandwidth, total_latency,
                  "[Total    ");
    std::cout << "-----------------------------------------------------"
                 "---------------------------\n";
  }
}

void ZeBandwidth::test_device2host(void) {
  long double total_bandwidth = 0.0;
  long double total_latency = 0.0;

  std::cout << std::endl;
  std::cout << "DEVICE-TO-HOST BANDWIDTH AND LATENCY" << std::endl;
  if (csv_output) {
    std::cout << "Transfer_size,Bandwidth_(GBPS),Latency_(usec)" << std::endl;
  }

  for (auto size : transfer_size) {
    long double total_time_nsec = 0;
    std::vector<long double> device_times_nsec(benchmark->_devices.size());
    for (int i = 0; i < device_times_nsec.size(); i++) {
      device_times_nsec[i] = 0;
    }

    for (auto device_id : device_ids) {
      benchmark->memoryAlloc(size, &device_buffers[device_id]);
      benchmark->memoryAllocHost(size, &host_buffers[device_id]);
    }

    transfer_size_test(size, host_buffers, device_buffers, device_times_nsec,
                       total_time_nsec);

    std::cout << "-----------------------------------------------------"
                 "---------------------------\n";
    std::cout << "Device->Host\n";
    for (auto device_id : device_ids) {
      benchmark->memoryFree(device_buffers[device_id]);
      benchmark->memoryFree(host_buffers[device_id]);

      calculate_metrics(device_times_nsec[device_id],
                        static_cast<long double>(size * number_iterations),
                        total_bandwidth, total_latency);
      print_results(size, total_bandwidth, total_latency,
                    "\t[Device " + std::to_string(device_id) + " ");
    }

    calculate_metrics(
        total_time_nsec,
        static_cast<long double>(device_ids.size() * size * number_iterations),
        total_bandwidth, total_latency);
    print_results(size * device_ids.size(), total_bandwidth, total_latency,
                  "[Total    ");
    std::cout << "-----------------------------------------------------"
                 "---------------------------\n";
  }
}

void ZeBandwidth::test_bidir(void) {
  long double total_bandwidth = 0.0;
  long double total_latency = 0.0;

  std::cout << std::endl;
  std::cout
      << "BIDIRECTIONAL HOST-TO-DEVICE/DEVICE-TO-HOST BANDWIDTH AND LATENCY"
      << std::endl;
  if (csv_output) {
    std::cout << "Transfer_size,Bandwidth_(GBPS),Latency_(usec)" << std::endl;
  }

  for (auto size : transfer_size) {
    long double total_time_nsec = 0;
    std::vector<long double> device_times_nsec(benchmark->_devices.size());
    for (int i = 0; i < device_times_nsec.size(); i++) {
      device_times_nsec[i] = 0;
    }

    for (auto device_id : device_ids) {
      benchmark->memoryAlloc(size, &device_buffers[device_id]);
      benchmark->memoryAllocHost(size, &host_buffers[device_id]);
      benchmark->memoryAlloc(size, &device_buffers_bidir[device_id]);
      benchmark->memoryAllocHost(size, &host_buffers_bidir[device_id]);
    }

    transfer_bidir_size_test(size, device_buffers, host_buffers, host_buffers_bidir,
                             device_buffers_bidir, device_times_nsec,
                             total_time_nsec);

    std::cout << "-----------------------------------------------------"
                 "---------------------------\n";
    std::cout << "Host<->Device\n";
    for (auto device_id : device_ids) {
      benchmark->memoryFree(device_buffers[device_id]);
      benchmark->memoryFree(host_buffers[device_id]);
      benchmark->memoryFree(device_buffers_bidir[device_id]);
      benchmark->memoryFree(host_buffers_bidir[device_id]);

      calculate_metrics(device_times_nsec[device_id],
                        static_cast<long double>(2 * size * number_iterations),
                        total_bandwidth, total_latency);
      print_results(size, total_bandwidth, total_latency,
                    "\t[Device " + std::to_string(device_id) + " ");
    }

    calculate_metrics(total_time_nsec,
                      static_cast<long double>(device_ids.size() * 2 * size *
                                               number_iterations),
                      total_bandwidth, total_latency);
    print_results(size * device_ids.size(), total_bandwidth, total_latency,
                  "[Total    ");
    std::cout << "-----------------------------------------------------"
                 "---------------------------\n";
  }
}

//---------------------------------------------------------------------
// Utility function to query queue group properties
//---------------------------------------------------------------------
void ZeBandwidth::ze_bandwidth_query_engines() {

  for (auto device_id : device_ids) {
    uint32_t numQueueGroups = 0;
    benchmark->deviceGetCommandQueueGroupProperties(device_id, &numQueueGroups,
                                                    nullptr);

    if (numQueueGroups == 0) {
      std::cout << " No queue groups found\n" << std::endl;
      exit(0);
    }

    queueProperties.resize(numQueueGroups);
    for (auto queueProperty : queueProperties) {
      queueProperty = {ZE_STRUCTURE_TYPE_COMMAND_QUEUE_GROUP_PROPERTIES,
                       nullptr};
    }
    benchmark->deviceGetCommandQueueGroupProperties(device_id, &numQueueGroups,
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
        std::cout << " Group " << i
                  << " (copy): " << queueProperties[i].numQueues << " queues\n";
      }
    }

    if (command_queue_group_ordinal >= numQueueGroups) {
      command_queue_group_ordinal = 0;
      std::cout << "Specified command queue group "
                << command_queue_group_ordinal
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
      std::cout << "Specified command queue index "
                << command_queue_group_ordinal << " is not valid for group "
                << command_queue_group_ordinal << ", defaulting to first index"
                << std::endl;
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
      std::cout << "[Device " << device_id << "] Running H2D with "
                << command_queue_group_ordinal << "," << command_queue_index
                << "\n";
    }
    if (run_dev2host) {
      std::cout << "[Device " << device_id << "] Running D2H with "
                << command_queue_group_ordinal << "," << command_queue_index
                << "\n";
    }
    if (run_bidirectional) {
      std::cout << "[Device " << device_id << "] Running H2D with "
                << command_queue_group_ordinal << "," << command_queue_index
                << " and "
                << "D2H with " << command_queue_group_ordinal1 << ","
                << command_queue_index1 << "\n";
    }

    if (use_immediate_command_list == false) {
      benchmark->commandQueueCreate(device_id, command_queue_group_ordinal,
                                    command_queue_index,
                                    &command_queue[device_id]);
      benchmark->commandListCreate(device_id, command_queue_group_ordinal,
                                   &command_list[device_id]);

      benchmark->commandQueueCreate(device_id, command_queue_group_ordinal1,
                                    command_queue_index1,
                                    &command_queue1[device_id]);
      benchmark->commandListCreate(device_id, command_queue_group_ordinal1,
                                   &command_list1[device_id]);
    } else {
      ze_command_queue_desc_t command_queue_description{};
      command_queue_description.stype = ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC;
      command_queue_description.pNext = nullptr;
      command_queue_description.ordinal = command_queue_group_ordinal;
      command_queue_description.index = command_queue_index;
      command_queue_description.mode = ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS;

      SUCCESS_OR_TERMINATE(zeCommandListCreateImmediate(
          benchmark->context, benchmark->_devices[device_id],
          &command_queue_description, &command_list[device_id]));

      command_queue_description.ordinal = command_queue_group_ordinal1;
      command_queue_description.index = command_queue_index1;
      SUCCESS_OR_TERMINATE(zeCommandListCreateImmediate(
          benchmark->context, benchmark->_devices[device_id],
          &command_queue_description, &command_list1[device_id]));
    }
  }

  if (verify) {
    benchmark->commandQueueCreate(0, 0, 0, &command_queue_verify);
    benchmark->commandListCreate(0, 0, &command_list_verify);
  }

  ze_event_pool_desc_t event_pool_desc = {ZE_STRUCTURE_TYPE_EVENT_POOL_DESC};
  event_pool_desc.count = 2 * device_ids.size() + 1;
  event_pool_desc.flags = ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
  SUCCESS_OR_TERMINATE(zeEventPoolCreate(
      benchmark->context, &event_pool_desc, benchmark->_devices.size(),
      benchmark->_devices.data(), &event_pool));

  ze_event_desc_t event_desc = {ZE_STRUCTURE_TYPE_EVENT_DESC};
  event_desc.signal = ZE_EVENT_SCOPE_FLAG_DEVICE;
  event_desc.wait = ZE_EVENT_SCOPE_FLAG_HOST;

  uint32_t event_index = 0;
  for (auto device_id : device_ids) {
    event_desc.index = event_index++;
    SUCCESS_OR_TERMINATE(
        zeEventCreate(event_pool, &event_desc, &event[device_id]));
    event_desc.index = event_index++;
    SUCCESS_OR_TERMINATE(
        zeEventCreate(event_pool, &event_desc, &event1[device_id]));
  }

  event_desc.index = event_index;
  event_desc.signal = ZE_EVENT_SCOPE_FLAG_HOST;
  event_desc.wait = ZE_EVENT_SCOPE_FLAG_DEVICE;
  SUCCESS_OR_TERMINATE(zeEventCreate(event_pool, &event_desc, &wait_event));
}

int main(int argc, char **argv) {
  ZeBandwidth bw;
  size_t default_size;
  srand(1);

  bw.parse_arguments(argc, argv);

  bw.command_queue.resize(bw.benchmark->_devices.size());
  bw.command_queue1.resize(bw.benchmark->_devices.size());
  bw.command_list.resize(bw.benchmark->_devices.size());
  bw.command_list1.resize(bw.benchmark->_devices.size());
  bw.event.resize(bw.benchmark->_devices.size());
  bw.event1.resize(bw.benchmark->_devices.size());

  if (bw.device_ids.empty()) {
    bw.device_ids.push_back(0);
  }

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

    bw.device_buffers.resize(bw.benchmark->_devices.size());
    bw.device_buffers_bidir.resize(bw.benchmark->_devices.size());
    bw.host_buffers.resize(bw.benchmark->_devices.size());
    bw.host_buffers_bidir.resize(bw.benchmark->_devices.size());

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
