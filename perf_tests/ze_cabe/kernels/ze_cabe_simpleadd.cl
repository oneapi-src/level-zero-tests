/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

__kernel void NaiveAdd(int n, global int *a, global int *b) {
    for(int i = 0; i < n; ++i) 
    {
        b[i] = a[i] + 1;
    }
}
