/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

/*
 * The test cases are generated using macros and #include's to facilitate
 * code resuse when creating nano probes.  Each test case is written once
 * and imported per nano probe such as the latency and hardware counter
 * probes.
 *
 * Each test case is written with the NANO_PROBE() macro around the
 * function call to be measured. When each test case is imported,
 * the NANO_PROBE() is redefined to a type of probe, so that the
 * test case is generated for that specific probe.
 *
 * When a test case is added, benchmark.hpp should be updated.
 */

#include "benchmark.hpp"

#include "api_static_probe.hpp"

namespace ze_api_benchmarks {
/*
 * For each test case created, the NANO_PROBE macro needs to be redefined to
 * generate code with the specific probe.
 */
#undef NANO_PROBE
#define NANO_PROBE PROBE_MEASURE_LATENCY_ITERATION
namespace latency {
#include "benchmark_template/command_list.cpp"
#include "benchmark_template/ipc.cpp"
#include "benchmark_template/set_parameter.cpp"
} /* namespace latency */

#undef NANO_PROBE
#define NANO_PROBE PROBE_MEASURE_HARDWARE_COUNTERS
namespace hardware_counter {
#include "benchmark_template/command_list.cpp"
#include "benchmark_template/ipc.cpp"
#include "benchmark_template/set_parameter.cpp"
} /* namespace hardware_counter */

#undef NANO_PROBE
#define NANO_PROBE PROBE_MEASURE_FUNCTION_CALL_RATE
namespace fuction_call_rate {
#include "benchmark_template/command_list.cpp"
#include "benchmark_template/ipc.cpp"
#include "benchmark_template/set_parameter.cpp"
} /* namespace fuction_call_rate */
} /* namespace ze_api_benchmarks */
