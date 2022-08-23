/*
 *
 * Copyright (C) 2019-2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */
#include "ze_peer.h"

const std::string version = "2.0";

// Defaults
bool ZePeer::use_queue_in_destination = false;
bool ZePeer::run_continuously = false;
bool ZePeer::bidirectional = false;
bool ZePeer::validate_results = false;
bool ZePeer::parallel_copy_to_single_target = false;
bool ZePeer::parallel_copy_to_multiple_targets = false;
uint32_t ZePeer::number_iterations = 50;
const size_t max_number_of_elements = 268435456; /* 256 MB */

void stress_handler(int signal) { exit(1); }

void print_results_header(std::vector<uint32_t> remote_device_ids,
                          std::vector<uint32_t> local_device_ids,
                          peer_test_t test_type,
                          peer_transfer_t transfer_type) {
  std::string test_type_string = "Bandwidth";
  if (test_type == PEER_LATENCY) {
    test_type_string = "Latency";
  }

  std::string transfer_type_string = "Read";
  std::string test_arrow = "<---";
  if (transfer_type == PEER_WRITE) {
    transfer_type_string = "Write";
    test_arrow = "--->";
  }
  if (ZePeer::bidirectional) {
    test_arrow = "<--->";
  }

  std::stringstream output_stream;
  output_stream << test_type_string << " " << transfer_type_string << " : ";

  output_stream << "Device( ";
  for (auto local_device_id : local_device_ids) {
    output_stream << local_device_id << " ";
  }
  output_stream << ")" << test_arrow;

  output_stream << "Device( ";
  for (auto remote_device_id : remote_device_ids) {
    output_stream << remote_device_id << " ";
  }
  output_stream << ")\n";

  std::cout << output_stream.str();
}

void run_ipc_test(int size_to_run, uint32_t remote_device_id,
                  uint32_t local_device_id, uint32_t queue,
                  peer_test_t test_type, peer_transfer_t transfer_type) {

  if (ZePeer::bidirectional) {
    std::cerr << "[ERROR] Bidirectional mode with IPC tests not implemented\n";
    return;
  }

  pid_t pid;
  int sv[2];
  if (socketpair(PF_UNIX, SOCK_STREAM, 0, sv) < 0) {
    perror("socketpair");
    exit(1);
  }

  std::vector<uint32_t> remote_device_ids{remote_device_id};
  std::vector<uint32_t> local_device_ids{local_device_id};
  std::vector<uint32_t> queues{queue};
  print_results_header(remote_device_ids, local_device_ids, test_type,
                       transfer_type);

  for (int number_of_elements = 8; number_of_elements <= max_number_of_elements;
       number_of_elements *= 2) {
    if (size_to_run != -1) {
      number_of_elements = size_to_run;
    }
    pid = fork();
    if (pid == 0) {
      pid_t test_pid = fork();
      if (test_pid == 0) {
        ZePeer peer(local_device_ids, remote_device_ids, queues);
        if (ZePeer::validate_results) {
          peer.warm_up_iterations = 0;
          peer.number_iterations = 1;
        }

        peer.bandwidth_latency_ipc(
            test_type, transfer_type, false /* is_server */, sv[1],
            number_of_elements, local_device_id, remote_device_id);
      } else {
        ZePeer peer(local_device_ids, remote_device_ids, queues);
        if (ZePeer::validate_results) {
          peer.warm_up_iterations = 0;
          peer.number_iterations = 1;
        }

        peer.bandwidth_latency_ipc(
            test_type, transfer_type, true /* is_server */, sv[0],
            number_of_elements, remote_device_id, local_device_id);
      }
    } else {
      int child_status;
      pid_t child_pid = wait(&child_status);
      if (child_pid <= 0) {
        std::cerr << "Client terminated abruptly with error code "
                  << strerror(errno) << "\n";
        std::terminate();
      }
    }
    if (size_to_run != -1) {
      break;
    }
  }
  std::cout << std::endl;

  close(sv[0]);
  close(sv[1]);
}

void run_test(int size_to_run, std::vector<uint32_t> &remote_device_ids,
              std::vector<uint32_t> &local_device_ids,
              std::vector<uint32_t> &queues, peer_test_t test_type,
              peer_transfer_t transfer_type) {
  print_results_header(remote_device_ids, local_device_ids, test_type,
                       transfer_type);

  for (int number_of_elements = 8; number_of_elements <= max_number_of_elements;
       number_of_elements *= 2) {
    if (size_to_run != -1) {
      number_of_elements = size_to_run;
    }
    ZePeer peer(remote_device_ids, local_device_ids, queues);
    if (ZePeer::validate_results) {
      peer.warm_up_iterations = 0;
      peer.number_iterations = 1;
    }

    if (peer.run_continuously) {
      struct sigaction sigIntHandler;
      sigIntHandler.sa_handler = stress_handler;
      sigemptyset(&sigIntHandler.sa_mask);
      sigIntHandler.sa_flags = 0;
      sigaction(SIGINT, &sigIntHandler, NULL);
    }

    if (ZePeer::parallel_copy_to_multiple_targets == false &&
        ZePeer::parallel_copy_to_single_target == false) {
      if (ZePeer::bidirectional) {
        peer.bidirectional_bandwidth_latency(
            test_type, transfer_type, number_of_elements,
            remote_device_ids.front(), local_device_ids.front(),
            queues.front());
      } else {
        peer.bandwidth_latency(test_type, transfer_type, number_of_elements,
                               remote_device_ids.front(),
                               local_device_ids.front(), queues.front());
      }
    } else {
      if (ZePeer::parallel_copy_to_multiple_targets) {
        peer.bandwidth_latency_parallel_to_multiple_targets(
            test_type, transfer_type, number_of_elements, remote_device_ids,
            local_device_ids);
      } else if (ZePeer::parallel_copy_to_single_target) {
        if (remote_device_ids.size() > 1) {
          std::cerr << "[ERROR] Number of dst devices needs to be one "
                    << "for parallel_single_target test\n";
          return;
        }
        if (local_device_ids.size() > 1) {
          std::cerr << "[ERROR] Number of src devices needs to be one "
                    << "for parallel_single_target test\n";
          return;
        }
        if (queues.size() != 1 && queues.size() % 2) {
          std::cerr
              << "[ERROR] Number of engines passed with option -u needs to be "
              << "divisible by 2 for parallel_single_target test\n";
          return;
        }
        peer.bandwidth_latency_parallel_to_single_target(
            test_type, transfer_type, number_of_elements,
            remote_device_ids.front(), local_device_ids.front());
      }
    }
    if (size_to_run != -1) {
      break;
    }
  }
  std::cout << std::endl;
}

int main(int argc, char **argv) {
  bool run_ipc = false;
  std::vector<uint32_t> remote_device_ids{};
  std::vector<uint32_t> local_device_ids{};
  std::vector<uint32_t> queues{};
  int size_to_run = -1;
  peer_transfer_t transfer_type_to_run = PEER_TRANSFER_MAX;
  peer_test_t test_type_to_run = PEER_TEST_MAX;
  uint32_t num_devices = 32;

  auto parse_and_insert = [&](std::string &s,
                              std::vector<uint32_t> &vector_of_indexes) {
    if (isdigit(s[0])) {
      vector_of_indexes.push_back(atoi(s.c_str()));
    } else {
      std::cerr << usage_str;
      exit(-1);
    }
  };

  for (int i = 1; i < argc; i++) {
    if ((strcmp(argv[i], "-h") == 0) || (strcmp(argv[i], "--help") == 0)) {
      std::cout << usage_str;
      exit(0);
    } else if (strcmp(argv[i], "--version") == 0) {
      std::cout << "ze_peer v" << version.c_str() << "\n";
      exit(0);
    } else if ((strcmp(argv[i], "-q") == 0)) {
      ZePeer peer(&num_devices);
      peer.query_engines();
      exit(0);
    } else if ((strcmp(argv[i], "-i") == 0)) {
      if (isdigit(argv[i + 1][0])) {
        ZePeer::number_iterations = atoi(argv[i + 1]);
      } else {
        std::cout << usage_str;
        exit(-1);
      }
      i++;
    } else if ((strcmp(argv[i], "-t") == 0)) {
      if ((i + 1) >= argc) {
        std::cout << usage_str;
        exit(-1);
      }
      if (strcmp(argv[i + 1], "transfer_bw") == 0) {
        test_type_to_run = PEER_BANDWIDTH;
        i++;
      } else if (strcmp(argv[i + 1], "latency") == 0) {
        test_type_to_run = PEER_LATENCY;
        i++;
      } else {
        std::cout << usage_str;
        exit(-1);
      }
    } else if ((strcmp(argv[i], "-o") == 0)) {
      if ((i + 1) >= argc) {
        std::cout << usage_str;
        exit(-1);
      }
      if (strcmp(argv[i + 1], "read") == 0) {
        transfer_type_to_run = PEER_READ;
        i++;
      } else if (strcmp(argv[i + 1], "write") == 0) {
        transfer_type_to_run = PEER_WRITE;
        i++;
      } else {
        std::cout << usage_str;
        exit(-1);
      }
    } else if (strcmp(argv[i], "-c") == 0) {
      ZePeer::run_continuously = true;
    } else if (strcmp(argv[i], "-b") == 0) {
      ZePeer::bidirectional = true;
    } else if ((strcmp(argv[i], "--parallel_single_target") == 0)) {
      ZePeer::parallel_copy_to_single_target = true;
    } else if (strcmp(argv[i], "--parallel_multiple_targets") == 0) {
      ZePeer::parallel_copy_to_multiple_targets = true;
    } else if (strcmp(argv[i], "--ipc") == 0) {
      run_ipc = true;
    } else if ((strcmp(argv[i], "-x") == 0)) {
      if ((i + 1) >= argc) {
        std::cout << usage_str;
        exit(-1);
      }
      if (strcmp(argv[i + 1], "src") == 0) {
        ZePeer::use_queue_in_destination = false;
        i++;
      } else if (strcmp(argv[i + 1], "dst") == 0) {
        ZePeer::use_queue_in_destination = true;
        i++;
      } else {
        std::cout << usage_str;
        exit(-1);
      }
    } else if (strcmp(argv[i], "-d") == 0) {
      std::string remote_device_ids_string = argv[i + 1];
      const std::string comma = ",";

      size_t pos = 0;
      size_t start = 0;
      std::string device_id_string = "";
      while ((pos = remote_device_ids_string.find(comma, start)) !=
             std::string::npos) {
        device_id_string = remote_device_ids_string.substr(start, pos);
        start = pos + 1;
        parse_and_insert(device_id_string, remote_device_ids);
      }
      device_id_string = remote_device_ids_string.substr(
          start, remote_device_ids_string.length());
      parse_and_insert(device_id_string, remote_device_ids);
      i++;
    } else if (strcmp(argv[i], "-s") == 0) {
      std::string local_device_ids_string = argv[i + 1];
      const std::string comma = ",";

      size_t pos = 0;
      size_t start = 0;
      std::string device_id_string = "";
      while ((pos = local_device_ids_string.find(comma, start)) !=
             std::string::npos) {
        device_id_string = local_device_ids_string.substr(start, pos);
        start = pos + 1;
        parse_and_insert(device_id_string, local_device_ids);
      }
      device_id_string = local_device_ids_string.substr(
          start, local_device_ids_string.length());
      parse_and_insert(device_id_string, local_device_ids);
      i++;
    } else if (strcmp(argv[i], "-u") == 0) {
      std::string local_queues_string = argv[i + 1];
      const std::string comma = ",";

      size_t pos = 0;
      size_t start = 0;
      std::string queue_string = "";
      while ((pos = local_queues_string.find(comma, start)) !=
             std::string::npos) {
        queue_string = local_queues_string.substr(start, pos);
        start = pos + 1;
        parse_and_insert(queue_string, queues);
      }
      queue_string =
          local_queues_string.substr(start, local_queues_string.length());
      parse_and_insert(queue_string, queues);

      i++;
    } else if (strcmp(argv[i], "-z") == 0) {
      if (isdigit(argv[i + 1][0])) {
        size_to_run = atoi(argv[i + 1]);
      } else {
        std::cout << usage_str;
        exit(-1);
      }
      i++;
    } else if (strcmp(argv[i], "-v") == 0) {
      ZePeer::validate_results = true;
    } else {
      std::cout << usage_str;
      exit(-1);
    }
  }

  if (run_ipc == false) {
    // Detect number of devices
    ZePeer peerQueryDevices(&num_devices);
  }

  if (run_ipc) {
    std::cout << "============================================================="
                 "===================\n"
              << "IPC tests\n"
              << "============================================================="
                 "===================\n";
  }

  std::cout << "============================================================="
               "===================\n"
            << "Single Process "
            << ((ZePeer::bidirectional == 0) ? "Unidirectional "
                                             : "Bidirectional ")
            << "tests\n"
            << "============================================================="
               "===================\n";

  if (ZePeer::parallel_copy_to_multiple_targets ||
      ZePeer::parallel_copy_to_single_target) {

    // set default source devices
    if (local_device_ids.empty()) {
      local_device_ids.push_back(0u);
    }

    // set default destination devices
    if (remote_device_ids.empty()) {
      if (ZePeer::parallel_copy_to_single_target) {
        remote_device_ids.push_back(0u);
      } else {
        for (uint32_t remote_device_id = 0; remote_device_id < num_devices;
             remote_device_id++) {
          remote_device_ids.push_back(remote_device_id);
        }
      }
    }

    for (uint32_t test_type = 0;
         test_type < static_cast<uint32_t>(PEER_TEST_MAX); test_type++) {
      if (test_type_to_run != PEER_TEST_MAX &&
          static_cast<peer_test_t>(test_type) != test_type_to_run) {
        continue;
      }
      for (uint32_t transfer_type = 0;
           transfer_type < static_cast<uint32_t>(PEER_TRANSFER_MAX);
           transfer_type++) {
        if (transfer_type_to_run != PEER_TRANSFER_MAX &&
            static_cast<peer_transfer_t>(transfer_type) !=
                transfer_type_to_run) {
          continue;
        }
        std::cout << "-----------------------------------------------------"
                     "---------------------------\n";
        run_test(size_to_run, remote_device_ids, local_device_ids, queues,
                 static_cast<peer_test_t>(test_type),
                 static_cast<peer_transfer_t>(transfer_type));
        std::cout << "-----------------------------------------------------"
                     "---------------------------\n";
      }
    }
  } else {
    // set default source devices
    if (local_device_ids.empty()) {
      for (uint32_t local_device_id = 0; local_device_id < num_devices;
           local_device_id++) {
        local_device_ids.push_back(local_device_id);
      }
    }

    // set default destination devices
    if (remote_device_ids.empty()) {
      for (uint32_t remote_device_id = 0; remote_device_id < num_devices;
           remote_device_id++) {
        remote_device_ids.push_back(remote_device_id);
      }
    }

    // set default queue
    if (queues.empty()) {
      queues.push_back(0);
    }

    uint32_t current_queue_index = 0;

    for (auto local_device_id : local_device_ids) {
      for (auto remote_device_id : remote_device_ids) {
        if (local_device_id == remote_device_id) {
          continue;
        }
        for (uint32_t test_type = 0;
             test_type < static_cast<uint32_t>(PEER_TEST_MAX); test_type++) {
          if (test_type_to_run != PEER_TEST_MAX &&
              static_cast<peer_test_t>(test_type) != test_type_to_run) {
            continue;
          }
          for (uint32_t transfer_type = 0;
               transfer_type < static_cast<uint32_t>(PEER_TRANSFER_MAX);
               transfer_type++) {
            if (transfer_type_to_run != PEER_TRANSFER_MAX &&
                static_cast<peer_transfer_t>(transfer_type) !=
                    transfer_type_to_run) {
              continue;
            }
            std::cout << "-----------------------------------------------------"
                         "---------------------------\n";
            if (run_ipc) {
              run_ipc_test(size_to_run, remote_device_id, local_device_id,
                           queues[current_queue_index %
                                  static_cast<uint32_t>(queues.size())],
                           static_cast<peer_test_t>(test_type),
                           static_cast<peer_transfer_t>(transfer_type));
            } else {
              std::vector<uint32_t> tmp_remote_device_ids{remote_device_id};
              std::vector<uint32_t> tmp_local_device_ids{local_device_id};
              std::vector<uint32_t> tmp_queues{
                  queues[current_queue_index %
                         static_cast<uint32_t>(queues.size())]};
              run_test(size_to_run, tmp_remote_device_ids, tmp_local_device_ids,
                       tmp_queues, static_cast<peer_test_t>(test_type),
                       static_cast<peer_transfer_t>(transfer_type));
            }
            std::cout << "-----------------------------------------------------"
                         "---------------------------\n";
          }
        }
        current_queue_index++;
      }
    }
  }

  return 0;
}

ZePeer::ZePeer(uint32_t *num_devices) {

  benchmark = new ZeApp();

  this->device_count = benchmark->allDevicesInit();
  if (num_devices) {
    *num_devices = device_count;
  }

  if (this->device_count <= 1) {
    std::cerr << "ERROR: More than 1 device needed" << std::endl;
    std::terminate();
  }
}

ZePeer::ZePeer(std::vector<uint32_t> &remote_device_ids,
               std::vector<uint32_t> &local_device_ids,
               std::vector<uint32_t> &queues) {

  benchmark = new ZeApp();

  this->device_count = benchmark->allDevicesInit();

  if (this->device_count <= 1) {
    std::cerr << "ERROR: More than 1 device needed" << std::endl;
    std::terminate();
  }

  for (auto dst_device_id : remote_device_ids) {
    for (auto src_device_id : local_device_ids) {
      if (benchmark->_devices.size() <
          std::max(dst_device_id + 1, src_device_id + 1)) {
        std::cout << "ERROR: Number for source or destination device not "
                  << "available: Only " << benchmark->_devices.size()
                  << " devices "
                  << "detected" << std::endl;
        std::terminate();
      }

      if (!benchmark->canAccessPeer(dst_device_id, src_device_id)) {
        std::cerr << "Devices " << src_device_id << " and " << dst_device_id
                  << " do not have P2P capabilities " << std::endl;
        std::terminate();
      }
    }
  }

  ze_peer_devices.resize(benchmark->_devices.size());

  ze_buffers.resize(benchmark->_devices.size());
  ze_src_buffers.resize(benchmark->_devices.size());
  ze_dst_buffers.resize(benchmark->_devices.size());
  ze_ipc_buffers.resize(benchmark->_devices.size());

  this->queues = queues;

  for (uint32_t d = 0; d < benchmark->_devices.size(); d++) {
    uint32_t numQueueGroups = 0;
    benchmark->deviceGetCommandQueueGroupProperties(d, &numQueueGroups,
                                                    nullptr);

    if (numQueueGroups == 0) {
      std::cout << " No queue groups found\n" << std::endl;
      std::terminate();
    }

    std::vector<ze_command_queue_group_properties_t> queueProperties(
        numQueueGroups);
    for (auto queueProperty : queueProperties) {
      queueProperty = {ZE_STRUCTURE_TYPE_COMMAND_QUEUE_GROUP_PROPERTIES,
                       nullptr};
    }
    benchmark->deviceGetCommandQueueGroupProperties(d, &numQueueGroups,
                                                    queueProperties.data());

    bool option_u_empty = this->queues.empty();
    uint32_t engineIndex = 0;
    for (uint32_t g = 0; g < numQueueGroups; g++) {
      for (uint32_t q = 0; q < queueProperties[g].numQueues; q++) {
        ze_command_queue_handle_t command_queue;
        benchmark->commandQueueCreate(d, g, q, &command_queue);

        ze_command_list_handle_t command_list;
        benchmark->commandListCreate(d, g, &command_list);

        auto enginePair = std::make_pair(command_queue, command_list);
        ze_peer_devices[d].engines.push_back(enginePair);

        // use compute engines by default. Select the indexes from device 0
        if (option_u_empty && (queueProperties[g].flags &
                               ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE)) {
          this->queues.push_back(engineIndex);
        }

        engineIndex++;
      }
    }
  }

  ze_event_pool_desc_t event_pool_desc = {ZE_STRUCTURE_TYPE_EVENT_POOL_DESC};
  event_pool_desc.count = 1;
  event_pool_desc.flags = {};
  SUCCESS_OR_TERMINATE(zeEventPoolCreate(benchmark->context, &event_pool_desc,
                                         0, nullptr, &event_pool));

  ze_event_desc_t event_desc = {ZE_STRUCTURE_TYPE_EVENT_DESC};
  event_desc.signal = ZE_EVENT_SCOPE_FLAG_HOST;
  SUCCESS_OR_TERMINATE(zeEventCreate(event_pool, &event_desc, &event));
}

ZePeer::~ZePeer() {
  for (auto &device : ze_peer_devices) {
    for (auto enginePair : device.engines) {
      benchmark->commandQueueDestroy(enginePair.first);
      benchmark->commandListDestroy(enginePair.second);
    }
  }
  if (event) {
    SUCCESS_OR_TERMINATE(zeEventDestroy(event));
    SUCCESS_OR_TERMINATE(zeEventPoolDestroy(event_pool));
  }
  benchmark->allDevicesCleanup();
  delete benchmark;
}
