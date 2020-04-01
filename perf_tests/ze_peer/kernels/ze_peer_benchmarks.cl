/*
 *
 * Copyright (C) 2019-2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

__kernel void single_copy_peer_to_peer(__global ulong *dest,
                                       __global ulong *src) {
    const int g_id = get_global_id(0);
    dest[g_id] = src[g_id];
}
