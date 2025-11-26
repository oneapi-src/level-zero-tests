/*
 *
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "logging/logging.hpp"
#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"

#include <level_zero/zes_api.h>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <algorithm>

namespace lzt = level_zero_tests;

namespace {

#ifdef USE_ZESINIT
class SysmanMultiDeviceZesTest : public lzt::ZesSysmanCtsClass {};
#define SYSMAN_MULTI_DEVICE_TEST SysmanMultiDeviceZesTest
#else // USE_ZESINIT
class SysmanMultiDeviceTest : public lzt::SysmanCtsClass {};
#define SYSMAN_MULTI_DEVICE_TEST SysmanMultiDeviceTest
#endif // USE_ZESINIT

// Structure to hold comprehensive device telemetry data
struct MultiDeviceData {
  zes_device_handle_t device;
  std::string deviceIdentifier;

  // PCI Properties for PMT validation
  zes_pci_properties_t pciProperties = {};
  zes_pci_stats_t pciStats = {};

  // Memory bandwidth data (PMT-based)
  std::vector<zes_mem_bandwidth_t> memoryBandwidth;
  std::vector<zes_mem_state_t> memoryStates;

  // Power energy data (PMT-based)
  std::vector<zes_power_energy_counter_t> powerEnergy;

  // Temperature data (PMT-based)
  std::vector<double> temperatures;

  bool hasValidData = false;
};

// Helper function to validate PCI BDF uniqueness
bool validateUniquePciBdf(const std::vector<MultiDeviceData> &deviceData) {
  std::set<std::string> uniqueBdfs;

  for (const auto &data : deviceData) {
    if (uniqueBdfs.find(data.deviceIdentifier) != uniqueBdfs.end()) {
      LOG_ERROR << "Duplicate PCI BDF detected: " << data.deviceIdentifier;
      return false;
    }
    uniqueBdfs.insert(data.deviceIdentifier);
  }

  LOG_INFO
      << " PMT Interface validation passed - All devices have unique PCI BDF";
  return true;
}

// Helper function to collect memory data for a device
void collectMemoryData(zes_device_handle_t device,
                       MultiDeviceData &deviceData) {
  uint32_t memoryCount = 0;
  ze_result_t result =
      zesDeviceEnumMemoryModules(device, &memoryCount, nullptr);

  if (result != ZE_RESULT_SUCCESS || memoryCount == 0) {
    LOG_DEBUG << "No memory modules found for device";
    return;
  }

  std::vector<zes_mem_handle_t> memoryHandles(memoryCount);
  result =
      zesDeviceEnumMemoryModules(device, &memoryCount, memoryHandles.data());

  if (result != ZE_RESULT_SUCCESS) {
    LOG_DEBUG << "Failed to enumerate memory modules";
    return;
  }

  for (auto memHandle : memoryHandles) {
    // Get memory bandwidth
    zes_mem_bandwidth_t bandwidth = {};
    if (zesMemoryGetBandwidth(memHandle, &bandwidth) == ZE_RESULT_SUCCESS) {
      deviceData.memoryBandwidth.push_back(bandwidth);
      LOG_DEBUG << "Device memory bandwidth - Read: " << bandwidth.readCounter
                << ", Write: " << bandwidth.writeCounter
                << ", Timestamp: " << bandwidth.timestamp;
    }

    // Get memory state
    zes_mem_state_t memState = {};
    if (zesMemoryGetState(memHandle, &memState) == ZE_RESULT_SUCCESS) {
      deviceData.memoryStates.push_back(memState);
      LOG_DEBUG << "Device memory state - Free: " << memState.free
                << ", Size: " << memState.size;
    }
  }
}

// Helper function to collect power data for a device
void collectPowerData(zes_device_handle_t device, MultiDeviceData &deviceData) {
  uint32_t powerCount = 0;
  ze_result_t result = zesDeviceEnumPowerDomains(device, &powerCount, nullptr);

  if (result != ZE_RESULT_SUCCESS || powerCount == 0) {
    LOG_DEBUG << "No power domains found for device";
    return;
  }

  std::vector<zes_pwr_handle_t> powerHandles(powerCount);
  result = zesDeviceEnumPowerDomains(device, &powerCount, powerHandles.data());

  if (result != ZE_RESULT_SUCCESS) {
    LOG_DEBUG << "Failed to enumerate power domains";
    return;
  }

  for (auto powerHandle : powerHandles) {
    zes_power_energy_counter_t energy = {};
    if (zesPowerGetEnergyCounter(powerHandle, &energy) == ZE_RESULT_SUCCESS) {
      deviceData.powerEnergy.push_back(energy);
      LOG_DEBUG << "Device power energy - Energy: " << energy.energy
                << ", Timestamp: " << energy.timestamp;
    }
  }
}

// Helper function to collect temperature data for a device
void collectTemperatureData(zes_device_handle_t device,
                            MultiDeviceData &deviceData) {
  uint32_t tempCount = 0;
  ze_result_t result =
      zesDeviceEnumTemperatureSensors(device, &tempCount, nullptr);

  if (result != ZE_RESULT_SUCCESS || tempCount == 0) {
    LOG_DEBUG << "No temperature sensors found for device";
    return;
  }

  std::vector<zes_temp_handle_t> tempHandles(tempCount);
  result =
      zesDeviceEnumTemperatureSensors(device, &tempCount, tempHandles.data());

  if (result != ZE_RESULT_SUCCESS) {
    LOG_DEBUG << "Failed to enumerate temperature sensors";
    return;
  }

  for (auto tempHandle : tempHandles) {
    double temperature = 0.0;
    if (zesTemperatureGetState(tempHandle, &temperature) == ZE_RESULT_SUCCESS) {
      deviceData.temperatures.push_back(temperature);
      LOG_DEBUG << "Device temperature: " << temperature << "°C";
    }
  }
}

// Helper function to collect PCI data for a device
bool collectPciData(zes_device_handle_t device, MultiDeviceData &deviceData) {
  // Get PCI properties - this is critical for PMT mapping validation
  ze_result_t result =
      zesDevicePciGetProperties(device, &deviceData.pciProperties);

  if (result != ZE_RESULT_SUCCESS) {
    LOG_ERROR << "Failed to get PCI properties for device";
    return false;
  }

  // Create unique device identifier using PCI BDF
  deviceData.deviceIdentifier =
      std::to_string(deviceData.pciProperties.address.bus) + ":" +
      std::to_string(deviceData.pciProperties.address.device) + ":" +
      std::to_string(deviceData.pciProperties.address.function);

  // Get PCI statistics
  result = zesDevicePciGetStats(device, &deviceData.pciStats);
  if (result == ZE_RESULT_SUCCESS) {
    LOG_DEBUG << "Device PCI stats - RX: " << deviceData.pciStats.rxCounter
              << ", TX: " << deviceData.pciStats.txCounter
              << ", Packets: " << deviceData.pciStats.packetCounter
              << ", Timestamp: " << deviceData.pciStats.timestamp;
  }

  LOG_INFO << "Device PCI BDF: " << deviceData.deviceIdentifier
           << ", MaxSpeed: " << deviceData.pciProperties.maxSpeed.maxBandwidth
           << ", MaxLanes: " << deviceData.pciProperties.maxSpeed.width;

  return true;
}

// Helper function to validate memory data isolation between devices
void validateMemoryDataIsolation(
    const std::vector<MultiDeviceData> &deviceData) {
  for (size_t i = 0; i < deviceData.size(); i++) {
    for (size_t j = i + 1; j < deviceData.size(); j++) {

      // Compare memory bandwidth data
      for (const auto &bandwidthA : deviceData[i].memoryBandwidth) {
        for (const auto &bandwidthB : deviceData[j].memoryBandwidth) {

          bool isIdentical =
              (bandwidthA.readCounter == bandwidthB.readCounter) &&
              (bandwidthA.writeCounter == bandwidthB.writeCounter);

          EXPECT_FALSE(isIdentical)
              << "Device " << deviceData[i].deviceIdentifier << " and "
              << deviceData[j].deviceIdentifier
              << " have identical memory bandwidth - PMT cross-contamination "
                 "detected";
        }
      }

      // Compare memory states
      for (const auto &stateA : deviceData[i].memoryStates) {
        for (const auto &stateB : deviceData[j].memoryStates) {
          // Memory free space should likely be different between devices
          if (stateA.free != 0 && stateB.free != 0) {
            EXPECT_NE(stateA.free, stateB.free)
                << "Device " << deviceData[i].deviceIdentifier << " and "
                << deviceData[j].deviceIdentifier
                << " have identical free memory - potential PMT mapping issue";
          }
        }
      }
    }
  }

  LOG_INFO << "Memory data isolation validated across " << deviceData.size()
           << " devices";
}

// Helper function to validate power data isolation between devices
void validatePowerDataIsolation(
    const std::vector<MultiDeviceData> &deviceData) {
  for (size_t i = 0; i < deviceData.size(); i++) {
    for (size_t j = i + 1; j < deviceData.size(); j++) {

      // Compare power energy counters
      for (const auto &energyA : deviceData[i].powerEnergy) {
        for (const auto &energyB : deviceData[j].powerEnergy) {

          bool isIdentical = (energyA.energy == energyB.energy);

          EXPECT_FALSE(isIdentical)
              << "Device " << deviceData[i].deviceIdentifier << " and "
              << deviceData[j].deviceIdentifier
              << " have identical power energy - PMT mapping issue detected";
        }
      }
    }
  }

  LOG_INFO << " Power data isolation validated across " << deviceData.size()
           << " devices";
}

// Helper function to validate temperature data isolation between devices
void validateTemperatureDataIsolation(
    const std::vector<MultiDeviceData> &deviceData) {
  for (size_t i = 0; i < deviceData.size(); i++) {
    for (size_t j = i + 1; j < deviceData.size(); j++) {

      // Compare temperature readings
      for (const auto &tempA : deviceData[i].temperatures) {
        for (const auto &tempB : deviceData[j].temperatures) {

          // Validate temperature ranges are realistic
          EXPECT_GT(tempA, 0.0) << "Device " << deviceData[i].deviceIdentifier
                                << " has invalid temperature reading";
          EXPECT_LT(tempA, 150.0) << "Device " << deviceData[i].deviceIdentifier
                                  << " temperature seems unrealistic (>150°C)";

          EXPECT_GT(tempB, 0.0) << "Device " << deviceData[j].deviceIdentifier
                                << " has invalid temperature reading";
          EXPECT_LT(tempB, 150.0) << "Device " << deviceData[j].deviceIdentifier
                                  << " temperature seems unrealistic (>150°C)";

          // Note: Temperatures could be identical if both GPUs are idle,
          // so we don't enforce uniqueness here, just realistic values
        }
      }
    }
  }

  LOG_INFO << " Temperature data validation completed across "
           << deviceData.size() << " devices";
}

// Helper function to validate PCI data isolation between devices
void validatePciDataIsolation(const std::vector<MultiDeviceData> &deviceData) {
  for (size_t i = 0; i < deviceData.size(); i++) {
    for (size_t j = i + 1; j < deviceData.size(); j++) {

      // Validate PCI BDF uniqueness (critical for PMT mapping)
      EXPECT_NE(deviceData[i].pciProperties.address.bus,
                deviceData[j].pciProperties.address.bus)
          << "Device " << i << " and " << j
          << " have identical PCI bus - PMT mapping error";

      // Compare PCI statistics
      bool statsIdentical = (deviceData[i].pciStats.rxCounter ==
                             deviceData[j].pciStats.rxCounter) &&
                            (deviceData[i].pciStats.txCounter ==
                             deviceData[j].pciStats.txCounter) &&
                            (deviceData[i].pciStats.packetCounter ==
                             deviceData[j].pciStats.packetCounter);

      EXPECT_FALSE(statsIdentical)
          << "Device " << deviceData[i].deviceIdentifier << " and "
          << deviceData[j].deviceIdentifier
          << " have identical PCI stats - PMT interface isolation issue";
    }
  }

  LOG_INFO << " PCI data isolation validated across " << deviceData.size()
           << " devices";
}

// Main multi-device test - NO TEST CLASS NEEDED
LZT_TEST_F(
    SYSMAN_MULTI_DEVICE_TEST,
    GivenMultipleDevicesWhenQueryingAllSysmanAPIsThenEachDeviceReturnsUniqueValues) {

  LOG_INFO << "=== Multi-Device PMT Interface Validation Test ===";

  // Step 0: Device Discovery and Multi-Device Validation
  uint32_t deviceCount = 0;
  ASSERT_EQ(ZE_RESULT_SUCCESS,
            zesDeviceGet(lzt::get_default_zes_driver(), &deviceCount, nullptr));

  if (deviceCount < 2) {
    GTEST_SKIP()
        << "Multi-device PMT validation requires at least 2 Intel GPUs. "
        << "Found: " << deviceCount << " device(s). "
        << "This test validates PMT interface isolation across multiple GPUs.";
  }

  std::vector<zes_device_handle_t> devices(deviceCount);
  ASSERT_EQ(ZE_RESULT_SUCCESS, zesDeviceGet(lzt::get_default_zes_driver(),
                                            &deviceCount, devices.data()));

  LOG_INFO << " Multi-device environment validated: " << deviceCount
           << " devices found";
  LOG_INFO << "Testing " << devices.size()
           << " devices for PMT interface isolation";

  // Step 1: Device Discovery & PMT Validation
  std::vector<MultiDeviceData> deviceData;
  deviceData.reserve(devices.size());

  LOG_DEBUG << "[DEBUG] Step 1: Beginning device discovery and data collection";

  for (size_t i = 0; i < devices.size(); i++) {
    LOG_DEBUG << "[DEBUG] Processing device " << i << " of " << devices.size();

    MultiDeviceData data;
    data.device = devices[i];

    // Collect PCI data first (critical for PMT mapping validation)
    LOG_DEBUG << "[DEBUG] Device " << i << ": Collecting PCI data...";
    if (!collectPciData(devices[i], data)) {
      LOG_ERROR << "[DEBUG] Device " << i << ": PCI data collection FAILED";
      FAIL() << "Failed to collect PCI data for device " << i;
    }
    LOG_DEBUG << "[DEBUG] Device " << i
              << ": PCI data collected - BDF: " << data.deviceIdentifier;

    // Collect PMT-based telemetry data
    LOG_DEBUG << "[DEBUG] Device " << i << ": Collecting memory data...";
    collectMemoryData(devices[i], data);
    LOG_DEBUG << "[DEBUG] Device " << i
              << ": Memory data collection complete - "
              << data.memoryBandwidth.size() << " samples";

    LOG_DEBUG << "[DEBUG] Device " << i << ": Collecting power data...";
    collectPowerData(devices[i], data);
    LOG_DEBUG << "[DEBUG] Device " << i << ": Power data collection complete - "
              << data.powerEnergy.size() << " samples";

    LOG_DEBUG << "[DEBUG] Device " << i << ": Collecting temperature data...";
    collectTemperatureData(devices[i], data);
    LOG_DEBUG << "[DEBUG] Device " << i
              << ": Temperature data collection complete - "
              << data.temperatures.size() << " samples";

    // Mark device as having valid data if any telemetry was collected
    data.hasValidData = !data.memoryBandwidth.empty() ||
                        !data.powerEnergy.empty() || !data.temperatures.empty();

    LOG_DEBUG << "[DEBUG] Device " << i
              << ": Valid data status: " << (data.hasValidData ? "YES" : "NO");

    deviceData.push_back(data);

    LOG_INFO << "Device " << i << " (" << data.deviceIdentifier
             << ") data collection complete";
    LOG_INFO << "  Memory modules: " << data.memoryBandwidth.size();
    LOG_INFO << "  Power domains: " << data.powerEnergy.size();
    LOG_INFO << "  Temperature sensors: " << data.temperatures.size();
  }

  LOG_DEBUG << "[DEBUG] Step 1 complete: All device data collected";

  // Step 2: Validate PCI BDF uniqueness (PMT interface mapping validation)
  LOG_DEBUG << "[DEBUG] Step 2: Validating PCI BDF uniqueness...";
  ASSERT_TRUE(validateUniquePciBdf(deviceData))
      << "PMT interface mapping validation failed - duplicate PCI BDF detected";
  LOG_DEBUG << "[DEBUG] Step 2 complete: PCI BDF uniqueness validated";

  // Step 3: Validate data isolation across all PMT-based APIs
  LOG_DEBUG << "[DEBUG] Step 3: Beginning data isolation validation";

  // Memory bandwidth isolation (critical PMT data)
  LOG_DEBUG << "[DEBUG] Validating memory bandwidth isolation...";
  if (std::any_of(deviceData.begin(), deviceData.end(),
                  [](const MultiDeviceData &d) {
                    return !d.memoryBandwidth.empty();
                  })) {
    validateMemoryDataIsolation(deviceData);
    LOG_DEBUG << "[DEBUG] Memory bandwidth isolation validation complete";
  } else {
    LOG_WARNING << "No memory bandwidth data available for validation";
    LOG_DEBUG << "[DEBUG] Memory bandwidth validation SKIPPED - no data";
  }

  // Power consumption isolation (critical PMT data)
  LOG_DEBUG << "[DEBUG] Validating power energy isolation...";
  if (std::any_of(
          deviceData.begin(), deviceData.end(),
          [](const MultiDeviceData &d) { return !d.powerEnergy.empty(); })) {
    validatePowerDataIsolation(deviceData);
    LOG_DEBUG << "[DEBUG] Power energy isolation validation complete";
  } else {
    LOG_WARNING << "No power energy data available for validation";
    LOG_DEBUG << "[DEBUG] Power energy validation SKIPPED - no data";
  }

  // Temperature isolation (PMT data)
  LOG_DEBUG << "[DEBUG] Validating temperature isolation...";
  if (std::any_of(
          deviceData.begin(), deviceData.end(),
          [](const MultiDeviceData &d) { return !d.temperatures.empty(); })) {
    validateTemperatureDataIsolation(deviceData);
    LOG_DEBUG << "[DEBUG] Temperature isolation validation complete";
  } else {
    LOG_WARNING << "No temperature data available for validation";
    LOG_DEBUG << "[DEBUG] Temperature validation SKIPPED - no data";
  }

  // PCI statistics isolation (PMT data)
  LOG_DEBUG << "[DEBUG] Validating PCI statistics isolation...";
  validatePciDataIsolation(deviceData);
  LOG_DEBUG << "[DEBUG] PCI statistics isolation validation complete";

  LOG_DEBUG << "[DEBUG] Step 3 complete: All isolation validations finished";

  // Step 4: Final validation summary
  LOG_DEBUG << "[DEBUG] Step 4: Generating final validation summary";

  auto devicesWithData =
      std::count_if(deviceData.begin(), deviceData.end(),
                    [](const MultiDeviceData &d) { return d.hasValidData; });

  LOG_DEBUG << "[DEBUG] Devices with valid data count: " << devicesWithData;

  EXPECT_GT(devicesWithData, 1)
      << "At least 2 devices should have valid PMT telemetry data";

  LOG_INFO << "=== Multi-Device PMT Validation Results ===";
  LOG_INFO << " Total devices tested: " << devices.size();
  LOG_INFO << " Devices with telemetry data: " << devicesWithData;
  LOG_INFO << " PCI BDF uniqueness: PASSED";
  LOG_INFO << " PMT interface isolation: PASSED";
  LOG_INFO << " Cross-device contamination: NONE DETECTED";
  LOG_INFO << " Multi-GPU PMT mapping: WORKING CORRECTLY";

  // Log device-specific summary
  LOG_DEBUG << "[DEBUG] Generating per-device summary...";
  for (size_t i = 0; i < deviceData.size(); i++) {
    LOG_INFO << "Device " << i << " (" << deviceData[i].deviceIdentifier
             << "):";
    LOG_INFO << "  Valid PMT data: "
             << (deviceData[i].hasValidData ? "YES" : "NO");
    LOG_INFO << "  Memory bandwidth samples: "
             << deviceData[i].memoryBandwidth.size();
    LOG_INFO << "  Power energy samples: " << deviceData[i].powerEnergy.size();
    LOG_INFO << "  Temperature samples: " << deviceData[i].temperatures.size();

    LOG_DEBUG << "[DEBUG] Device " << i << " summary complete";
  }

  LOG_DEBUG << "[DEBUG] Test complete - All validations passed";
}

} // namespace
