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
    auto p_count = lzt::get_pci_bar_count(device);
  }
}
TEST_F(
    PciModuleTest,
    GivenSysmanHandleWhenRetrievingPCIBarsThenValidBarPropertiesAreReturned) {
  for (auto device : devices) {
    uint32_t p_count = 0;
    auto pci_bars = lzt::get_pci_bars(device, &p_count);
    for (auto pci_bar : pci_bars) {
      EXPECT_GE(pci_bar.type, ZES_PCI_BAR_TYPE_MMIO);
      EXPECT_LE(pci_bar.type, ZES_PCI_BAR_TYPE_MEM);
      EXPECT_GE((UINT64_MAX - pci_bar.base), pci_bar.size);
      EXPECT_GE(pci_bar.index, 0);
      EXPECT_LE(pci_bar.index, MAX_BARS); // Check for no. of PCI bars
      EXPECT_GT(pci_bar.size, 0);
    }
  }
}
TEST_F(
    PciModuleTest,
    GivenSysmanHandleWhenRetrievingPCIBarsExtensionThenValidBarPropertiesAreReturned) {
  for (auto device : devices) {
    uint32_t p_count = 0;
    auto pci_bars = lzt::get_pci_bars_extension(device, &p_count);
    for (auto pci_bar : pci_bars) {
      EXPECT_GE(pci_bar.type, ZES_PCI_BAR_TYPE_MMIO);
      EXPECT_LE(pci_bar.type, ZES_PCI_BAR_TYPE_MEM);
      EXPECT_GE((UINT64_MAX - pci_bar.base), pci_bar.size);
      EXPECT_GE(pci_bar.index, 0);
      EXPECT_LE(pci_bar.index, MAX_BARS); // Check for no. of PCI bars
      EXPECT_GT(pci_bar.size, 0);
      zes_pci_bar_properties_1_2_t *pBarPropsExt =
          static_cast<zes_pci_bar_properties_1_2_t *>(pci_bar.pNext);
      EXPECT_GE(pBarPropsExt->resizableBarSupported, 0);
      EXPECT_LE(pBarPropsExt->resizableBarSupported, 1);
      EXPECT_GE(pBarPropsExt->resizableBarEnabled, 0);
      EXPECT_LE(pBarPropsExt->resizableBarEnabled, 1);
    }
  }
}
TEST_F(PciModuleTest,
       GivenSysmanHandleWhenRetrievingPCIStatsThenStatsAreReturned) {
  for (auto device : devices) {
    auto pciProps = lzt::get_pci_properties(device);
    auto pci_stats_initial = lzt::get_pci_stats(device);
    EXPECT_LE(pci_stats_initial.txCounter, UINT64_MAX);
    EXPECT_LE(pci_stats_initial.rxCounter, UINT64_MAX);
    if (pciProps.haveReplayCounters == true)
      EXPECT_LE(pci_stats_initial.replayCounter, UINT64_MAX);
    if (pciProps.havePacketCounters == true)
      EXPECT_LE(pci_stats_initial.packetCounter, UINT64_MAX);
    EXPECT_LE(pci_stats_initial.timestamp, UINT64_MAX);
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
    auto pci_stats_later = lzt::get_pci_stats(device);
    EXPECT_LE(pci_stats_later.txCounter, UINT64_MAX);
    EXPECT_LE(pci_stats_later.rxCounter, UINT64_MAX);
    EXPECT_LE(pci_stats_later.replayCounter, UINT64_MAX);
    EXPECT_LE(pci_stats_later.packetCounter, UINT64_MAX);
    EXPECT_LE(pci_stats_later.timestamp, UINT64_MAX);
    if (pciProps.haveBandwidthCounters == true) {
      EXPECT_GT(pci_stats_later.txCounter, pci_stats_initial.txCounter);
      EXPECT_GT(pci_stats_later.rxCounter, pci_stats_initial.rxCounter);
    }
    if (pciProps.havePacketCounters == true) {
      EXPECT_GT(pci_stats_later.packetCounter, pci_stats_initial.packetCounter);
    }
    if (pciProps.haveReplayCounters == true) {
      EXPECT_GE(pci_stats_later.replayCounter, pci_stats_initial.replayCounter);
    }
    EXPECT_NE(pci_stats_later.timestamp, pci_stats_initial.timestamp);
  }
}
} // namespace
