/*
 *
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

kernel void memory_add(global int* result, global int* source, int value) {
    const int gid = get_global_id(0);
    result[gid] = source[gid] + value;
}

kernel void memory_atomic_add(global int* result, global int* source, int value) {
    const int gid = get_global_id(0);
    atomic_add(&result[gid], source[gid] + value);
}
