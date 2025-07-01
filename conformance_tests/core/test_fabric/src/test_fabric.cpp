/*
 *
 * Copyright (C) 2022-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "logging/logging.hpp"

#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;
namespace bp = boost::process;

#include <level_zero/ze_api.h>

namespace {

LZT_TEST(zeFabricVertexGetTests,
         GivenZeroCountWhenRetrievingFabricVerticesThenValidCountReturned) {
  auto vertex_count = lzt::get_ze_fabric_vertex_count();
  EXPECT_GT(vertex_count, 0);
}

LZT_TEST(
    zeFabricVertexGetTests,
    GivenValidCountWhenRetrievingFabricVerticesThenNotNullFabricVerticesAreReturned) {

  auto vertex_count = lzt::get_ze_fabric_vertex_count();
  ASSERT_GT(vertex_count, 0);
  auto vertices = lzt::get_ze_fabric_vertices(vertex_count);
  for (const auto &vertex : vertices) {
    EXPECT_NE(nullptr, vertex);
  }
}

LZT_TEST(
    zeFabricVertexGetTests,
    GivenValidCountWhenRetrievingFabricVerticesThenVerifyMultipleCallsReturnSameVertexHandles) {

  auto vertices_first = lzt::get_ze_fabric_vertices();
  auto vertices_second = lzt::get_ze_fabric_vertices();
  EXPECT_TRUE(vertices_first == vertices_second);

  for (auto &vertex : vertices_first) {
    auto subVertices_first = lzt::get_ze_fabric_sub_vertices(vertex);
    auto subVertices_second = lzt::get_ze_fabric_sub_vertices(vertex);
    EXPECT_TRUE(subVertices_first == subVertices_second);
  }
}

LZT_TEST(
    zeFabricVertexGetTests,
    GivenAffinityMaskIsSetWhenCallingFabricVertexGetThenVerifyOnlyExpectedVerticesAndSubVerticesAreReturned) {

  auto devices = lzt::get_ze_devices();
  if (devices.size() < 1) {
    LOG_WARNING << "Test not executed due to not enough devices";
    GTEST_SKIP();
  }

  auto sub_devices = lzt::get_ze_sub_devices(devices[0]);
  if (sub_devices.size() < 2) {
    LOG_WARNING << "Test not executed due to not enough sub-devices";
    GTEST_SKIP();
  }

  bp::environment child_env = boost::this_process::environment();
  child_env["ZE_AFFINITY_MASK"] = "0.1";
  fs::path helper_path(fs::current_path() / "fabric");
  std::vector<fs::path> paths;
  paths.push_back(helper_path);
  fs::path helper = bp::search_path("test_fabric_helper", paths);
  bp::child fabric_helper(helper, child_env);

  fabric_helper.wait();
  const auto is_affinity_set_correctly = (fabric_helper.exit_code() == 0);
  EXPECT_TRUE(is_affinity_set_correctly);
}

LZT_TEST(
    zeFabricVertexGetPropertiesTests,
    GivenValidFabricVertexWhenRetrievingPropertiesThenValidPropertiesAreReturned) {

  auto vertices = lzt::get_ze_fabric_vertices();
  for (const auto &vertex : vertices) {
    ze_fabric_vertex_exp_properties_t properties =
        lzt::get_ze_fabric_vertex_properties(vertex);
    ze_device_handle_t device = nullptr;
    if (ZE_RESULT_SUCCESS == zeFabricVertexGetDeviceExp(vertex, &device)) {
      ze_device_properties_t device_properties{};
      device_properties = lzt::get_device_properties(device);
      EXPECT_EQ(0, memcmp(properties.uuid.id, device_properties.uuid.id,
                          sizeof(properties.uuid.id)));
      EXPECT_EQ(ZE_FABRIC_VERTEX_EXP_TYPE_DEVICE, properties.type);
      EXPECT_FALSE(properties.remote);

      ze_pci_ext_properties_t pciProperties{};
      if (ZE_RESULT_SUCCESS ==
          zeDevicePciGetPropertiesExt(device, &pciProperties)) {
        EXPECT_EQ(properties.address.bus, pciProperties.address.bus);
        EXPECT_EQ(properties.address.device, pciProperties.address.device);
        EXPECT_EQ(properties.address.function, pciProperties.address.function);
      }
    } else {
      EXPECT_TRUE(properties.remote);
    }
  }
}

LZT_TEST(
    zeFabricVertexGetTests,
    GivenValidVerticesAndSubverticesThenVerifyVerticesAreDevicesAndSubverticesAreSubDevices) {

  auto vertices = lzt::get_ze_fabric_vertices();

  for (auto &vertex : vertices) {
    ze_device_handle_t device = {};
    ze_device_properties_t device_properties = {};
    EXPECT_ZE_RESULT_SUCCESS(zeFabricVertexGetDeviceExp(vertex, &device));
    EXPECT_ZE_RESULT_SUCCESS(zeDeviceGetProperties(device, &device_properties));
    EXPECT_EQ(device_properties.flags & ZE_DEVICE_PROPERTY_FLAG_SUBDEVICE, 0u);

    auto count = lzt::get_ze_fabric_sub_vertices_count(vertex);
    if (count > 0) {
      std::vector<ze_fabric_vertex_handle_t> sub_vertices(count);
      sub_vertices = lzt::get_ze_fabric_sub_vertices(vertex);
      for (const auto &sub_vertex : sub_vertices) {
        ze_device_handle_t sub_device = {};
        ze_device_properties_t sub_device_properties = {};
        EXPECT_ZE_RESULT_SUCCESS(
            zeFabricVertexGetDeviceExp(sub_vertex, &sub_device));
        EXPECT_ZE_RESULT_SUCCESS(
            zeDeviceGetProperties(sub_device, &sub_device_properties));
        EXPECT_EQ(sub_device_properties.flags &
                      ZE_DEVICE_PROPERTY_FLAG_SUBDEVICE,
                  ZE_DEVICE_PROPERTY_FLAG_SUBDEVICE);
      }
    }
  }
}

LZT_TEST(
    zeFabricSubVertexGetTests,
    GivenValidCountWhenRetrievingFabricSubVerticesThenNotNullFabricVerticesAreReturned) {

  auto vertices = lzt::get_ze_fabric_vertices();
  for (auto &vertex : vertices) {
    ASSERT_NE(nullptr, vertex);

    auto count = lzt::get_ze_fabric_sub_vertices_count(vertex);

    ze_device_handle_t device = nullptr;
    if (ZE_RESULT_SUCCESS == zeFabricVertexGetDeviceExp(vertex, &device)) {
      EXPECT_EQ(lzt::get_ze_sub_device_count(device), count);
    }

    if (count > 0) {
      std::vector<ze_fabric_vertex_handle_t> sub_vertices(count);
      sub_vertices = lzt::get_ze_fabric_sub_vertices(vertex);
      for (const auto &sub_vertex : sub_vertices) {
        EXPECT_NE(nullptr, sub_vertex);
      }
    }
  }
}

LZT_TEST(
    zeFabricVertexGetPropertiesTests,
    GivenValidFabricSubVertexWhenRetrievingPropertiesThenValidPropertiesAreReturned) {

  auto vertices = lzt::get_ze_fabric_vertices();
  for (const auto &vertex : vertices) {
    auto sub_vertices = lzt::get_ze_fabric_sub_vertices(vertex);
    for (const auto &sub_vertex : sub_vertices) {
      ze_fabric_vertex_exp_properties_t properties =
          lzt::get_ze_fabric_vertex_properties(sub_vertex);

      ze_device_handle_t device = nullptr;
      if (ZE_RESULT_SUCCESS ==
          zeFabricVertexGetDeviceExp(sub_vertex, &device)) {
        ze_device_properties_t device_properties{};
        device_properties = lzt::get_device_properties(device);
        EXPECT_EQ(0, memcmp(properties.uuid.id, device_properties.uuid.id,
                            sizeof(properties.uuid.id)));
        EXPECT_EQ(ZE_FABRIC_VERTEX_EXP_TYPE_SUBDEVICE, properties.type);
        EXPECT_FALSE(properties.remote);

        ze_pci_ext_properties_t pciProperties{};
        if (ZE_RESULT_SUCCESS ==
            zeDevicePciGetPropertiesExt(device, &pciProperties)) {
          EXPECT_EQ(properties.address.bus, pciProperties.address.bus);
          EXPECT_EQ(properties.address.device, pciProperties.address.device);
          EXPECT_EQ(properties.address.function,
                    pciProperties.address.function);
        }
      } else {
        EXPECT_TRUE(properties.remote);
      }
    }
  }
}

LZT_TEST(
    zeDeviceGetFabricVertexTests,
    GivenValidDeviceAndSubDeviceWhenGettingVertexThenValidVertexIsReturned) {

  auto devices = lzt::get_ze_devices();
  for (const auto &device : devices) {

    std::vector<ze_device_handle_t> sub_devices{};
    sub_devices = lzt::get_ze_sub_devices(device);

    for (const auto &sub_device : sub_devices) {
      ze_fabric_vertex_handle_t vertex{};
      ze_device_handle_t device_vertex{};
      EXPECT_ZE_RESULT_SUCCESS(zeDeviceGetFabricVertexExp(sub_device, &vertex));
      ASSERT_NE(vertex, nullptr);
      EXPECT_ZE_RESULT_SUCCESS(
          zeFabricVertexGetDeviceExp(vertex, &device_vertex));
      ASSERT_NE(device_vertex, nullptr);
      EXPECT_EQ(device_vertex, sub_device);
    }

    ze_fabric_vertex_handle_t vertex{};
    ze_device_handle_t device_vertex{};
    EXPECT_ZE_RESULT_SUCCESS(zeDeviceGetFabricVertexExp(device, &vertex));
    ASSERT_NE(vertex, nullptr);
    EXPECT_ZE_RESULT_SUCCESS(
        zeFabricVertexGetDeviceExp(vertex, &device_vertex));
    ASSERT_NE(device_vertex, nullptr);
    EXPECT_EQ(device_vertex, device);
  }
}

// Fabric Edge related

static std::vector<ze_fabric_vertex_handle_t> fabric_get_all_vertices() {

  std::vector<ze_fabric_vertex_handle_t> all_vertices;
  auto vertices = lzt::get_ze_fabric_vertices();
  for (const auto &vertex : vertices) {
    all_vertices.push_back(vertex);

    auto sub_vertices = lzt::get_ze_fabric_sub_vertices(vertex);
    for (const auto &sub_vertex : sub_vertices) {
      all_vertices.push_back(sub_vertex);
    }
  }

  return all_vertices;
}

static std::vector<ze_fabric_edge_handle_t> fabric_get_all_edges() {

  std::vector<ze_fabric_edge_handle_t> all_edges;
  std::vector<ze_fabric_vertex_handle_t> vertices = fabric_get_all_vertices();
  auto vertex_count = vertices.size();
  if (vertex_count >= 2) {
    for (auto &vertex_a : vertices) {
      for (auto &vertex_b : vertices) {
        auto edge_count = lzt::get_ze_fabric_edge_count(vertex_a, vertex_b);
        std::vector<ze_fabric_edge_handle_t> edges{};
        edges = lzt::get_ze_fabric_edges(vertex_a, vertex_b, edge_count);
        for (auto &edge : edges) {
          if (edge == nullptr) {
            continue;
          }

          if (std::count(all_edges.begin(), all_edges.end(), edge) == 0) {
            all_edges.push_back(edge);
          }
        }
      }
    }
  }

  return all_edges;
}

LZT_TEST(zeFabricEdgeGetTests,
         GivenZeroCountWhenRetrievingFabricEdgesThenValidCountReturned) {

  std::vector<ze_fabric_vertex_handle_t> vertices = fabric_get_all_vertices();
  auto vertex_count = vertices.size();
  if (vertex_count < 2) {
    LOG_WARNING << "Test not executed due to not enough vertices";
    GTEST_SKIP();
  }

  for (auto &vertex_a : vertices) {
    for (auto &vertex_b : vertices) {
      EXPECT_GE(lzt::get_ze_fabric_edge_count(vertex_a, vertex_b), 0);
    }
  }
}

LZT_TEST(
    zeFabricEdgeGetTests,
    GivenValidEdgesWhenRetrievingFabricEdgesThenVerifyMultipleCallsReturnsSameEdges) {

  std::vector<ze_fabric_vertex_handle_t> vertices = fabric_get_all_vertices();
  auto vertex_count = vertices.size();
  if (vertex_count < 2) {
    LOG_WARNING << "Test not executed due to not enough vertices";
    GTEST_SKIP();
  }

  for (auto &vertex_a : vertices) {
    for (auto &vertex_b : vertices) {
      auto edge_count = lzt::get_ze_fabric_edge_count(vertex_a, vertex_b);
      auto edges_first =
          lzt::get_ze_fabric_edges(vertex_a, vertex_b, edge_count);
      auto edges_second =
          lzt::get_ze_fabric_edges(vertex_a, vertex_b, edge_count);
      EXPECT_TRUE(edges_first == edges_second);
    }
  }
}

LZT_TEST(
    zeFabricEdgeGetTests,
    GivenFabricEdgeExistsBetweenSubVerticesOfDifferentVerticesThenVerifyEdgeExistsBetweenVertices) {

  if (lzt::get_ze_device_count() < 2) {
    LOG_WARNING << "Test not executed due to not enough devices";
    GTEST_SKIP();
  }

  auto devices = lzt::get_ze_devices();
  if (lzt::get_ze_sub_device_count(devices[0]) == 0) {
    LOG_WARNING << "Test not executed due to not enough sub devices";
    GTEST_SKIP();
  }

  if (lzt::get_ze_sub_device_count(devices[1]) == 0) {
    LOG_WARNING << "Test not executed due to not enough sub devices";
    GTEST_SKIP();
  }

  bool is_sub_device_connected = false;
  // Check connectivity between the subdevices of the devices
  for (auto sub_device_root0 : lzt::get_ze_sub_devices(devices[0])) {

    ze_fabric_vertex_handle_t vertex_a{};
    EXPECT_ZE_RESULT_SUCCESS(
        zeDeviceGetFabricVertexExp(sub_device_root0, &vertex_a));

    for (auto sub_device_root1 : lzt::get_ze_sub_devices(devices[1])) {
      ze_fabric_vertex_handle_t vertex_b{};

      EXPECT_ZE_RESULT_SUCCESS(
          zeDeviceGetFabricVertexExp(sub_device_root1, &vertex_b));
      if (lzt::get_ze_fabric_edge_count(vertex_a, vertex_b) > 0) {
        is_sub_device_connected = true;
        break;
      }
    }
  }

  if (is_sub_device_connected == true) {
    ze_fabric_vertex_handle_t vertex_a{};
    EXPECT_ZE_RESULT_SUCCESS(zeDeviceGetFabricVertexExp(devices[0], &vertex_a));
    ze_fabric_vertex_handle_t vertex_b{};
    EXPECT_ZE_RESULT_SUCCESS(zeDeviceGetFabricVertexExp(devices[1], &vertex_b));
    EXPECT_GT(lzt::get_ze_fabric_edge_count(vertex_a, vertex_b), 0);
  } else {
    LOG_WARNING << "Test not executed since fabric edges do not exist between "
                   "sub devices";
  }
}

LZT_TEST(
    zeFabricEdgeGetTests,
    GivenValidCountWhenRetrievingFabricEdgesThenValidFabricEdgesAreReturned) {

  std::vector<ze_fabric_vertex_handle_t> vertices = fabric_get_all_vertices();
  auto vertex_count = vertices.size();
  if (vertex_count < 2) {
    LOG_WARNING << "Test not executed due to not enough vertices";
    GTEST_SKIP();
  }

  for (auto &vertex_a : vertices) {
    for (auto &vertex_b : vertices) {
      auto edge_count = lzt::get_ze_fabric_edge_count(vertex_a, vertex_b);
      std::vector<ze_fabric_edge_handle_t> edges(edge_count);
      edges = lzt::get_ze_fabric_edges(vertex_a, vertex_b, edge_count);
      for (auto &edge : edges) {
        EXPECT_NE(edge, nullptr);
        ze_fabric_vertex_handle_t check_vertex_a = nullptr,
                                  check_vertex_b = nullptr;
        EXPECT_ZE_RESULT_SUCCESS(
            zeFabricEdgeGetVerticesExp(edge, &check_vertex_a, &check_vertex_b));
        EXPECT_TRUE(check_vertex_a == vertex_a || check_vertex_a == vertex_b);
        EXPECT_TRUE(check_vertex_b == vertex_a || check_vertex_b == vertex_b);
      }
    }
  }
}

LZT_TEST(zeFabricEdgeGetTests,
         GivenValidFabricEdgesThenValidEdgePropertiesAreReturned) {

  std::vector<ze_fabric_edge_handle_t> edges = fabric_get_all_edges();
  if (edges.size() == 0) {
    LOG_WARNING << "Test not executed due to not enough edges";
    GTEST_SKIP();
  }

  for (auto &edge : edges) {
    ze_fabric_edge_exp_properties_t property =
        lzt::get_ze_fabric_edge_properties(edge);
    if (property.bandwidth != 0) {
      EXPECT_TRUE(property.bandwidthUnit ==
                      ZE_BANDWIDTH_UNIT_BYTES_PER_NANOSEC ||
                  property.bandwidthUnit == ZE_BANDWIDTH_UNIT_BYTES_PER_CLOCK);
    }

    EXPECT_TRUE(property.latencyUnit == ZE_LATENCY_UNIT_NANOSEC ||
                property.latencyUnit == ZE_LATENCY_UNIT_CLOCK ||
                property.latencyUnit == ZE_LATENCY_UNIT_HOP ||
                property.latencyUnit == ZE_LATENCY_UNIT_UNKNOWN);
    EXPECT_TRUE(property.duplexity ==
                    ZE_FABRIC_EDGE_EXP_DUPLEXITY_HALF_DUPLEX ||
                property.duplexity == ZE_FABRIC_EDGE_EXP_DUPLEXITY_FULL_DUPLEX);
  }
}

LZT_TEST(zeFabricEdgeGetTests,
         GivenValidFabricEdgesThenEdgePropertyUuidIsUnique) {

  std::vector<ze_fabric_edge_handle_t> edges = fabric_get_all_edges();
  if (edges.size() == 0) {
    LOG_WARNING << "Test not executed due to not enough edges";
    GTEST_SKIP();
  }

  std::vector<ze_uuid_t> uuids;

  auto is_uuid_unique = [uuids](ze_uuid_t &uuid) {
    for (auto &prev_uuid : uuids) {
      int status = std::memcmp(prev_uuid.id, uuid.id, sizeof(prev_uuid.id));
      if (status == 0) {
        return false;
      }
    }
    return true;
  };

  for (auto &edge : edges) {
    ze_fabric_edge_exp_properties_t property =
        lzt::get_ze_fabric_edge_properties(edge);
    const bool unique_status = is_uuid_unique(property.uuid);
    EXPECT_TRUE(unique_status);
    if (unique_status == true) {
      uuids.push_back(property.uuid);
    } else {
      break;
    }
  }
}

LZT_TEST(zeFabricEdgeGetTests,
         GivenValidFabricEdgesThenVerifyDevicesCanAccessPeer) {
  std::vector<ze_fabric_edge_handle_t> edges = fabric_get_all_edges();
  if (edges.size() == 0) {
    LOG_WARNING << "Test not executed due to not enough edges";
    GTEST_SKIP();
  }

  for (auto &edge : edges) {
    ze_fabric_vertex_handle_t vertex_a = nullptr, vertex_b = nullptr;
    EXPECT_ZE_RESULT_SUCCESS(
        zeFabricEdgeGetVerticesExp(edge, &vertex_a, &vertex_b));
    ze_device_handle_t device_a{}, device_b{};
    EXPECT_ZE_RESULT_SUCCESS(zeFabricVertexGetDeviceExp(vertex_a, &device_a));
    EXPECT_ZE_RESULT_SUCCESS(zeFabricVertexGetDeviceExp(vertex_b, &device_b));
    EXPECT_TRUE(lzt::can_access_peer(device_a, device_b));
  }
}

static void
fabric_edge_get_minimum_bandwidth(std::vector<ze_fabric_edge_handle_t> &edges,
                                  ze_bandwidth_unit_t &bandwidth_unit,
                                  uint32_t &bandwidth) {

  for (auto &edge : edges) {
    auto edge_property = lzt::get_ze_fabric_edge_properties(edges[0]);
    if (edge_property.bandwidth == 0) {
      continue;
    }

    // Ignore different bandwidth units
    if ((bandwidth_unit != ZE_BANDWIDTH_UNIT_UNKNOWN) &&
        (bandwidth_unit != edge_property.bandwidthUnit)) {
      continue;
    }

    bandwidth = std::min(bandwidth, edge_property.bandwidth);
    bandwidth_unit = edge_property.bandwidthUnit;
  }
}

LZT_TEST(
    zeFabricEdgeGetTests,
    GivenValidFabricEdgesThenVerifyThatRootVerticesEdgeBandwidthIsAtleastTheLowestBandwidthOfTheSubVerticesBandwidth) {

  if (lzt::get_ze_device_count() < 2) {
    LOG_WARNING << "Test not executed due to not enough devices";
    GTEST_SKIP();
  }

  auto devices = lzt::get_ze_devices();
  if (lzt::get_ze_sub_device_count(devices[0]) == 0) {
    LOG_WARNING << "Test not executed due to not enough sub devices";
    GTEST_SKIP();
  }

  if (lzt::get_ze_sub_device_count(devices[1]) == 0) {
    LOG_WARNING << "Test not executed due to not enough sub devices";
    GTEST_SKIP();
  }

  // Identify Edges between subdevices and find the least bandwidth
  uint32_t sub_device_bandwidth = std::numeric_limits<uint32_t>::max();
  auto sub_device_bandwidth_unit = ZE_BANDWIDTH_UNIT_UNKNOWN;
  auto sub_devices_root0 = lzt::get_ze_sub_devices(devices[0]);
  for (auto sub_device_root0 : sub_devices_root0) {
    ze_fabric_vertex_handle_t vertex_a{};
    EXPECT_ZE_RESULT_SUCCESS(
        zeDeviceGetFabricVertexExp(sub_device_root0, &vertex_a));
    auto sub_devices_root1 = lzt::get_ze_sub_devices(devices[1]);

    for (auto sub_device_root1 : sub_devices_root1) {
      ze_fabric_vertex_handle_t vertex_b{};

      EXPECT_ZE_RESULT_SUCCESS(
          zeDeviceGetFabricVertexExp(sub_device_root1, &vertex_b));
      auto edge_count = lzt::get_ze_fabric_edge_count(vertex_a, vertex_b);
      if (edge_count > 0) {

        auto edges = lzt::get_ze_fabric_edges(vertex_a, vertex_b, edge_count);
        fabric_edge_get_minimum_bandwidth(edges, sub_device_bandwidth_unit,
                                          sub_device_bandwidth);
      }
    }
  }

  EXPECT_NE(sub_device_bandwidth_unit, ZE_BANDWIDTH_UNIT_UNKNOWN);

  if (sub_device_bandwidth_unit != ZE_BANDWIDTH_UNIT_UNKNOWN) {

    uint32_t device_bandwidth = std::numeric_limits<uint32_t>::max();
    auto device_bandwidth_unit = ZE_BANDWIDTH_UNIT_UNKNOWN;
    ze_fabric_vertex_handle_t vertex_a{}, vertex_b{};
    EXPECT_ZE_RESULT_SUCCESS(zeDeviceGetFabricVertexExp(devices[0], &vertex_a));
    EXPECT_ZE_RESULT_SUCCESS(zeDeviceGetFabricVertexExp(devices[1], &vertex_b));

    auto edge_count = lzt::get_ze_fabric_edge_count(vertex_a, vertex_b);
    EXPECT_GT(edge_count, 0u);
    auto edges = lzt::get_ze_fabric_edges(vertex_a, vertex_b, edge_count);
    fabric_edge_get_minimum_bandwidth(edges, device_bandwidth_unit,
                                      device_bandwidth);
    EXPECT_NE(device_bandwidth_unit, ZE_BANDWIDTH_UNIT_UNKNOWN);

    EXPECT_GE(device_bandwidth, sub_device_bandwidth);
  }
}

static void fabric_vertex_copy_memory(ze_fabric_vertex_handle_t &vertex_a,
                                      ze_fabric_vertex_handle_t &vertex_b,
                                      uint32_t copy_size,
                                      ze_memory_type_t usm_type,
                                      bool is_immediate) {

  ze_device_handle_t device_a{}, device_b{};
  ASSERT_ZE_RESULT_SUCCESS(zeFabricVertexGetDeviceExp(vertex_a, &device_a));
  ASSERT_ZE_RESULT_SUCCESS(zeFabricVertexGetDeviceExp(vertex_b, &device_b));
  ASSERT_TRUE(lzt::can_access_peer(device_a, device_b));
  ASSERT_TRUE(lzt::get_p2p_properties(device_a, device_b).flags &
              ZE_DEVICE_P2P_PROPERTY_FLAG_ACCESS);
  if (usm_type == ZE_MEMORY_TYPE_SHARED) {
    const auto mem_access_caps_a = lzt::get_memory_access_properties(device_a);
    const auto mem_access_caps_b = lzt::get_memory_access_properties(device_b);
    if ((mem_access_caps_a.sharedCrossDeviceAllocCapabilities &
         mem_access_caps_b.sharedCrossDeviceAllocCapabilities &
         ZE_MEMORY_ACCESS_CAP_FLAG_RW) == 0) {
      GTEST_SKIP()
          << "Cross-device access of shared USM buffers is unsupported";
    }
  }
  LOG_INFO << "Copy memory from (vertex: " << vertex_a
           << " device: " << device_a << ") to (vertex: " << vertex_b
           << " device: " << device_b << ")" << std::endl;

  auto cmd_bundle_a = lzt::create_command_bundle(device_a, is_immediate);
  auto cmd_bundle_b = lzt::create_command_bundle(device_b, is_immediate);

  const size_t size = copy_size;
  void *memory_a = nullptr;
  void *memory_b = nullptr;
  void *memory_c = nullptr;
  if (usm_type == ZE_MEMORY_TYPE_DEVICE) {
    memory_a = lzt::allocate_device_memory(size, 1, 0, device_a,
                                           lzt::get_default_context());
    memory_b = lzt::allocate_device_memory(size, 1, 0, device_b,
                                           lzt::get_default_context());
    memory_c = lzt::allocate_host_memory(size, 1, lzt::get_default_context());
  } else if (usm_type == ZE_MEMORY_TYPE_SHARED) {
    // Pass nullptr as device handle instead of device_a & device_b
    memory_a = lzt::allocate_shared_memory(size, 1, 0, 0, nullptr,
                                           lzt::get_default_context());
    memory_b = lzt::allocate_shared_memory(size, 1, 0, 0, nullptr,
                                           lzt::get_default_context());
    memory_c = memory_b;
  } else {
    FAIL() << "Unknown memory type";
  }
  uint8_t pattern_a = 0xAB;
  uint8_t pattern_b = 0xFF;
  const int pattern_size = 1;

  // Fill with default pattern for both devices
  lzt::append_memory_fill(cmd_bundle_a.list, memory_a, &pattern_a, pattern_size,
                          size, nullptr);
  lzt::close_command_list(cmd_bundle_a.list);
  lzt::execute_and_sync_command_bundle(cmd_bundle_a, UINT64_MAX);
  lzt::reset_command_list(cmd_bundle_a.list);

  lzt::append_memory_fill(cmd_bundle_b.list, memory_b, &pattern_b, pattern_size,
                          size, nullptr);
  lzt::close_command_list(cmd_bundle_b.list);
  lzt::execute_and_sync_command_bundle(cmd_bundle_b, UINT64_MAX);
  lzt::reset_command_list(cmd_bundle_b.list);

  // Do memory copy between devices
  lzt::append_memory_copy(cmd_bundle_a.list, memory_b, memory_a, size);
  lzt::close_command_list(cmd_bundle_a.list);
  lzt::execute_and_sync_command_bundle(cmd_bundle_a, UINT64_MAX);

  // If using device USM, copy to the host to verify
  if (usm_type == ZE_MEMORY_TYPE_DEVICE) {
    lzt::append_memory_copy(cmd_bundle_b.list, memory_c, memory_b, size);
    lzt::close_command_list(cmd_bundle_b.list);
    lzt::execute_and_sync_command_bundle(cmd_bundle_b, UINT64_MAX);
  }

  for (uint32_t i = 0; i < size; i++) {
    EXPECT_EQ(static_cast<uint8_t *>(memory_c)[i], pattern_a)
        << "Memory Fill did not match.";
    if (::testing::Test::HasFailure()) {
      break;
    }
  }

  lzt::free_memory(memory_a);
  lzt::free_memory(memory_b);
  if (usm_type == ZE_MEMORY_TYPE_DEVICE) {
    lzt::free_memory(memory_c);
  }
  lzt::destroy_command_bundle(cmd_bundle_a);
  lzt::destroy_command_bundle(cmd_bundle_b);
}

class zeFabricEdgeCopyTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<
          std::tuple<uint32_t, ze_memory_type_t, bool>> {};

LZT_TEST_P(zeFabricEdgeCopyTests,
           GivenValidFabricEdgesThenCopyIsSuccessfulBetweenThem) {

  std::vector<ze_fabric_edge_handle_t> edges = fabric_get_all_edges();
  if (edges.size() == 0) {
    LOG_WARNING << "Test not executed due to not enough edges";
    GTEST_SKIP();
  }

  uint32_t copy_size = std::get<0>(GetParam());
  LOG_DEBUG << "Test Copy Size " << copy_size;
  ze_memory_type_t usm_type = std::get<1>(GetParam());
  bool is_immediate = std::get<2>(GetParam());

  for (auto &edge : edges) {
    ze_fabric_vertex_handle_t vertex_a = nullptr, vertex_b = nullptr;
    ASSERT_ZE_RESULT_SUCCESS(
        zeFabricEdgeGetVerticesExp(edge, &vertex_a, &vertex_b));

    fabric_vertex_copy_memory(vertex_a, vertex_b, copy_size, usm_type,
                              is_immediate);
  }
}

INSTANTIATE_TEST_SUITE_P(
    zeFabricEdgeCopyTestAlignedAllocations, zeFabricEdgeCopyTests,
    ::testing::Combine(::testing::Values(1024u * 1024u, 64u * 1024u, 8u * 1024u,
                                         1u * 1024u, 64u, 1u),
                       ::testing::Values(ZE_MEMORY_TYPE_DEVICE,
                                         ZE_MEMORY_TYPE_SHARED),
                       ::testing::Bool()));

} // namespace
