/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "ze_bandwidth.hpp"

static const char *usage_str =
    "\n ze_bandwidth [OPTIONS]"
    "\n"
    "\n OPTIONS:"
    "\n  -t, string               run a particular test [default:h2d, d2h]:"
    "\n      h2d or H2D                       run only Host-to-Device tests"
    "\n      d2h or D2H                       run only Device-to-Host tests "
    "\n      bidir                            run only bidirectional tests "
    "\n  -v                       enable verification"
    "\n                            [default:  disabled]"
    "\n  -i                       set number of iterations per transfer"
    "\n                            [default:  500]"
    "\n  -w                       set number of warmup iterations"
    "\n                            [default:  10]"
    "\n  -s                       select only one transfer size (bytes) "
    "\n  -sb                      select beginning transfer size (bytes)"
    "\n                            [default:  1]"
    "\n  -se                      select ending transfer size (bytes)"
    "\n                            [default: 2^30]"
    "\n  -q                       query for number of engines available"
    "\n  -d                       comma separated list of devices for "
    "\n                            parallel h2d/d2h tests (default: 0)"
    "\n  -g, group                select engine group (default: 0)."
    "\n                            when using bidir tests, a comma-separated "
    "list "
    "\n                            of engine groups may be passed, for h2d and "
    "d2h"
    "\n  -n, number               select engine index (default: 0)"
    "\n                            when using bidir tests, a comma-separated "
    "list "
    "\n                            of engine groups may be passed, for h2d and "
    "d2h"
    "\n  --immediate              use immediate command lists (default: "
    "disabled)"
    "\n  --csv                    output in csv format (default: disabled)"
    "\n  -h, --help               display help message"
    "\n";

template <typename T> inline constexpr uint32_t to_u32(T val) {
  return static_cast<uint32_t>(val);
}
template <> inline uint32_t to_u32<const char *>(const char *str) {
  return static_cast<uint32_t>(atoi(str));
}
template <> inline uint32_t to_u32<char *>(char *str) {
  return static_cast<uint32_t>(atoi(str));
}
static uint32_t sanitize_ulong(char *in) {
  unsigned long temp = strtoul(in, NULL, 0);
  if (ERANGE == errno) {
    fprintf(stderr, "%s out of range of type ulong\n", in);
  } else if (temp > UINT32_MAX) {
    fprintf(stderr, "%ld greater than UINT32_MAX\n", temp);
  } else {
    return to_u32(temp);
  }
  return 0;
}

//---------------------------------------------------------------------
// Utility function which parses the arguments to ze_peak and
// sets the test parameters accordingly for main to execute the tests
// with the correct environment.
//---------------------------------------------------------------------
int ZeBandwidth::parse_arguments(int argc, char **argv) {
  auto parse_and_insert = [&](std::string &s,
                              std::vector<uint32_t> &vector_of_indexes) {
    if (isdigit(s[0])) {
      vector_of_indexes.push_back(to_u32(s.c_str()));
    } else {
      std::cerr << usage_str;
      exit(-1);
    }
  };

  for (int i = 1; i < argc; i++) {
    if ((strcmp(argv[i], "-h") == 0) || (strcmp(argv[i], "--help") == 0)) {
      std::cout << usage_str;
      exit(0);
    } else if (strcmp(argv[i], "-v") == 0) {
      verify = true;
    } else if (strcmp(argv[i], "-i") == 0) {
      if ((i + 1) < argc) {
        number_iterations = sanitize_ulong(argv[i + 1]);
        i++;
      }
    } else if (strcmp(argv[i], "-w") == 0) {
      if ((i + 1) < argc) {
        warmup_iterations = sanitize_ulong(argv[i + 1]);
        i++;
      }
    } else if (strcmp(argv[i], "-s") == 0) {
      if ((i + 1) < argc) {
        transfer_lower_limit = sanitize_ulong(argv[i + 1]);
        transfer_upper_limit = transfer_lower_limit;
        i++;
      }
    } else if (strcmp(argv[i], "-sb") == 0) {
      if ((i + 1) < argc) {
        transfer_lower_limit = sanitize_ulong(argv[i + 1]);
        i++;
      }
    } else if (strcmp(argv[i], "-se") == 0) {
      if ((i + 1) < argc) {
        transfer_upper_limit = sanitize_ulong(argv[i + 1]);
        i++;
      }
    } else if ((strcmp(argv[i], "-q") == 0)) {
      query_engines = true;
      i++;
    } else if ((strcmp(argv[i], "-g") == 0)) {
      enable_fixed_ordinal_index = true;
      std::string queues_string = argv[i + 1];
      const std::string comma = ",";

      size_t pos = 0;
      size_t start = 0;
      std::string queue_string = "";
      if ((pos = queues_string.find(comma, start)) != std::string::npos) {
        queue_string = queues_string.substr(start, pos);
        start = pos + 1;
        command_queue_group_ordinal = to_u32(queue_string.c_str());
        queue_string = queues_string.substr(start, queues_string.length());
        command_queue_group_ordinal1 = to_u32(queue_string.c_str());
      } else {
        command_queue_group_ordinal = to_u32(argv[i + 1]);
        command_queue_group_ordinal1 = command_queue_group_ordinal;
      }
      i++;
    } else if ((strcmp(argv[i], "--csv") == 0)) {
      csv_output = true;
    } else if ((strcmp(argv[i], "--immediate") == 0)) {
      use_immediate_command_list = true;
    } else if ((strcmp(argv[i], "-n") == 0)) {
      enable_fixed_ordinal_index = true;
      std::string queues_string = argv[i + 1];
      const std::string comma = ",";

      size_t pos = 0;
      size_t start = 0;
      std::string queue_string = "";
      if ((pos = queues_string.find(comma, start)) != std::string::npos) {
        queue_string = queues_string.substr(start, pos);
        start = pos + 1;
        command_queue_index = to_u32(queue_string.c_str());
        queue_string = queues_string.substr(start, queues_string.length());
        command_queue_index1 = to_u32(queue_string.c_str());
      } else {
        command_queue_index = to_u32(argv[i + 1]);
        command_queue_index1 = command_queue_index;
      }
      i++;
    } else if (strcmp(argv[i], "-d") == 0) {
      std::string list_device_ids_string = argv[i + 1];
      const std::string comma = ",";

      size_t pos = 0;
      size_t start = 0;
      std::string device_id_string = "";
      while ((pos = list_device_ids_string.find(comma, start)) !=
             std::string::npos) {
        device_id_string = list_device_ids_string.substr(start, pos);
        start = pos + 1;
        parse_and_insert(device_id_string, device_ids);
      }
      device_id_string =
          list_device_ids_string.substr(start, list_device_ids_string.length());
      parse_and_insert(device_id_string, device_ids);
      i++;
    } else if ((strcmp(argv[i], "-t") == 0)) {
      run_host2dev = true;
      run_dev2host = true;
      run_bidirectional = true;

      if ((i + 1) >= argc) {
        std::cout << usage_str;
        exit(-1);
      }
      if ((strcmp(argv[i + 1], "h2d") == 0) ||
          (strcmp(argv[i + 1], "H2D") == 0)) {
        run_host2dev = true;
        run_dev2host = false;
        run_bidirectional = false;
        i++;
      } else if ((strcmp(argv[i + 1], "d2h") == 0) ||
                 (strcmp(argv[i + 1], "D2H") == 0)) {
        run_host2dev = false;
        run_dev2host = true;
        run_bidirectional = false;
        i++;
      } else if ((strcmp(argv[i + 1], "bidir") == 0)) {
        run_host2dev = false;
        run_dev2host = false;
        run_bidirectional = true;
        i++;
      } else {
        std::cout << usage_str;
        exit(-1);
      }
    } else {
      std::cout << usage_str;
      exit(-1);
    }
  }
  return 0;
}
