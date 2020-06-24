/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

typedef struct _SobelKernel {
    uint upper_left;
    uint upper_middle;
    uint upper_right;
    uint middle_left;
    uint middle;
    uint middle_right;
    uint lower_left;
    uint lower_middle;
    uint lower_right;
} SobelKernel;

int compute_horizontal_derivative(const SobelKernel p) {
    return -p.upper_left - 2 * p.middle_left - p.lower_left + p.upper_right +
        2 * p.middle_right + p.lower_right;
}

int compute_vertical_derivative(const SobelKernel p) {
    return -p.upper_left - 2 * p.upper_middle - p.upper_right + p.lower_left +
        2 * p.lower_middle + p.lower_right;
}

int compute_gradient(SobelKernel p) {
    float h = (float)compute_horizontal_derivative(p);
    float v = (float)compute_vertical_derivative(p);
    return (uint)sqrt(h*h + v*v);
}

bool is_boundary(const int x, const int y, const int width, const int height) {
    return (x < 1) || (x >= (width - 1)) || (y < 1) || (y >= (height - 1));
}

kernel void sobel(global uint *inputBuffer, global uint *outputBuffer, int width, int height) {
    const int x = get_global_id(0);
    const int y = get_global_id(1);
    const int offset = y * width + x;

    if (is_boundary(x, y, width, height)) {
        outputBuffer[offset] = inputBuffer[offset];
        return;
    }

    SobelKernel pixels;
    pixels.middle = inputBuffer[offset];
    pixels.middle_left = inputBuffer[offset - 1];
    pixels.middle_right = inputBuffer[offset + 1];
    pixels.upper_left = inputBuffer[offset - 1 - width];
    pixels.upper_middle = inputBuffer[offset - width];
    pixels.upper_right = inputBuffer[offset + 1 - width];
    pixels.lower_left = inputBuffer[offset - 1 + width];
    pixels.lower_middle = inputBuffer[offset + width];
    pixels.lower_right = inputBuffer[offset + 1 + width];

    int g = compute_gradient(pixels);
    uint pixel = g < 0 ? 0 : g > 255 ? 255 : g;
    outputBuffer[offset] = pixel;
}
