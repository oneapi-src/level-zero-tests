/*
 *
 * Copyright (C) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

ulong __spirv_SpecConstant(int, ulong);

kernel void test(__global ulong *output) {
    *output = __spirv_SpecConstant(0, 10) + __spirv_SpecConstant(1, 30) + __spirv_SpecConstant(2, 50) + __spirv_SpecConstant(3, 70);
}
