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

  // PCI Properties for multi-device telemetry validation
  zes_pci_properties_t pciProperties = {};
  zes_pci_stats_t pciStats = {};

  // Memory bandwidth daat
  std::vector<zes_mem_bandwidth_t> memoryBandwidth;
  std::vector<zes_mem_state_t> memoryStates;

  // Power energy data
  std::vector<zes_power_energy_counter_t> powerEnergy;

  // Temperature data
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

  LOG_INFO << " validation passed - All devices have unique PCI BDF";
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

  // Store memory handles with their properties for sorting
  struct MemoryModuleInfo {
    zes_mem_handle_t handle;
    zes_mem_properties_t properties;
  };
  std::vector<MemoryModuleInfo> memoryModules;

  // First, collect all memory handles with their properties
  for (auto memHandle : memoryHandles) {
    MemoryModuleInfo moduleInfo;
    moduleInfo.handle = memHandle;

    ze_result_t propResult =
        zesMemoryGetProperties(memHandle, &moduleInfo.properties);
    if (propResult == ZE_RESULT_SUCCESS) {
      memoryModules.push_back(moduleInfo);
    }
  }

  // Sort memory modules by type and location for consistent ordering
  std::sort(memoryModules.begin(), memoryModules.end(),
            [](const MemoryModuleInfo &a, const MemoryModuleInfo &b) {
              if (a.properties.type != b.properties.type) {
                return a.properties.type < b.properties.type;
              }
              if (a.properties.location != b.properties.location) {
                return a.properties.location < b.properties.location;
              }
              return a.properties.physicalSize < b.properties.physicalSize;
            });

  // Now collect bandwidth and state data in sorted order
  for (const auto &module : memoryModules) {
    // Get memory bandwidth
    zes_mem_bandwidth_t bandwidth = {};
    if (zesMemoryGetBandwidth(module.handle, &bandwidth) == ZE_RESULT_SUCCESS) {
      deviceData.memoryBandwidth.push_back(bandwidth);
      LOG_DEBUG << "Device memory bandwidth - Read: " << bandwidth.readCounter
                << ", Write: " << bandwidth.writeCounter
                << ", Timestamp: " << bandwidth.timestamp
                << ", Type: " << module.properties.type
                << ", Location: " << module.properties.location;
    }

    // Get memory state
    zes_mem_state_t memState = {};
    if (zesMemoryGetState(module.handle, &memState) == ZE_RESULT_SUCCESS) {
      deviceData.memoryStates.push_back(memState);
      LOG_DEBUG << "Device memory state - Free: " << memState.free
                << ", Size: " << memState.size
                << ", Type: " << module.properties.type;
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

  // Store power handles with their properties for sorting
  struct PowerDomainInfo {
    zes_pwr_handle_t handle;
    zes_power_properties_t properties;
  };
  std::vector<PowerDomainInfo> powerDomains;

  // First, collect all power handles with their properties
  for (auto powerHandle : powerHandles) {
    PowerDomainInfo domainInfo;
    domainInfo.handle = powerHandle;

    ze_result_t propResult =
        zesPowerGetProperties(powerHandle, &domainInfo.properties);
    if (propResult == ZE_RESULT_SUCCESS) {
      powerDomains.push_back(domainInfo);
    }
  }

  // Sort power domains for consistent ordering
  std::sort(powerDomains.begin(), powerDomains.end(),
            [](const PowerDomainInfo &a, const PowerDomainInfo &b) {
              if (a.properties.onSubdevice != b.properties.onSubdevice) {
                return a.properties.onSubdevice < b.properties.onSubdevice;
              }
              if (a.properties.subdeviceId != b.properties.subdeviceId) {
                return a.properties.subdeviceId < b.properties.subdeviceId;
              }
              return a.properties.defaultLimit < b.properties.defaultLimit;
            });

  // Now collect energy data in sorted order
  for (const auto &domain : powerDomains) {
    zes_power_energy_counter_t energy = {};
    if (zesPowerGetEnergyCounter(domain.handle, &energy) == ZE_RESULT_SUCCESS) {
      deviceData.powerEnergy.push_back(energy);
      LOG_DEBUG << "Device power energy - Energy: " << energy.energy
                << ", Timestamp: " << energy.timestamp
                << ", Subdevice: " << domain.properties.onSubdevice
                << ", SubdeviceId: " << domain.properties.subdeviceId;
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

  // Store temperature handles with their properties for sorting
  struct TemperatureSensorInfo {
    zes_temp_handle_t handle;
    zes_temp_properties_t properties;
  };
  std::vector<TemperatureSensorInfo> tempSensors;

  // First, collect all temperature handles with their properties
  for (auto tempHandle : tempHandles) {
    TemperatureSensorInfo sensorInfo;
    sensorInfo.handle = tempHandle;

    ze_result_t propResult =
        zesTemperatureGetProperties(tempHandle, &sensorInfo.properties);
    if (propResult == ZE_RESULT_SUCCESS) {
      tempSensors.push_back(sensorInfo);
    }
  }

  // Sort temperature sensors for consistent ordering
  std::sort(tempSensors.begin(), tempSensors.end(),
            [](const TemperatureSensorInfo &a, const TemperatureSensorInfo &b) {
              if (a.properties.type != b.properties.type) {
                return a.properties.type < b.properties.type;
              }
              if (a.properties.onSubdevice != b.properties.onSubdevice) {
                return a.properties.onSubdevice < b.properties.onSubdevice;
              }
              return a.properties.subdeviceId < b.properties.subdeviceId;
            });

  // Now collect temperature data in sorted order
  for (const auto &sensor : tempSensors) {
    double temperature = 0.0;
    if (zesTemperatureGetState(sensor.handle, &temperature) ==
        ZE_RESULT_SUCCESS) {
      deviceData.temperatures.push_back(temperature);
      LOG_DEBUG << "Device temperature: " << temperature << "°C"
                << ", Type: " << sensor.properties.type
                << ", Subdevice: " << sensor.properties.onSubdevice
                << ", SubdeviceId: " << sensor.properties.subdeviceId;
    }
  }
}

// Helper function to collect PCI data for a device
bool collectPciData(zes_device_handle_t device, MultiDeviceData &deviceData) {
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
      LOG_DEBUG << "Comparing memory data between device "
                << deviceData[i].deviceIdentifier << " and "
                << deviceData[j].deviceIdentifier;

      // Verify both devices have same number of memory modules (expected for
      // identical hardware)
      EXPECT_EQ(deviceData[i].memoryBandwidth.size(),
                deviceData[j].memoryBandwidth.size())
          << "Device " << deviceData[i].deviceIdentifier << " and "
          << deviceData[j].deviceIdentifier
          << " have different memory module counts";

      EXPECT_EQ(deviceData[i].memoryStates.size(),
                deviceData[j].memoryStates.size())
          << "Device " << deviceData[i].deviceIdentifier << " and "
          << deviceData[j].deviceIdentifier
          << " have different memory state counts";

      // Compare memory bandwidth data with 1:1 mapping (same index = same
      // module type/location)
      size_t minBandwidthCount = std::min(deviceData[i].memoryBandwidth.size(),
                                          deviceData[j].memoryBandwidth.size());
      for (size_t k = 0; k < minBandwidthCount; k++) {
        const auto &bandwidthA = deviceData[i].memoryBandwidth[k];
        const auto &bandwidthB = deviceData[j].memoryBandwidth[k];

        bool isIdentical = (bandwidthA.readCounter == bandwidthB.readCounter) &&
                           (bandwidthA.writeCounter == bandwidthB.writeCounter);

        EXPECT_FALSE(isIdentical)
            << "Device " << deviceData[i].deviceIdentifier << " and "
            << deviceData[j].deviceIdentifier
            << " have identical memory bandwidth for module " << k
            << " - cross-contamination detected"
            << " (Read: " << bandwidthA.readCounter
            << ", Write: " << bandwidthA.writeCounter << ")";
      }

      // FIX: Compare memory states with 1:1 mapping (not nested loops!)
      size_t minStateCount = std::min(deviceData[i].memoryStates.size(),
                                      deviceData[j].memoryStates.size());
      for (size_t k = 0; k < minStateCount; k++) {
        const auto &stateA = deviceData[i].memoryStates[k];
        const auto &stateB = deviceData[j].memoryStates[k];

        // Memory free space should be different between devices for same module
        // index
        if (stateA.free != 0 && stateB.free != 0) {
          EXPECT_NE(stateA.free, stateB.free)
              << "Device " << deviceData[i].deviceIdentifier << " and "
              << deviceData[j].deviceIdentifier
              << " have identical free memory for module " << k
              << " - potential mapping issue (Free: " << stateA.free
              << " bytes)";
        }
      }
    }
  }

  LOG_INFO << "Memory data isolation validated across " << deviceData.size()
           << " devices with sorted module ordering";
}

// Helper function to validate power data isolation between devices
void validatePowerDataIsolation(
    const std::vector<MultiDeviceData> &deviceData) {
  for (size_t i = 0; i < deviceData.size(); i++) {
    for (size_t j = i + 1; j < deviceData.size(); j++) {
      LOG_DEBUG << "Comparing power data between device "
                << deviceData[i].deviceIdentifier << " and "
                << deviceData[j].deviceIdentifier;

      // FIX: Verify both devices have same number of power domains
      EXPECT_EQ(deviceData[i].powerEnergy.size(),
                deviceData[j].powerEnergy.size())
          << "Device " << deviceData[i].deviceIdentifier << " and "
          << deviceData[j].deviceIdentifier
          << " have different power domain counts";

      // FIX: Compare power energy counters with 1:1 mapping (same index =
      // same domain type)
      size_t minPowerCount = std::min(deviceData[i].powerEnergy.size(),
                                      deviceData[j].powerEnergy.size());
      for (size_t k = 0; k < minPowerCount; k++) {
        const auto &energyA = deviceData[i].powerEnergy[k];
        const auto &energyB = deviceData[j].powerEnergy[k];

        bool isIdentical = (energyA.energy == energyB.energy);

        EXPECT_FALSE(isIdentical) << "Device " << deviceData[i].deviceIdentifier
                                  << " and " << deviceData[j].deviceIdentifier
                                  << " have identical power energy for domain "
                                  << k << " - mapping issue detected"
                                  << " (Energy: " << energyA.energy << ")";
      }
    }
  }

  LOG_INFO << "Power data isolation validated across " << deviceData.size()
           << " devices with sorted domain ordering";
}

// Helper function to validate temperature data isolation between devices
void validateTemperatureDataIsolation(
    const std::vector<MultiDeviceData> &deviceData) {
  for (size_t i = 0; i < deviceData.size(); i++) {
    for (size_t j = i + 1; j < deviceData.size(); j++) {
      LOG_DEBUG << "Comparing temperature data between device "
                << deviceData[i].deviceIdentifier << " and "
                << deviceData[j].deviceIdentifier;

      // Verify both devices have same number of temperature sensors
      EXPECT_EQ(deviceData[i].temperatures.size(),
                deviceData[j].temperatures.size())
          << "Device " << deviceData[i].deviceIdentifier << " and "
          << deviceData[j].deviceIdentifier
          << " have different temperature sensor counts";

      // Compare temperature readings with 1:1 mapping (same index = same sensor
      // type)
      size_t minTempCount = std::min(deviceData[i].temperatures.size(),
                                     deviceData[j].temperatures.size());
      for (size_t k = 0; k < minTempCount; k++) {
        const auto &tempA = deviceData[i].temperatures[k];
        const auto &tempB = deviceData[j].temperatures[k];

        // Temperatures can be identical if both GPUs are idle - log as warning
        // instead of failure
        if (tempA == tempB) {
          LOG_WARNING << "Device " << deviceData[i].deviceIdentifier << " and "
                      << deviceData[j].deviceIdentifier
                      << " have identical temperature for sensor " << k << " ("
                      << tempA << "°C) - this is acceptable if GPUs are idle";
        } else {
          LOG_DEBUG << "Sensor " << k << " temps - Device "
                    << deviceData[i].deviceIdentifier << ": " << tempA
                    << "°C, Device " << deviceData[j].deviceIdentifier << ": "
                    << tempB << "°C";
        }
      }
    }
  }

  LOG_INFO << "✅ Temperature data validation completed across "
           << deviceData.size() << " devices with sorted sensor ordering";
}

// Helper function to validate PCI data isolation between devices
void validatePciDataIsolation(const std::vector<MultiDeviceData> &deviceData) {
  for (size_t i = 0; i < deviceData.size(); i++) {
    for (size_t j = i + 1; j < deviceData.size(); j++) {

      // Validate PCI BDF uniqueness
      EXPECT_NE(deviceData[i].pciProperties.address.bus,
                deviceData[j].pciProperties.address.bus)
          << "Device " << i << " and " << j
          << " have identical PCI bus - mapping error";

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
          << " have identical PCI stats - isolation issue";
    }
  }

  LOG_INFO << " PCI data isolation validated across " << deviceData.size()
           << " devices";
}

LZT_TEST_F(
    SYSMAN_MULTI_DEVICE_TEST,
    GivenMultipleDevicesWhenQueryingAllSysmanAPIsThenEachDeviceReturnsUniqueValues) {

  LOG_INFO << "=== Multi-Device Telemetry Validation Test ===";

  // Device Discovery and Multi-Device Validation
  uint32_t deviceCount = 0;
  ASSERT_ZE_RESULT_SUCCESS(
      zesDeviceGet(lzt::get_default_zes_driver(), &deviceCount, nullptr));

  if (deviceCount < 2) {
    GTEST_SKIP()
        << "Multi-device validation requires at least 2 Intel GPUs. "
        << "Found: " << deviceCount << " device(s). "
        << "This test validates interface isolation across multiple GPUs.";
  }

  std::vector<zes_device_handle_t> devices(deviceCount);
  ASSERT_ZE_RESULT_SUCCESS(zesDeviceGet(lzt::get_default_zes_driver(),
                                        &deviceCount, devices.data()));

  LOG_INFO << "Multi-device environment validated: " << deviceCount
           << " devices found";

  // Device Discovery & Validation
  std::vector<MultiDeviceData> deviceData;
  deviceData.reserve(devices.size());

  LOG_DEBUG << "Beginning device discovery and data collection";

  for (size_t i = 0; i < devices.size(); i++) {
    LOG_DEBUG << "Processing device " << i << " of " << devices.size();

    MultiDeviceData data;
    data.device = devices[i];

    // Collect PCI data first
    if (!collectPciData(devices[i], data)) {
      FAIL() << "Failed to collect PCI data for device " << i;
    }

    // Collect Multi-device telemetry data in sorted order
    collectMemoryData(devices[i], data);
    collectPowerData(devices[i], data);
    collectTemperatureData(devices[i], data);

    // FIX: Mark device as having valid data if any telemetry was collected
    data.hasValidData = !data.memoryBandwidth.empty() ||
                        !data.memoryStates.empty() ||
                        !data.powerEnergy.empty() || !data.temperatures.empty();

    if (data.hasValidData) {
      deviceData.push_back(data);
      LOG_INFO << "Device " << i << " (" << data.deviceIdentifier
               << ") data collection complete";
      LOG_INFO << "  Memory bandwidth: " << data.memoryBandwidth.size();
      LOG_INFO << "  Memory states: " << data.memoryStates.size();
      LOG_INFO << "  Power domains: " << data.powerEnergy.size();
      LOG_INFO << "  Temperature sensors: " << data.temperatures.size();
    } else {
      LOG_WARNING << "Device " << i << " has no valid telemetry data";
    }
  }

  // FIX:  Validate PCI BDF uniqueness (critical)
  ASSERT_TRUE(validateUniquePciBdf(deviceData))
      << "PCI BDF validation failed - apping issue detected";

  validateMemoryDataIsolation(deviceData);
  validatePowerDataIsolation(deviceData);
  validateTemperatureDataIsolation(deviceData);
  validatePciDataIsolation(deviceData);
}

} // namespace
