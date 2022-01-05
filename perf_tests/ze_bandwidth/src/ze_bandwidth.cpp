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
    benchmark->commandListDestroy(command_list_verify);
    benchmark->commandListDestroy(command_list);
    benchmark->commandQueueDestroy(command_queue);
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
  total_data_transfer /= static_cast<long double>(1e9); /* Units in Gigabytes */

  total_bandwidth = total_data_transfer / total_time_s;
  total_latency = total_time_nsec / (1e3 * number_iterations);
}

void ZeBandwidth::print_results_host2device(size_t buffer_size,
                                            long double total_bandwidth,
                                            long double total_latency) {
  if (csv_output) {
    std::cout << buffer_size << "," << std::setprecision(6) << total_bandwidth
              << "," << std::setprecision(2) << total_latency << std::endl;
  } else {
    std::cout << "Host->Device[" << std::fixed << std::setw(10) << buffer_size
              << "]:  BW = " << std::setw(9) << std::setprecision(6)
              << total_bandwidth << " GBPS  Latency = " << std::setw(9)
              << std::setprecision(2) << total_latency << " usec" << std::endl;
  }
}

void ZeBandwidth::print_results_device2host(size_t buffer_size,
                                            long double total_bandwidth,
                                            long double total_latency) {
  if (csv_output) {
    std::cout << buffer_size << "," << std::setprecision(6) << total_bandwidth
              << "," << std::setprecision(2) << total_latency << std::endl;
  } else {
    std::cout << "Device->Host[" << std::fixed << std::setw(10) << buffer_size
              << "]:  BW = " << std::setw(9) << std::setprecision(6)
              << total_bandwidth << " GBPS  Latency = " << std::setw(9)
              << std::setprecision(2) << total_latency << " usec" << std::endl;
  }
}

void ZeBandwidth::measure_transfer_verify(size_t buffer_size,
                                          uint32_t num_transfer,
                                          long double &host2dev_time_nsec,
                                          long double &dev2host_time_nsec) {
  Timer<std::chrono::nanoseconds::period> timer;

  uint8_t *xmt = static_cast<uint8_t *>(host_buffer);
  uint8_t *rcv = static_cast<uint8_t *>(host_buffer_verify);
  host2dev_time_nsec = 0.0;
  dev2host_time_nsec = 0.0;

  for (uint32_t i = 0; i < num_transfer; i++) {
    xmt[0] = rand() & 0xff;
    xmt[buffer_size - 1] = rand() & 0xff;

    timer.start();
    benchmark->commandQueueExecuteCommandList(command_queue, 1, &command_list);
    benchmark->commandQueueSynchronize(command_queue);
    timer.end();
    host2dev_time_nsec += timer.period_minus_overhead();

    timer.start();
    benchmark->commandQueueExecuteCommandList(command_queue, 1,
                                              &command_list_verify);
    benchmark->commandQueueSynchronize(command_queue);
    timer.end();
    dev2host_time_nsec += timer.period_minus_overhead();

    // Comparing all transfer bytes results in significant increase in
    // execution time
    //(likely due to cache invalidation and flushing effects)
    // To minimize this penalty only verify first and last byte of transfer.
    if ((xmt[0] != rcv[0]) || (xmt[buffer_size - 1] != rcv[buffer_size - 1])) {
      throw std::runtime_error("Host memory verification failed ");
    }
  }
}

long double ZeBandwidth::measure_transfer(uint32_t num_transfer) {
  Timer<std::chrono::nanoseconds::period> timer;

  timer.start();
  for (uint32_t i = 0; i < num_transfer; i++) {
    benchmark->commandQueueExecuteCommandList(command_queue, 1, &command_list);
    benchmark->commandQueueSynchronize(command_queue);
  }
  timer.end();

  return timer.period_minus_overhead();
}

void ZeBandwidth::transfer_size_test_verify(size_t size,
                                            long double &host2dev_time_nsec,
                                            long double &dev2host_time_nsec) {
  size_t element_size = sizeof(uint8_t);
  size_t buffer_size = element_size * size;

  benchmark->commandListAppendMemoryCopy(command_list, device_buffer,
                                         host_buffer, buffer_size);
  benchmark->commandListClose(command_list);

  benchmark->commandListAppendMemoryCopy(
      command_list_verify, host_buffer_verify, device_buffer, buffer_size);
  benchmark->commandListClose(command_list_verify);

  measure_transfer_verify(buffer_size, number_iterations, host2dev_time_nsec,
                          dev2host_time_nsec);

  benchmark->commandListReset(command_list);
  benchmark->commandListReset(command_list_verify);
}

void ZeBandwidth::transfer_size_test(size_t size, void *destination_buffer,
                                     void *source_buffer,
                                     long double &total_time_nsec) {
  size_t element_size = sizeof(uint8_t);
  size_t buffer_size = element_size * size;
  long double total_time_s;
  long double total_data_transfer;

  benchmark->commandListAppendMemoryCopy(command_list, destination_buffer,
                                         source_buffer, buffer_size);
  benchmark->commandListClose(command_list);
  total_time_nsec = measure_transfer(number_iterations);
  benchmark->commandListReset(command_list);
}

void ZeBandwidth::test_host2device(void) {
  long double total_bandwidth = 0.0;
  long double total_latency = 0.0;

  std::cout << std::endl;
  if (verify) {
    std::cout << "HOST-TO-DEVICE BANDWIDTH AND LATENCY WITH VERIFICATION"
              << std::endl;
    if (csv_output) {
      std::cout << "Transfer_size,Bandwidth_(GBPS),Latency_(usec)" << std::endl;
    }
    for (auto size : transfer_size) {
      long double host2dev_time_nsec;
      long double dev2host_time_nsec;
      long double total_bandwidth;
      long double total_latency;

      benchmark->memoryAlloc(size, &device_buffer);
      benchmark->memoryAllocHost(size, &host_buffer);
      benchmark->memoryAllocHost(size, &host_buffer_verify);

      transfer_size_test_verify(size, host2dev_time_nsec, dev2host_time_nsec);

      benchmark->memoryFree(device_buffer);
      benchmark->memoryFree(host_buffer);
      benchmark->memoryFree(host_buffer_verify);

      calculate_metrics(host2dev_time_nsec,
                        static_cast<long double>(size * number_iterations),
                        total_bandwidth, total_latency);
      print_results_host2device(size, total_bandwidth, total_latency);
    }
  } else {
    std::cout << "HOST-TO-DEVICE BANDWIDTH AND LATENCY" << std::endl;
    if (csv_output) {
      std::cout << "Transfer_size,Bandwidth_(GBPS),Latency_(usec)" << std::endl;
    }
    for (auto size : transfer_size) {
      long double total_time_nsec;

      benchmark->memoryAlloc(size, &device_buffer);
      benchmark->memoryAllocHost(size, &host_buffer);

      transfer_size_test(size, device_buffer, host_buffer, total_time_nsec);

      benchmark->memoryFree(device_buffer);
      benchmark->memoryFree(host_buffer);

      calculate_metrics(total_time_nsec,
                        static_cast<long double>(size * number_iterations),
                        total_bandwidth, total_latency);
      print_results_host2device(size, total_bandwidth, total_latency);
    }
  }
}

void ZeBandwidth::test_device2host(void) {
  long double total_bandwidth;
  long double total_latency;

  std::cout << std::endl;
  if (verify) {
    std::cout << "DEVICE-TO-HOST BANDWIDTH AND LATENCY WITH VERIFICATION"
              << std::endl;
    if (csv_output) {
      std::cout << "Transfer_size,Bandwidth_(GBPS),Latency_(usec)" << std::endl;
    }
    for (auto size : transfer_size) {
      long double host2dev_time_nsec;
      long double dev2host_time_nsec;
      long double total_bandwidth;
      long double total_latency;

      benchmark->memoryAlloc(size, &device_buffer);
      benchmark->memoryAllocHost(size, &host_buffer);
      benchmark->memoryAllocHost(size, &host_buffer_verify);

      transfer_size_test_verify(size, host2dev_time_nsec, dev2host_time_nsec);

      benchmark->memoryFree(device_buffer);
      benchmark->memoryFree(host_buffer);
      benchmark->memoryFree(host_buffer_verify);

      calculate_metrics(dev2host_time_nsec,
                        static_cast<long double>(size * number_iterations),
                        total_bandwidth, total_latency);
      print_results_device2host(size, total_bandwidth, total_latency);
    }
  } else {
    std::cout << "DEVICE-TO-HOST BANDWIDTH AND LATENCY" << std::endl;
    if (csv_output) {
      std::cout << "Transfer_size,Bandwidth_(GBPS),Latency_(usec)" << std::endl;
    }
    for (auto size : transfer_size) {
      long double total_time_nsec;

      benchmark->memoryAlloc(size, &device_buffer);
      benchmark->memoryAllocHost(size, &host_buffer);

      transfer_size_test(size, host_buffer, device_buffer, total_time_nsec);

      benchmark->memoryFree(device_buffer);
      benchmark->memoryFree(host_buffer);

      calculate_metrics(total_time_nsec,
                        static_cast<long double>(size * number_iterations),
                        total_bandwidth, total_latency);
      print_results_device2host(size, total_bandwidth, total_latency);
    }
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

  if (enable_fixed_ordinal_index) {
    if (command_queue_group_ordinal >= numQueueGroups) {
      std::cout << "Specified command queue group "
                << command_queue_group_ordinal
                << " is not valid, defaulting to first group" << std::endl;
      benchmark->commandQueueCreate(0, &command_queue);
      benchmark->commandListCreate(&command_list);
      benchmark->commandListCreate(&command_list_verify);
    } else if (queueProperties[command_queue_group_ordinal].flags &
               ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE) {
      std::cout << "The ordinal provided matches COMPUTE engine, so "
                   "fixed ordinal will be used\n";
      if (command_queue_index >=
          queueProperties[command_queue_group_ordinal].numQueues) {
        command_queue_index = 0;
      }
      benchmark->commandQueueCreate(0, command_queue_group_ordinal,
                                    command_queue_index, &command_queue);
      benchmark->commandListCreate(0, command_queue_group_ordinal,
                                   &command_list);
      benchmark->commandListCreate(0, command_queue_group_ordinal,
                                   &command_list_verify);

    } else if ((queueProperties[command_queue_group_ordinal].flags &
                ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COPY) &&
               !(queueProperties[command_queue_group_ordinal].flags &
                 ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE)) {
      std::cout << "The ordinal provided matches COPY engine, so "
                   "fixed ordinal will be used\n";
      if (command_queue_index >=
          queueProperties[command_queue_group_ordinal].numQueues) {
        command_queue_index = 0;
      }
      benchmark->commandQueueCreate(0, command_queue_group_ordinal,
                                    command_queue_index, &command_queue);
      benchmark->commandListCreate(0, command_queue_group_ordinal,
                                   &command_list);
      benchmark->commandListCreate(0, command_queue_group_ordinal,
                                   &command_list_verify);
    } else {
      std::cout << "The ordinal provided neither matches COMPUTE nor COPY "
                   "engine, so disabling fixed dispatch\n";
      benchmark->commandQueueCreate(0, &command_queue);
      benchmark->commandListCreate(&command_list);
      benchmark->commandListCreate(&command_list_verify);
    }
    std::cout << std::endl;
  } else {
    benchmark->commandQueueCreate(0, &command_queue);
    benchmark->commandListCreate(&command_list);
    benchmark->commandListCreate(&command_list_verify);
  }
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

    if (bw.run_host2dev) {
      bw.test_host2device();
    }

    if (bw.run_dev2host) {
      bw.test_device2host();
    }

    std::cout << std::endl;

    std::cout << std::flush;
  }
  return 0;
}
