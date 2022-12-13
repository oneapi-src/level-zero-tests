/*
 *
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"

namespace lzt = level_zero_tests;

int main(int argc, char **argv) {

  ze_result_t result = zeInit(0);
  if (result != ZE_RESULT_SUCCESS) {
    std::cout << "[fabric helper] zeInit failed";
    exit(1);
  }
  auto vertices = lzt::get_ze_fabric_vertices();

  if (vertices.size() != 1) {
    LOG_DEBUG << "vertices size  : " << vertices.size();
    return 1;
  }

  auto sub_vertices = lzt::get_ze_fabric_sub_vertices(vertices[0]);
  if (sub_vertices.size() > 1) {
    LOG_DEBUG << "sub_vertices size  : " << sub_vertices.size();
    return 1;
  }
  return 0;
}
