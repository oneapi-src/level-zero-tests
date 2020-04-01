/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef _BENCHMARK_HPP_
#define _BENCHMARK_HPP_

#include "api_static_probe.hpp"
#include "ze_app.hpp"

namespace ze_api_benchmarks {
inline void init() { api_static_probe_init(); }
inline void cleanup() { api_static_probe_cleanup(); }

namespace latency {
#include "benchmark_template/command_list.hpp"
#include "benchmark_template/ipc.hpp"
#include "benchmark_template/set_parameter.hpp"
} /* namespace latency */
namespace hardware_counter {
#include "benchmark_template/command_list.hpp"
#include "benchmark_template/ipc.hpp"
#include "benchmark_template/set_parameter.hpp"
} /* namespace hardware_counter */
namespace fuction_call_rate {
#include "benchmark_template/command_list.hpp"
#include "benchmark_template/ipc.hpp"
#include "benchmark_template/set_parameter.hpp"
} /* namespace fuction_call_rate */
} /* namespace ze_api_benchmarks */

#endif /* _BENCHMARK_HPP_ */
