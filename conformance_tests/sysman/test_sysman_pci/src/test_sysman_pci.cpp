/*
 *
 * Copyright (C) 2019-2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "logging/logging.hpp"
#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#define MAX_BARS 6

namespace lzt = level_zero_tests;

#include <level_zero/zes_api.h>

namespace {
class PciModuleTest : public lzt::SysmanCtsClass {};
TEST_F(PciModuleTest, GivenSysmanHandleWhenRetrievingStateThenStateIsReturned) {
  for (auto device : devices) {
    zes_pci_properties_t pciProps = {};
    pciProps = lzt::get_pci_properties(device);
    zes_pci_state_t pciState = {};
    pciState = lzt::get_pci_state(device);
    EXPECT_LE(pciState.speed.gen, pciProps.maxSpeed.gen);
    EXPECT_LE(pciState.speed.width, pciProps.maxSpeed.width);
    EXPECT_LE(pciState.speed.maxBandwidth, pciProps.maxSpeed.maxBandwidth);
    EXPECT_GE(pciState.status, ZES_PCI_LINK_STATUS_UNKNOWN);
    EXPECT_LE(pciState.status, ZES_PCI_LINK_STATUS_STABILITY_ISSUES);
    if (pciState.status == ZES_PCI_LINK_STATUS_STABILITY_ISSUES) {
      EXPECT_GE(pciState.qualityIssues, ZES_PCI_LINK_QUAL_ISSUE_FLAG_REPLAYS);
      EXPECT_LE(pciState.qualityIssues, ZES_PCI_LINK_QUAL_ISSUE_FLAG_SPEED);
      EXPECT_EQ(pciState.stabilityIssues,
                ZES_PCI_LINK_STAB_ISSUE_FLAG_RETRAINING);
    } else {
      EXPECT_EQ(pciState.qualityIssues, 0);
      EXPECT_EQ(pciState.stabilityIssues, 0);
    }
  }
}

TEST_F(PciModuleTest,
       GivenSysmanHandleWhenRetrievingPCIPropertiesThenpropertiesIsReturned) {
  for (auto device : devices) {
    zes_pci_properties_t pciProps = {};
    pciProps = lzt::get_pci_properties(device);
    auto pciStats = lzt::get_pci_stats(device);
    EXPECT_GE(pciProps.address.domain, 0);
    EXPECT_LT(pciProps.address.domain, MAX_DOMAINs);
    EXPECT_GE(pciProps.address.bus, 0);
    EXPECT_LT(pciProps.address.bus, MAX_BUSES_PER_DOMAIN);
    EXPECT_GE(pciProps.address.device, 0);
    EXPECT_LT(pciProps.address.device, MAX_DEVICES_PER_BUS);
    EXPECT_GE(pciProps.address.function, 0);
    EXPECT_LT(pciProps.address.function, MAX_FUNCTIONS_PER_DEVICE);
    EXPECT_GE(pciProps.maxSpeed.gen, 0);
    EXPECT_LE(pciProps.maxSpeed.gen, PCI_SPEED_MAX_LINK_GEN);
    EXPECT_GE(pciProps.maxSpeed.width, 0);
    EXPECT_LE(pciProps.maxSpeed.width, PCI_SPEED_MAX_LANE_WIDTH);
    EXPECT_GE(pciProps.maxSpeed.maxBandwidth, 0);
    EXPECT_LE(pciProps.maxSpeed.maxBandwidth, UINT64_MAX);
    if (pciProps.haveReplayCounters == false) {
      EXPECT_EQ(pciStats.replayCounter, 0);
    }
    if (pciProps.havePacketCounters == false) {
      EXPECT_EQ(pciStats.packetCounter, 0);
    }
    if (pciProps.haveBandwidthCounters == false) {
      EXPECT_EQ(pciStats.rxCounter, 0);
    }
    if (pciProps.haveBandwidthCounters == false) {
      EXPECT_EQ(pciStats.txCounter, 0);
    }
  }
}
TEST_F(PciModuleTest,
       GivenSysmanHandleWhenRetrievingPCIBarsThenNonZeroCountValueIsReturned) {
  for (auto device : devices) {
    auto pCount = lzt::get_pci_bar_count(device);
  }
}
TEST_F(
    PciModuleTest,
    GivenSysmanHandleWhenRetrievingPCIBarsThenValidBarPropertiesAreReturned) {
  for (auto device : devices) {
    uint32_t pCount = 0;
    auto pciBars = lzt::get_pci_bars(device, &pCount);
    for (auto pciBar : pciBars) {
      EXPECT_GE(pciBar.type, ZES_PCI_BAR_TYPE_MMIO);
      EXPECT_LE(pciBar.type, ZES_PCI_BAR_TYPE_MEM);
      EXPECT_GE((UINT64_MAX - pciBar.base), pciBar.size);
      EXPECT_GE(pciBar.index, 0);
      EXPECT_LE(pciBar.index, MAX_BARS); // Check for no. of PCI bars
      EXPECT_GT(pciBar.size, 0);
    }
  }
}
TEST_F(PciModuleTest,
       GivenSysmanHandleWhenRetrievingPCIStatsThenStatsAreReturned) {
  for (auto device : devices) {
    auto pciProps = lzt::get_pci_properties(device);
    auto pciStatsInitial = lzt::get_pci_stats(device);
    EXPECT_LE(pciStatsInitial.txCounter, UINT64_MAX);
    EXPECT_LE(pciStatsInitial.rxCounter, UINT64_MAX);
    if (pciProps.haveReplayCounters == true)
      EXPECT_LE(pciStatsInitial.replayCounter, UINT64_MAX);
    if (pciProps.havePacketCounters == true)
      EXPECT_LE(pciStatsInitial.packetCounter, UINT64_MAX);
    EXPECT_LE(pciStatsInitial.timestamp, UINT64_MAX);
    ze_command_list_handle_t command_list = lzt::create_command_list();
    ze_command_queue_handle_t cq = lzt::create_command_queue();
    const size_t size = 4096;
    std::vector<uint8_t> host_memory1(size), host_memory2(size, 0);
    void *device_memory =
        lzt::allocate_device_memory(lzt::size_in_bytes(host_memory1));
    lzt::write_data_pattern(host_memory1.data(), size, 1);
    lzt::append_memory_copy(command_list, device_memory, host_memory1.data(),
                            lzt::size_in_bytes(host_memory1), nullptr);
    lzt::append_barrier(command_list, nullptr, 0, nullptr);
    lzt::append_memory_copy(command_list, host_memory2.data(), device_memory,
                            lzt::size_in_bytes(host_memory2), nullptr);
    lzt::append_barrier(command_list, nullptr, 0, nullptr);
    lzt::close_command_list(command_list);
    lzt::execute_command_lists(cq, 1, &command_list, nullptr);
    lzt::synchronize(cq, UINT64_MAX);

    lzt::validate_data_pattern(host_memory2.data(), size, 1);
    lzt::free_memory(device_memory);
    lzt::destroy_command_queue(cq);
    lzt::destroy_command_list(command_list);
    auto pciStatsLater = lzt::get_pci_stats(device);
    EXPECT_LE(pciStatsLater.txCounter, UINT64_MAX);
    EXPECT_LE(pciStatsLater.rxCounter, UINT64_MAX);
    EXPECT_LE(pciStatsLater.replayCounter, UINT64_MAX);
    EXPECT_LE(pciStatsLater.packetCounter, UINT64_MAX);
    EXPECT_LE(pciStatsLater.timestamp, UINT64_MAX);
    if (pciProps.haveBandwidthCounters == true) {
      EXPECT_GT(pciStatsLater.txCounter, pciStatsInitial.txCounter);
      EXPECT_GT(pciStatsLater.rxCounter, pciStatsInitial.rxCounter);
    }
    if (pciProps.havePacketCounters == true) {
      EXPECT_GT(pciStatsLater.packetCounter, pciStatsInitial.packetCounter);
    }
    if (pciProps.haveReplayCounters == true) {
      EXPECT_GE(pciStatsLater.replayCounter, pciStatsInitial.replayCounter);
    }
    EXPECT_NE(pciStatsLater.timestamp, pciStatsInitial.timestamp);
  }
}
} // namespace
