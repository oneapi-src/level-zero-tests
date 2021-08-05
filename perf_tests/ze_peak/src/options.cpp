/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "../include/ze_peak.h"

static const char *usage_str =
    "\n ze_peak [OPTIONS]"
    "\n"
    "\n OPTIONS:"
    "\n  -r, --driver num            choose driver (num starts with 0)"
    "\n  -d, --device num            choose device   (num starts with 0)"
    "\n  -e                          time using ze events instead of std "
    "chrono timer"
    "\n                              hide driver latencies [default: No]"
    "\n  -t, string                  selectively run a particular test"
    "\n      global_bw               selectively run global bandwidth test"
    "\n      hp_compute              selectively run half precision compute "
    "test"
    "\n      sp_compute              selectively run single precision compute "
    "test"
    "\n      dp_compute              selectively run double precision compute "
    "test"
    "\n      int_compute             selectively run integer compute test"
    "\n      transfer_bw             selectively run transfer bandwidth test"
    "\n      kernel_lat              selectively run kernel latency test"
    "\n  -a                          run all above tests [default]"
    "\n  -v                          enable verbose prints"
    "\n  -i                          set number of iterations to run[default: "
    "50]"
    "\n  -w                          set number of warmup iterations to "
    "run[default: 10]"
    "\n  -x                          enable explicit scaling [default: "
    "Disabled]"
    "\n  -q                          query for number of engines available"
    "\n  -g, group                   select engine group (default: 0)"
    "\n  -n, number                  select engine index (default: 0)"
    "\n  -h, --help                  display help message"
    "\n";

static uint32_t sanitize_ulong(char *in) {
  unsigned long temp = strtoul(in, NULL, 0);
  if (ERANGE == errno) {
    fprintf(stderr, "%s out of range of type ulong\n", in);
  } else if (temp > UINT32_MAX) {
    fprintf(stderr, "%ld greater than UINT32_MAX\n", temp);
  } else {
    return static_cast<uint32_t>(temp);
  }
  return 0;
}

//---------------------------------------------------------------------
// Utility function which parses the arguments to ze_peak and
// sets the test parameters accordingly for main to execute the tests
// with the correct environment.
//---------------------------------------------------------------------
int ZePeak::parse_arguments(int argc, char **argv) {
  for (int i = 1; i < argc; i++) {
    if ((strcmp(argv[i], "-h") == 0) || (strcmp(argv[i], "--help") == 0)) {
      std::cout << usage_str;
      exit(0);
    } else if ((strcmp(argv[i], "-r") == 0) ||
               (strcmp(argv[i], "--driver") == 0)) {
      if ((i + 1) < argc) {
        specified_driver = sanitize_ulong(argv[i + 1]);
        i++;
      }
    } else if ((strcmp(argv[i], "-d") == 0) ||
               (strcmp(argv[i], "--device") == 0)) {
      if ((i + 1) < argc) {
        specified_device = sanitize_ulong(argv[i + 1]);
        i++;
      }
    } else if (strcmp(argv[i], "-e") == 0) {
      use_event_timer = true;
    } else if (strcmp(argv[i], "-v") == 0) {
      verbose = true;
    } else if (strcmp(argv[i], "-i") == 0) {
      if ((i + 1) < argc) {
        iters = sanitize_ulong(argv[i + 1]);
        i++;
      }
    } else if (strcmp(argv[i], "-w") == 0) {
      if ((i + 1) < argc) {
        warmup_iterations = sanitize_ulong(argv[i + 1]);
        i++;
      }
    } else if ((strcmp(argv[i], "-t") == 0)) {
      run_global_bw = false;
      run_hp_compute = false;
      run_sp_compute = false;
      run_dp_compute = false;
      run_int_compute = false;
      run_transfer_bw = false;
      run_kernel_lat = false;
      if ((i + 1) >= argc) {
        std::cout << usage_str;
        exit(-1);
      }
      if (strcmp(argv[i + 1], "global_bw") == 0) {
        run_global_bw = true;
        i++;
      } else if (strcmp(argv[i + 1], "hp_compute") == 0) {
        run_hp_compute = true;
        i++;
      } else if (strcmp(argv[i + 1], "sp_compute") == 0) {
        run_sp_compute = true;
        i++;
      } else if (strcmp(argv[i + 1], "dp_compute") == 0) {
        run_dp_compute = true;
        i++;
      } else if (strcmp(argv[i + 1], "int_compute") == 0) {
        run_int_compute = true;
        i++;
      } else if (strcmp(argv[i + 1], "transfer_bw") == 0) {
        run_transfer_bw = true;
        i++;
      } else if (strcmp(argv[i + 1], "kernel_lat") == 0) {
        run_kernel_lat = true;
        i++;
      } else {
        std::cout << usage_str;
        exit(-1);
      }
    } else if (strcmp(argv[i], "-a") == 0) {
      run_global_bw = run_hp_compute = run_sp_compute = run_dp_compute =
          run_int_compute = run_transfer_bw = run_kernel_lat = true;
    } else if (strcmp(argv[i], "-x") == 0) {
      enable_explicit_scaling = true;
      i++;
    } else if ((strcmp(argv[i], "-q") == 0)) {
      query_engines = true;
      i++;
    } else if ((strcmp(argv[i], "-g") == 0)) {
      enable_fixed_ordinal_index = true;
      if (((i + 1) < argc) && isdigit(argv[i + 1][0])) {
        command_queue_group_ordinal = atoi(argv[i + 1]);
      }
      i++;
    } else if ((strcmp(argv[i], "-n") == 0)) {
      if (((i + 1) < argc) && isdigit(argv[i + 1][0])) {
        command_queue_index = atoi(argv[i + 1]);
      }
      i++;
    } else {
      std::cout << usage_str;
      exit(-1);
    }
  }
  return 0;
}
