/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef COMPUTE_API_BENCH_UTILS_HPP
#define COMPUTE_API_BENCH_UTILS_HPP

#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include <vector>
#include <string.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include "logging/logging.hpp"
#include "image/image.hpp"

namespace compute_api_bench {

std::vector<uint8_t> load_binary_file(size_t &length,
                                      const std::string &file_path);
std::string load_text_file(size_t &length, const std::string &file_path);
void save_csv(const std::string &csv_string, const std::string &csv_filename);

static const std::string red = "\033[0;31m";
static const std::string green = "\033[1;32m";
static const std::string yellow = "\033[1;33m";
static const std::string blue = "\033[1;34m";
static const std::string magenta = "\033[0;35m";
static const std::string cyan = "\033[0;36m";
static const std::string white = "\033[0;37m";
static const std::string reset = "\033[0m";
static const std::string intense_white = "\033[0;97m";
static const std::string light_red = "\033[1;31m";

enum Stages {
  CREATE_DEVICE,
  BUILD_PROGRAM,
  CREATE_BUFFERS_CMDLIST,
  EXECUTE_WORK,
  COUNT
};

static std::string StagesList[Stages::COUNT] = {
    "Device Creation", "Kernel Compilation", "Buffer&CmdList Creation",
    "Work Execution"};

template <class T> class BlackScholesData {
public:
  BlackScholesData(){};
  BlackScholesData(unsigned int num_options) : num_options(num_options) {}
  ~BlackScholesData(){};

  inline T get_random(T low, T high) {
    T t = (T)rand() / (T)RAND_MAX;
    return (1.0f - t) * low + t * high;
  }

  void generate_data() {
    int buffer_size = num_options * sizeof(T);

    option_years.assign(buffer_size, 0);
    option_strike.assign(buffer_size, 0);
    stock_price.assign(buffer_size, 0);
    call_result.assign(buffer_size, 0);
    put_result.assign(buffer_size, 0);

    srand(5347);
    for (int i = 0; i < num_options; ++i) {
      option_years[i] = get_random(0.25f, 10.0f);
      option_strike[i] = get_random(1.0f, 100.0f);
      stock_price[i] = get_random(5.0f, 30.0f);
    }
  }

  unsigned int num_options;
  std::vector<T> option_years;
  std::vector<T> option_strike;
  std::vector<T> stock_price;
  std::vector<T> call_result;
  std::vector<T> put_result;
};

template <typename T> inline T cnd(T d) {
  const T A1 = 0.31938153;
  const T A2 = -0.356563782;
  const T A3 = 1.781477937;
  const T A4 = -1.821255978;
  const T A5 = 1.330274429;
  const T RSQRT2PI = 0.39894228040143267793994605993438;

  T K = 1.0 / (1.0 + 0.2316419 * fabs(d));

  T val = RSQRT2PI * exp(-0.5 * d * d) *
          (K * (A1 + K * (A2 + K * (A3 + K * (A4 + K * A5)))));

  if (d > 0.)
    val = 1.0 - val;

  return val;
}

template <typename T>
inline void black_scholes_cpu(const T riskfree, const T volatility, const T t,
                              const T x, const T s, T &call, T &put) {
  const T sqrt1_2 = 0.70710678118654752440084436210485;
  const T c_half = 0.5;

  T sqrtT = sqrt(t);
  T d1 = (log(s / x) + (riskfree + c_half * volatility * volatility) * t) /
         (volatility * sqrtT);
  T d2 = d1 - volatility * sqrtT;
  T CNDD1 = cnd(d1);
  T CNDD2 = cnd(d2);
  T expRT = exp(-riskfree * t);

  call = s * CNDD1 - x * expRT * CNDD2;
  put = call + expRT - s;
}

struct SobelKernel {
  uint32_t upper_left;
  uint32_t upper_middle;
  uint32_t upper_right;
  uint32_t middle_left;
  uint32_t middle;
  uint32_t middle_right;
  uint32_t lower_left;
  uint32_t lower_middle;
  uint32_t lower_right;
};

inline int compute_horizontal_derivative(const SobelKernel p) {
  return -p.upper_left - 2 * p.middle_left - p.lower_left + p.upper_right +
         2 * p.middle_right + p.lower_right;
}

inline int compute_vertical_derivative(const SobelKernel p) {
  return -p.upper_left - 2 * p.upper_middle - p.upper_right + p.lower_left +
         2 * p.lower_middle + p.lower_right;
}

inline int compute_gradient(SobelKernel p) {
  float h = (float)compute_horizontal_derivative(p);
  float v = (float)compute_vertical_derivative(p);
  return (uint32_t)sqrt(h * h + v * v);
}

inline bool is_boundary(const int x, const int y, const int width,
                        const int height) {
  return (x < 1) || (x >= (width - 1)) || (y < 1) || (y >= (height - 1));
}

inline void sobel_cpu(uint32_t *inputBuffer, uint32_t *outputBuffer,
                      const int width, const int height) {

  for (int x = 0; x < width; ++x) {
    for (int y = 0; y < height; ++y) {

      const int offset = y * width + x;

      if (is_boundary(x, y, width, height)) {
        outputBuffer[offset] = inputBuffer[offset];
        continue;
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
      uint32_t pixel = g < 0 ? 0 : g > 255 ? 255 : g;
      outputBuffer[offset] = pixel;
    }
  }
  return;
}

#define MAXITERATION 50

struct float2 {
  float x;
  float y;
};

inline float get_color(const uint32_t iterations,
                       const uint32_t max_iteration) {
  float pixel = 0.0f;
  if (iterations != max_iteration) {
    const float factor = sqrt((float)iterations / (float)max_iteration);
    const float raw_value = round((float)max_iteration * factor);
    pixel = (raw_value / (float)max_iteration);
  }
  return pixel;
}

inline void mandelbrot_cpu(float *pixels, int width, int height) {
  for (int x = 0; x < width; ++x) {
    for (int y = 0; y < height; ++y) {

      float2 uv = {(float)x / (float)width, (float)y / (float)height};
      float2 params = {1.75f, 1.25f};
      float2 c = {(uv.x * 2.5f) - params.x, (uv.y * 2.5f) - params.y};
      float2 z = {0.0f, 0.0f};

      int i;

      for (i = 0; i < MAXITERATION; ++i) {
        if ((z.x * z.x + z.y * z.y) > 2 * 2) {
          break;
        }
        const float tmp = z.x * z.x - z.y * z.y + c.x;
        z.y = 2 * z.x * z.y + c.y;
        z.x = tmp;
      }
      const int iterations = i;

      const uint32_t pixel_offset = y * width + x;
      pixels[pixel_offset] = get_color(iterations, MAXITERATION);
    }
  }
}

} // namespace compute_api_bench

#endif // COMPUTE_API_BENCH_UTILS_HPP
