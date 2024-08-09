/*
 *
 * Copyright (C) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

ulong __spirv_SpecConstant(int, ulong);
ulong4 __spirv_SpecConstantComposite(ulong, ulong, ulong, ulong);

kernel void test(__global ulong4 *output) {
    ulong4 v = __spirv_SpecConstantComposite(
        __spirv_SpecConstant(0, 10),
        __spirv_SpecConstant(1, 20),
        __spirv_SpecConstant(2, 30),
        __spirv_SpecConstant(3, 40)
    );
    v += (ulong4)(1, 1, 1, 1);
    *output = v;
}
