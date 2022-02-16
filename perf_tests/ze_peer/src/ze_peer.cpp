/*
 *
 * Copyright (C) 2019-2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */
#include "ze_peer.h"

bool run_using_all_compute_engines = false;
bool run_using_all_copy_engines = false;

void print_results_header(uint32_t remote_device_id, uint32_t local_device_id,
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

  std::cout << test_type_string << " " << transfer_type_string << " : "
            << "Device(" << local_device_id << ")" << test_arrow << "Device("
            << remote_device_id << ")\n";

  if (test_type == PEER_BANDWIDTH) {
    std::cout << " Size   | BW [GBPS] " << std::endl;
  } else {
    std::cout << " Size   | Latency [us] " << std::endl;
  }
}

void run_ipc_test(size_t max_number_of_elements, int size_to_run,
                  const uint32_t command_queue_group_ordinal,
                  const uint32_t command_queue_index, uint32_t remote_device_id,
                  uint32_t local_device_id, bool bidirectional,
                  peer_test_t test_type, peer_transfer_t transfer_type,
                  bool validate, uint32_t *num_devices) {
  pid_t pid;
  int sv[2];
  if (socketpair(PF_UNIX, SOCK_STREAM, 0, sv) < 0) {
    perror("socketpair");
    exit(1);
  }

  print_results_header(remote_device_id, local_device_id, test_type,
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
        ZePeer peer(command_queue_group_ordinal, command_queue_index,
                    local_device_id, remote_device_id,
                    run_using_all_compute_engines, run_using_all_copy_engines,
                    num_devices);
        peer.bandwidth_latency_ipc(bidirectional, test_type, transfer_type,
                                   false /* is_server */, sv[1],
                                   number_of_elements, local_device_id,
                                   remote_device_id, validate);
      } else {
        ZePeer peer(command_queue_group_ordinal, command_queue_index,
                    local_device_id, remote_device_id,
                    run_using_all_compute_engines, run_using_all_copy_engines);
        peer.bandwidth_latency_ipc(bidirectional, test_type, transfer_type,
                                   true /* is_server */, sv[0],
                                   number_of_elements, remote_device_id,
                                   local_device_id, validate);
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

void run_test(size_t max_number_of_elements, int size_to_run,
              const uint32_t command_queue_group_ordinal,
              const uint32_t command_queue_index, uint32_t remote_device_id,
              uint32_t local_device_id, bool bidirectional,
              peer_test_t test_type, peer_transfer_t transfer_type,
              bool validate, uint32_t *num_devices) {
  print_results_header(remote_device_id, local_device_id, test_type,
                       transfer_type);

  for (int number_of_elements = 8; number_of_elements <= max_number_of_elements;
       number_of_elements *= 2) {
    if (size_to_run != -1) {
      number_of_elements = size_to_run;
    }
    ZePeer peer(command_queue_group_ordinal, command_queue_index,
                remote_device_id, local_device_id,
                run_using_all_compute_engines, run_using_all_copy_engines,
                num_devices);
    if (bidirectional) {
      peer.bidirectional_bandwidth_latency(test_type, transfer_type,
                                           number_of_elements, remote_device_id,
                                           local_device_id, validate);
    } else {
      if (run_using_all_compute_engines || run_using_all_copy_engines) {
        if (bidirectional) {
          std::cout
              << "All-engines tests only available for unidirectional test\n";
          std::terminate();
        }
        peer.bandwidth_latency_all_engines(test_type, transfer_type,
                                           number_of_elements, remote_device_id,
                                           local_device_id, validate);
      } else {
        peer.bandwidth_latency(test_type, transfer_type, number_of_elements,
                               remote_device_id, local_device_id, validate);
      }
    }
    if (size_to_run != -1) {
      break;
    }
  }
  std::cout << std::endl;
}

int main(int argc, char **argv) {
  bool run_all = false;
  bool run_ipc = false;
  bool run_bidirectional = false;
  uint32_t command_queue_group_ordinal = 0;
  uint32_t command_queue_index = 0;
  int src_device_id = -1;
  std::vector<uint32_t> dst_device_ids{};
  int size_to_run = -1;
  bool validate = false;
  peer_transfer_t transfer_type_to_run = PEER_TRANSFER_MAX;
  peer_test_t test_type_to_run = PEER_TEST_MAX;
  uint32_t num_devices = 32;

  size_t max_number_of_elements = 1024 * 1024 * 256; /* 256 MB */

  for (int i = 1; i < argc; i++) {
    if ((strcmp(argv[i], "-h") == 0) || (strcmp(argv[i], "--help") == 0)) {
      std::cout << usage_str;
      exit(0);
    } else if ((strcmp(argv[i], "-q") == 0)) {
      ZePeer peer(command_queue_group_ordinal, command_queue_index, 0, 1, false,
                  false);
      peer.query_engines();
      exit(0);
    } else if ((strcmp(argv[i], "-g") == 0)) {
      if (isdigit(argv[i + 1][0])) {
        command_queue_group_ordinal = atoi(argv[i + 1]);
      } else {
        std::cout << usage_str;
        exit(-1);
      }
      i++;
    } else if ((strcmp(argv[i], "-i") == 0)) {
      if (isdigit(argv[i + 1][0])) {
        command_queue_index = atoi(argv[i + 1]);
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
    } else if (strcmp(argv[i], "-m") == 0) {
      run_ipc = true;
    } else if (strcmp(argv[i], "-a") == 0) {
      run_all = true;
    } else if (strcmp(argv[i], "-b") == 0) {
      run_bidirectional = true;
    } else if (strcmp(argv[i], "-e") == 0) {
      run_using_all_compute_engines = true;
    } else if (strcmp(argv[i], "-l") == 0) {
      run_using_all_copy_engines = true;
    } else if (strcmp(argv[i], "-d") == 0) {
      std::string dst_device_ids_string = argv[i + 1];
      const std::string comma = ",";

      size_t pos = 0;
      size_t start = 0;
      std::string device_id_string = "";
      auto search_for_device_id = [&]() {
        if (isdigit(device_id_string[0])) {
          dst_device_ids.push_back(atoi(device_id_string.c_str()));
        } else {
          std::cout << usage_str;
          exit(-1);
        }
      };
      while ((pos = dst_device_ids_string.find(comma, start)) !=
             std::string::npos) {
        device_id_string = dst_device_ids_string.substr(start, pos);
        start = pos + 1;
        search_for_device_id();
      }
      device_id_string =
          dst_device_ids_string.substr(start, dst_device_ids_string.length());
      search_for_device_id();
      i++;
    } else if (strcmp(argv[i], "-s") == 0) {
      if (isdigit(argv[i + 1][0])) {
        src_device_id = atoi(argv[i + 1]);
      } else {
        std::cout << usage_str;
        exit(-1);
      }
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
      validate = true;
    } else {
      std::cout << usage_str;
      exit(-1);
    }
  }

  if (run_ipc) {
    std::cout << "============================================================="
                 "===================\n"
              << "IPC tests\n"
              << "============================================================="
                 "===================\n";
  } else {
    std::cout << "============================================================="
                 "===================\n"
              << "Single Process "
              << ((run_bidirectional == 0) ? "Unidirectional "
                                           : "Bidirectional ")
              << "tests\n"
              << "============================================================="
                 "===================\n";
  }

  for (uint32_t local_device_id = 0; local_device_id < num_devices;
       local_device_id++) {
    if (src_device_id != -1 && local_device_id != src_device_id) {
      continue;
    }
    for (uint32_t remote_device_id = 0; remote_device_id < num_devices;
         remote_device_id++) {
      if (local_device_id == remote_device_id) {
        continue;
      }
      if (!dst_device_ids.empty() &&
          std::find(dst_device_ids.begin(), dst_device_ids.end(),
                    remote_device_id) == dst_device_ids.end()) {
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
          if (run_ipc) {
            std::cout << "-----------------------------------------------------"
                         "---------------------------\n";
            run_ipc_test(max_number_of_elements, size_to_run,
                         command_queue_group_ordinal, command_queue_index,
                         remote_device_id, local_device_id,
                         false, // run_bidirectional
                         static_cast<peer_test_t>(test_type),
                         static_cast<peer_transfer_t>(transfer_type), validate,
                         &num_devices);
            std::cout << "-----------------------------------------------------"
                         "---------------------------\n";
          } else {
            std::cout << "-----------------------------------------------------"
                         "---------------------------\n";
            run_test(max_number_of_elements, size_to_run,
                     command_queue_group_ordinal, command_queue_index,
                     remote_device_id, local_device_id, run_bidirectional,
                     static_cast<peer_test_t>(test_type),
                     static_cast<peer_transfer_t>(transfer_type), validate,
                     &num_devices);
            std::cout << "-----------------------------------------------------"
                         "---------------------------\n";
          }
        }
      }
    }
  }

  return 0;
}
