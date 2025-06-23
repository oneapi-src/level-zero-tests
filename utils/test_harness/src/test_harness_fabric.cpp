/*
 *
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "test_harness/test_harness.hpp"
#include "utils/utils.hpp"

namespace lzt = level_zero_tests;

namespace level_zero_tests {

// Fabric Vertex related
uint32_t get_ze_fabric_vertex_count() {
  return get_ze_fabric_vertex_count(lzt::get_default_driver());
}

uint32_t get_ze_fabric_vertex_count(ze_driver_handle_t driver) {
  uint32_t count = 0;
  auto driver_initial = driver;
  EXPECT_ZE_RESULT_SUCCESS(zeFabricVertexGetExp(driver, &count, nullptr));
  EXPECT_EQ(driver, driver_initial);

  return count;
}

std::vector<ze_fabric_vertex_handle_t> get_ze_fabric_vertices() {
  return get_ze_fabric_vertices(get_ze_fabric_vertex_count());
}

std::vector<ze_fabric_vertex_handle_t> get_ze_fabric_vertices(uint32_t count) {
  return get_ze_fabric_vertices(count, lzt::get_default_driver());
}

std::vector<ze_fabric_vertex_handle_t>
get_ze_fabric_vertices(uint32_t count, ze_driver_handle_t driver) {
  uint32_t count_out = count;
  std::vector<ze_fabric_vertex_handle_t> vertices(count);

  auto driver_initial = driver;
  EXPECT_ZE_RESULT_SUCCESS(
      zeFabricVertexGetExp(driver, &count_out, vertices.data()));
  EXPECT_EQ(driver, driver_initial);
  if (count == get_ze_fabric_vertex_count())
    EXPECT_EQ(count_out, count);

  return vertices;
}

uint32_t get_ze_fabric_sub_vertices_count(ze_fabric_vertex_handle_t vertex) {
  uint32_t count = 0;

  auto vertex_initial = vertex;
  EXPECT_ZE_RESULT_SUCCESS(
      zeFabricVertexGetSubVerticesExp(vertex, &count, nullptr));
  EXPECT_EQ(vertex, vertex_initial);
  return count;
}

std::vector<ze_fabric_vertex_handle_t>
get_ze_fabric_sub_vertices(ze_fabric_vertex_handle_t vertex) {
  return get_ze_fabric_sub_vertices(vertex,
                                    get_ze_fabric_sub_vertices_count(vertex));
}

std::vector<ze_fabric_vertex_handle_t>
get_ze_fabric_sub_vertices(ze_fabric_vertex_handle_t vertex, uint32_t count) {
  std::vector<ze_fabric_vertex_handle_t> sub_vertices(count);

  auto vertex_initial = vertex;
  EXPECT_ZE_RESULT_SUCCESS(
      zeFabricVertexGetSubVerticesExp(vertex, &count, sub_vertices.data()));
  EXPECT_EQ(vertex, vertex_initial);
  return sub_vertices;
}

ze_fabric_vertex_exp_properties_t
get_ze_fabric_vertex_properties(ze_fabric_vertex_handle_t vertex) {

  auto vertex_initial = vertex;
  ze_fabric_vertex_exp_properties_t properties = {};
  properties.stype = ZE_STRUCTURE_TYPE_FABRIC_VERTEX_EXP_PROPERTIES;
  EXPECT_ZE_RESULT_SUCCESS(zeFabricVertexGetPropertiesExp(vertex, &properties));
  EXPECT_EQ(vertex, vertex_initial);
  return properties;
}

// Fabric Edge related

uint32_t get_ze_fabric_edge_count(ze_fabric_vertex_handle_t vertex_a,
                                  ze_fabric_vertex_handle_t vertex_b) {
  uint32_t count = 0;
  EXPECT_ZE_RESULT_SUCCESS(
      zeFabricEdgeGetExp(vertex_a, vertex_b, &count, nullptr));
  return count;
}

std::vector<ze_fabric_edge_handle_t>
get_ze_fabric_edges(ze_fabric_vertex_handle_t vertex_a,
                    ze_fabric_vertex_handle_t vertex_b) {
  return get_ze_fabric_edges(vertex_a, vertex_b,
                             get_ze_fabric_edge_count(vertex_a, vertex_b));
}

std::vector<ze_fabric_edge_handle_t>
get_ze_fabric_edges(ze_fabric_vertex_handle_t vertex_a,
                    ze_fabric_vertex_handle_t vertex_b, uint32_t count) {
  uint32_t count_out = count;
  std::vector<ze_fabric_edge_handle_t> edges(count);
  EXPECT_ZE_RESULT_SUCCESS(
      zeFabricEdgeGetExp(vertex_a, vertex_b, &count_out, edges.data()));
  EXPECT_EQ(count_out, count);
  if (count == 0) {
    return {};
  } else {
    return edges;
  }
}

ze_fabric_edge_exp_properties_t
get_ze_fabric_edge_properties(ze_fabric_edge_handle_t edge) {

  auto edge_initial = edge;
  ze_fabric_edge_exp_properties_t properties = {};
  properties.stype = ZE_STRUCTURE_TYPE_FABRIC_EDGE_EXP_PROPERTIES;
  EXPECT_ZE_RESULT_SUCCESS(zeFabricEdgeGetPropertiesExp(edge, &properties));
  EXPECT_EQ(edge, edge_initial);
  return properties;
}

}; // namespace level_zero_tests
