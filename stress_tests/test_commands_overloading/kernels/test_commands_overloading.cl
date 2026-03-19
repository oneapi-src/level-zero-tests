/*
 *
 * Copyright (C) 2021-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma extension cl_khr_int64_base_atomics: enable
#pragma extension cl_khr_int64_extended_atomics: enable

kernel void test_device_memory1(__global uint *out) {
    size_t tid = get_global_id(0);
    out[tid] = out[tid] + tid;
}

kernel void test_submissions(__global volatile atomic_ulong *counter) {
    atomic_fetch_add_explicit(counter, (ulong)1,
                              memory_order_relaxed,
                              memory_scope_device);
}