/*
 *
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef level_zero_tests_ZE_TEST_HARNESS_FABRIC_HPP
#define level_zero_tests_ZE_TEST_HARNESS_FABRIC_HPP

#include <level_zero/ze_api.h>
#include "gtest/gtest.h"

namespace level_zero_tests {
uint32_t get_ze_fabric_vertex_count();
uint32_t get_ze_fabric_vertex_count(ze_driver_handle_t driver);
std::vector<ze_fabric_vertex_handle_t> get_ze_fabric_vertices();
std::vector<ze_fabric_vertex_handle_t> get_ze_fabric_vertices(uint32_t count);
std::vector<ze_fabric_vertex_handle_t>
get_ze_fabric_vertices(uint32_t count, ze_driver_handle_t driver);
uint32_t get_ze_fabric_sub_vertices_count(ze_fabric_vertex_handle_t vertex);
std::vector<ze_fabric_vertex_handle_t>
get_ze_fabric_sub_vertices(ze_fabric_vertex_handle_t vertex);
std::vector<ze_fabric_vertex_handle_t>
get_ze_fabric_sub_vertices(ze_fabric_vertex_handle_t vertex, uint32_t count);
ze_fabric_vertex_exp_properties_t
get_ze_fabric_vertex_properties(ze_fabric_vertex_handle_t vertex);

// Fabric Edge related
uint32_t get_ze_fabric_edge_count(ze_fabric_vertex_handle_t vertex_a,
                                  ze_fabric_vertex_handle_t vertex_b);
std::vector<ze_fabric_edge_handle_t>
get_ze_fabric_edges(ze_fabric_vertex_handle_t vertex_a,
                    ze_fabric_vertex_handle_t vertex_b);
std::vector<ze_fabric_edge_handle_t>
get_ze_fabric_edges(ze_fabric_vertex_handle_t vertex_a,
                    ze_fabric_vertex_handle_t vertex_b, uint32_t count);
ze_fabric_edge_exp_properties_t
get_ze_fabric_edge_properties(ze_fabric_edge_handle_t edge);

}; // namespace level_zero_tests
#endif
