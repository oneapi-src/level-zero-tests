/*
 *
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

kernel void simple_test(__global uint *src, __global uint *dst) {
    size_t  tid = get_global_id(0);
    dst[tid] = src[tid];
}
