/*
 *
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

kernel void test_device_memory1(__global uint *out) {
    size_t tid = get_global_id(0);
    out[tid] = out[tid] + tid;
}
