/*
 *
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

int export_function_add(int x, int y);

kernel void import_function(int x, int y, __global int* result) {
    result[0] = export_function_add(x,y);
}