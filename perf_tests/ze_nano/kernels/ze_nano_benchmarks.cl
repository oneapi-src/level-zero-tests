/*
 *
 * Copyright (C) 2019-2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

kernel void function_parameter_buffers(global char *input_a,
                                       global char *input_b,
                                       global char *input_c,
                                       global char *input_d,
                                       global char *input_e,
                                       global char *input_f) {
}

kernel void function_parameter_integer(int a, int b, int c, int e, int f,
                                       int g) {
}

kernel void function_parameter_image(image2d_t input_a, image2d_t input_b,
                                     image2d_t input_c, image2d_t input_d,
                                     image2d_t input_e, image2d_t input_f) {
}

kernel void function_no_parameter() {
}
