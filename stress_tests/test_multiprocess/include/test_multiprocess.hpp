/*
 *
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include <array>

// Shared-memory layout used to collect per-child success flags.
// Supports up to 1024 child processes.
using ChildResults = std::array<int, 1024>;
