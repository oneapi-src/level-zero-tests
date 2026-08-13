/*
 *
 * Copyright (C) 2019-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "ze_bandwidth.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <string_view>

static const char *usage_str =
    "\n ze_bandwidth [OPTIONS]"
    "\n"
    "\n OPTIONS:"
    "\n  -t, string               run a particular test [default:h2d, d2h]:"
    "\n      h2d or H2D                       run only Host-to-Device tests"
    "\n      d2h or D2H                       run only Device-to-Host tests "
    "\n      bidir                            run only bidirectional tests "
    "\n      bidir_h2d                        bidirectional, report Host-to-"
    "Device only, "
    "\n                                       interfering copy runs at twice "
    "the size"
    "\n      bidir_d2h                        bidirectional, report Device-to-"
    "Host only, "
    "\n                                       interfering copy runs at twice "
    "the size"
    "\n      h2d_kernel                       Host-to-Device driven by a "
    "compute kernel"
    "\n      d2h_kernel                       Device-to-Host driven by a "
    "compute kernel"
    "\n      bidir_kernel                     bidirectional through a single "
    "kernel that "
    "\n                                       alternates direction across "
    "stripes"
    "\n      all_to_host                      Device-to-Host on one device "
    "while every "
    "\n                                       other device interferes, "
    "reported per device"
    "\n      host_to_all                      Host-to-Device on one device "
    "while every "
    "\n                                       other device interferes, "
    "reported per device"
    "\n      all_to_host_bidir                as all_to_host, with "
    "bidirectional traffic"
    "\n      host_to_all_bidir                as host_to_all, with "
    "bidirectional traffic"
    "\n      all_to_host_kernel               kernel driven variants of the "
    "above;"
    "\n      host_to_all_kernel               these default to every device, "
    "override"
    "\n      host_to_all_bidir_kernel         with -d. the split kernel is "
    "symmetric,"
    "\n                                       so there is no all_to_host "
    "variant of it"
    "\n  -v                       enable verification"
    "\n                            [default:  disabled]"
    "\n  -i                       set number of iterations per transfer"
    "\n                            [default:  500]"
    "\n  -w                       set number of warmup iterations"
    "\n                            [default:  100]"
    "\n  -s                       select only one transfer size (bytes) "
    "\n  -sb                      select beginning transfer size (bytes)"
    "\n                            [default:  1]"
    "\n  -se                      select ending transfer size (bytes)"
    "\n                            [default: 2^28]"
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
    "\n  --h2dEngine name         engine used for the Host-to-Device direction,"
    "\n                            e.g. bcs0, bcs1, ccs0. mutually exclusive "
    "with -g / -n"
    "\n  --d2hEngine name         engine used for the Device-to-Host direction,"
    "\n                            e.g. bcs0, bcs1, ccs0. mutually exclusive "
    "with -g / -n"
    "\n  --useEvents              measure with GPU timestamps instead of the "
    "host timer"
    "\n                            [default:  host timer]"
    "\n  --immediate              use immediate command lists (default: "
    "disabled)"
    "\n  --csv                    output in csv format (default: disabled)"
    "\n  -h, --help               display help message"
    "\n";

//---------------------------------------------------------------------
// One parser for every numeric argument. strtoul reported a non-numeric
// input as zero without setting errno, so "-i abc" ran zero iterations and
// reported nan, and "-i 12abc" silently became 12. Anything that is not a
// whole valid number is now rejected where it is given.
//---------------------------------------------------------------------
static uint32_t parse_u32(const std::string &value) {
  try {
    size_t parsed = 0;
    const unsigned long number = std::stoul(value, &parsed, 0);

    if (parsed != value.size() || number > UINT32_MAX) {
      throw std::out_of_range("not an unsigned 32-bit integer");
    }

    return static_cast<uint32_t>(number);
  } catch (const std::exception &) {
    std::cerr << "'" << value << "' is not a valid unsigned 32-bit integer\n";
    exit(-1);
  }
}

//---------------------------------------------------------------------
// Splits a comma separated argument. An empty token is rejected rather than
// parsed as zero, so a leading, repeated or trailing comma fails. The
// previous hand written split passed an absolute position where a length was
// expected, which only worked because atoi stopped at the comma.
//---------------------------------------------------------------------
static std::vector<std::string> split_csv(const std::string &value) {
  std::vector<std::string> tokens;
  size_t begin = 0;

  while (true) {
    const size_t end = value.find(',', begin);
    const std::string token = value.substr(begin, end - begin);

    if (token.empty()) {
      std::cerr << "Empty token in comma separated argument '" << value
                << "'\n";
      exit(-1);
    }

    tokens.push_back(token);
    if (end == std::string::npos) {
      return tokens;
    }

    begin = end + 1;
  }
}

//---------------------------------------------------------------------
// Utility function which parses the arguments to ze_peak and
// sets the test parameters accordingly for main to execute the tests
// with the correct environment.
//---------------------------------------------------------------------
int ZeBandwidth::parse_arguments(int argc, char **argv) {
  const std::vector<std::string> args(argv + 1, argv + argc);

  auto require_value = [&](size_t index) {
    if ((index + 1) >= args.size()) {
      std::cout << usage_str;
      exit(-1);
    }
  };

  auto parse_and_insert = [&](const std::string &token,
                              std::vector<uint32_t> &vector_of_indexes) {
    const uint32_t device_index = parse_u32(token);

    if (device_index >= benchmark->_devices.size()) {
      std::cerr << "Device " << device_index
                << " does not exist, the host enumerates 0 to "
                << (benchmark->_devices.size() - 1) << "\n";
      exit(-1);
    }

    // A repeated id would allocate twice into the same buffer slot, leaking
    // the first allocation and freeing the second one twice.
    if (std::find(vector_of_indexes.begin(), vector_of_indexes.end(),
                  device_index) != vector_of_indexes.end()) {
      std::cerr << "Device " << device_index
                << " listed more than once, ignoring the duplicate\n";
      return;
    }

    vector_of_indexes.push_back(device_index);
  };

  // The engine slots take one value shared by both directions, or two given
  // as a pair. Any other count is a request the tool cannot honour.
  auto parse_engine_pair = [&](const std::string &value, const char *option,
                               uint32_t &first, uint32_t &second) {
    const std::vector<std::string> tokens = split_csv(value);

    if (tokens.size() > 2) {
      std::cerr << option << " takes one value for both directions or two for "
                << "Host-to-Device and Device-to-Host, but " << tokens.size()
                << " were given\n";
      exit(-1);
    }

    first = parse_u32(tokens[0]);
    second = (tokens.size() == 2) ? parse_u32(tokens[1]) : first;
  };

  // Accepts both "--option value" and "--option=value".
  auto parse_named_value = [&](std::string_view option, size_t &index,
                               std::string &value) {
    const std::string &argument = args[index];

    if (argument == option) {
      require_value(index);
      value = args[++index];
      return true;
    }

    const std::string prefix = std::string(option) + "=";
    if (argument.starts_with(prefix)) {
      value = argument.substr(prefix.size());
      if (value.empty()) {
        std::cout << usage_str;
        exit(-1);
      }
      return true;
    }

    return false;
  };

  for (size_t i = 0; i < args.size(); i++) {
    const std::string &argument = args[i];

    if (argument == "-h" || argument == "--help") {
      std::cout << usage_str;
      exit(0);
    } else if (argument == "-v") {
      verify = true;
    } else if (argument == "-i") {
      require_value(i);
      number_iterations = parse_u32(args[++i]);
    } else if (argument == "-w") {
      require_value(i);
      warmup_iterations = parse_u32(args[++i]);
    } else if (argument == "-s") {
      require_value(i);
      transfer_lower_limit = parse_u32(args[++i]);
      transfer_upper_limit = transfer_lower_limit;
    } else if (argument == "-sb") {
      require_value(i);
      transfer_lower_limit = parse_u32(args[++i]);
    } else if (argument == "-se") {
      require_value(i);
      transfer_upper_limit = parse_u32(args[++i]);
    } else if (argument == "-q") {
      query_engines = true;
    } else if (parse_named_value("--h2dEngine", i, h2d_engine_name)) {
      enable_engine_names = true;
    } else if (parse_named_value("--d2hEngine", i, d2h_engine_name)) {
      enable_engine_names = true;
    } else if (argument == "--useEvents") {
      use_event_timer = true;
    } else if (argument == "--csv") {
      csv_output = true;
    } else if (argument == "--immediate") {
      use_immediate_command_list = true;
    } else if (argument == "-g") {
      require_value(i);
      enable_fixed_ordinal_index = true;
      parse_engine_pair(args[++i], "-g", command_queue_group_ordinal,
                        command_queue_group_ordinal1);
    } else if (argument == "-n") {
      require_value(i);
      enable_fixed_ordinal_index = true;
      parse_engine_pair(args[++i], "-n", command_queue_index,
                        command_queue_index1);
    } else if (argument == "-d") {
      require_value(i);
      for (const std::string &token : split_csv(args[++i])) {
        parse_and_insert(token, device_ids);
      }
      device_ids_explicit = true;
    } else if (argument == "-t") {
      require_value(i);
      const std::string test = args[++i];

      // Every selector is cleared, not only the three original ones, so that
      // a repeated -t replaces the previous choice instead of adding to it.
      run_host2dev = false;
      run_dev2host = false;
      run_bidirectional = false;
      run_bidir_h2d = false;
      run_bidir_d2h = false;
      run_host2dev_kernel = false;
      run_dev2host_kernel = false;
      run_bidirectional_kernel = false;
      run_all_host = false;

      if (test == "h2d" || test == "H2D") {
        run_host2dev = true;
      } else if (test == "d2h" || test == "D2H") {
        run_dev2host = true;
      } else if (test == "bidir") {
        run_bidirectional = true;
      } else if (test == "bidir_h2d") {
        run_bidir_h2d = true;
      } else if (test == "bidir_d2h") {
        run_bidir_d2h = true;
      } else if (test == "h2d_kernel") {
        run_host2dev_kernel = true;
      } else if (test == "d2h_kernel") {
        run_dev2host_kernel = true;
      } else if (test == "bidir_kernel") {
        run_bidirectional_kernel = true;
      } else if (test == "all_to_host" || test == "all_to_host_kernel" ||
                 test == "all_to_host_bidir" || test == "host_to_all" ||
                 test == "host_to_all_kernel" || test == "host_to_all_bidir" ||
                 test == "host_to_all_bidir_kernel") {
        run_all_host = true;
        all_host_source_is_host = test.starts_with("host_to_all");
        all_host_bidirectional = (test.find("_bidir") != std::string::npos);
        all_host_use_kernel = test.ends_with("_kernel");
      } else if (test == "all_to_host_bidir_kernel") {
        std::cerr << "all_to_host_bidir_kernel was removed: the split "
                     "direction kernel is symmetric, so it measured exactly "
                     "what host_to_all_bidir_kernel measures. Use that "
                     "instead.\n";
        exit(-1);
      } else {
        std::cout << usage_str;
        exit(-1);
      }
    } else {
      std::cout << usage_str;
      exit(-1);
    }
  }

  if (transfer_lower_limit == 0 || transfer_upper_limit == 0) {
    std::cerr << "Transfer size must be greater than zero\n";
    exit(-1);
  }

  if (transfer_lower_limit > transfer_upper_limit) {
    std::cerr << "Transfer size range is inverted: -sb " << transfer_lower_limit
              << " is greater than -se " << transfer_upper_limit
              << ", so the range would collapse to a single size\n";
    exit(-1);
  }

  // test_all_host closes, executes and resets its command lists, none of
  // which is valid on an immediate command list.
  if (run_all_host && use_immediate_command_list) {
    std::cerr << "--immediate is not supported by the multi device tests\n";
    exit(-1);
  }

  if (verify && (run_bidirectional || run_bidir_h2d || run_bidir_d2h)) {
    std::cerr << "NOTE: the bidirectional copy tests do not verify their "
                 "transfers, -v only reduces the iteration count\n";
  }

  if (enable_engine_names && enable_fixed_ordinal_index) {
    std::cerr << "--h2dEngine/--d2hEngine cannot be combined with -g or -n\n";
    std::cout << usage_str;
    exit(-1);
  }

  return 0;
}
