/*
 *
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef _STRESS_MULTIPROCESS_HPP_
#define _STRESS_MULTIPROCESS_HPP_

#include <array>

// Shared-memory layout used to collect per-child success flags.
// Supports up to 1024 child processes.
using ChildResults = std::array<int, 1024>;

#endif // _STRESS_MULTIPROCESS_HPP_
