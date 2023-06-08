/*
 *
 * Copyright (C) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "utils/utils.hpp"
#include "logging/logging.hpp"
#include "test_harness/test_harness.hpp"

#include <iostream>
#include <fstream>

namespace level_zero_tests {

ze_context_handle_t get_default_context() {
  ze_result_t result = ZE_RESULT_SUCCESS;

  static ze_context_handle_t context = nullptr;

  if (context) {
    return context;
  }

  ze_context_desc_t context_desc = {};
  context_desc.stype = ZE_STRUCTURE_TYPE_CONTEXT_DESC;
  result = zeContextCreate(get_default_driver(), &context_desc, &context);

  if (ZE_RESULT_SUCCESS != result) {
    throw std::runtime_error("zeContextCreate failed: " + to_string(result));
  }

  return context;
}

ze_driver_handle_t get_default_driver() {
  ze_result_t result = ZE_RESULT_SUCCESS;

  static ze_driver_handle_t driver = nullptr;
  int default_idx = 0;

  if (driver)
    return driver;

  char *user_driver_index = getenv("LZT_DEFAULT_DRIVER_IDX");
  if (user_driver_index != nullptr) {
    default_idx = std::stoi(user_driver_index);
  }

  std::vector<ze_driver_handle_t> drivers =
      level_zero_tests::get_all_driver_handles();
  if (drivers.size() == 0) {
    throw std::runtime_error("zeDriverGet failed: " + to_string(result));
  }

  if (default_idx >= drivers.size()) {
    LOG_ERROR << "Default Driver index " << default_idx
              << " invalid on this machine.";
    throw std::runtime_error("Get Default Driver failed");
  }
  driver = drivers[default_idx];
  if (!driver) {
    LOG_ERROR << "Invalid Driver handle at index " << default_idx;
    throw std::runtime_error("Get Default Driver failed");
  }

  LOG_INFO << "Default Driver retrieved at index " << default_idx;

  return driver;
}

ze_context_handle_t create_context() {
  return create_context(get_default_driver());
}

ze_context_handle_t create_context(ze_driver_handle_t driver) {
  ze_context_handle_t context;
  ze_context_desc_t ctxtDesc = {ZE_STRUCTURE_TYPE_CONTEXT_DESC, nullptr, 0};
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeContextCreate(driver, &ctxtDesc, &context));
  return context;
}

ze_context_handle_t create_context_ex(ze_driver_handle_t driver,
                                      std::vector<ze_device_handle_t> devices) {
  ze_context_handle_t context;
  ze_context_desc_t ctxtDesc = {ZE_STRUCTURE_TYPE_CONTEXT_DESC, nullptr, 0};
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeContextCreateEx(driver, &ctxtDesc, devices.size(), devices.data(),
                              &context));
  return context;
}

// Create a context with zeContextCreateEx that is visible to all devices on
// driver
ze_context_handle_t create_context_ex(ze_driver_handle_t driver) {
  ze_context_handle_t context;
  ze_context_desc_t ctxtDesc = {ZE_STRUCTURE_TYPE_CONTEXT_DESC, nullptr, 0};
  EXPECT_EQ(ZE_RESULT_SUCCESS,
            zeContextCreateEx(driver, &ctxtDesc, 0, nullptr, &context));
  return context;
}

void destroy_context(ze_context_handle_t context) {
  EXPECT_EQ(ZE_RESULT_SUCCESS, zeContextDestroy(context));
}

ze_device_handle_t get_default_device(ze_driver_handle_t driver) {
  ze_result_t result = ZE_RESULT_SUCCESS;

  static ze_device_handle_t device = nullptr;
  int default_idx = 0;
  char *default_name = nullptr;
  if (device)
    return device;

  char *user_device_index = getenv("LZT_DEFAULT_DEVICE_IDX");
  if (user_device_index != nullptr) {
    default_idx = std::stoi(user_device_index);
  }
  default_name = getenv("LZT_DEFAULT_DEVICE_NAME");

  std::vector<ze_device_handle_t> devices =
      level_zero_tests::get_ze_devices(driver);
  if (devices.size() == 0) {
    throw std::runtime_error("zeDeviceGet failed: " + to_string(result));
  }

  if (default_name != nullptr) {
    LOG_INFO << "Default Device to use has NAME:" << default_name;
    for (auto d : devices) {
      ze_device_properties_t device_props =
          level_zero_tests::get_device_properties(d);
      LOG_TRACE << "Device Name :" << device_props.name;
      if (strcmp(default_name, device_props.name) == 0) {
        device = d;
        break;
      }
    }
    if (!device) {
      LOG_ERROR << "Default Device name " << default_name
                << " invalid on this machine.";
      throw std::runtime_error("Get Default Device failed");
    }
  } else {
    if (default_idx >= devices.size()) {
      LOG_ERROR << "Default Device index " << default_idx
                << " invalid on this machine.";
      throw std::runtime_error("Get Default Device failed");
    }
    device = devices[default_idx];
    LOG_INFO << "Default Device retrieved at index " << default_idx;
  }
  return device;
}

uint32_t get_device_count(ze_driver_handle_t driver) {
  uint32_t count = 0;
  ze_result_t result = zeDeviceGet(driver, &count, nullptr);

  if (result) {
    throw std::runtime_error("zeDeviceGet failed: " + to_string(result));
  }
  return count;
}

uint32_t get_driver_handle_count() {
  uint32_t count = 0;
  ze_result_t result = zeDriverGet(&count, nullptr);

  if (result) {
    throw std::runtime_error("zeDriverGet failed: " + to_string(result));
  }
  return count;
}

uint32_t get_sub_device_count(ze_device_handle_t device) {
  uint32_t count = 0;

  ze_result_t result = zeDeviceGetSubDevices(device, &count, nullptr);

  if (result) {
    throw std::runtime_error("zeDeviceGetSubDevices failed: " +
                             to_string(result));
  }
  return count;
}

std::vector<ze_driver_handle_t> get_all_driver_handles() {
  ze_result_t result = ZE_RESULT_SUCCESS;
  uint32_t driver_handle_count = get_driver_handle_count();

  std::vector<ze_driver_handle_t> driver_handles(driver_handle_count);

  result = zeDriverGet(&driver_handle_count, driver_handles.data());

  if (result) {
    throw std::runtime_error("zeDriverGet failed: " + to_string(result));
  }
  return driver_handles;
}

std::vector<ze_device_handle_t> get_devices(ze_driver_handle_t driver) {

  ze_result_t result = ZE_RESULT_SUCCESS;

  uint32_t device_count = get_device_count(driver);
  std::vector<ze_device_handle_t> devices(device_count);

  result = zeDeviceGet(driver, &device_count, devices.data());

  if (result) {
    throw std::runtime_error("zeDeviceGet failed: " + to_string(result));
  }
  return devices;
}

std::vector<ze_device_handle_t> get_all_sub_devices() {
  auto driver = lzt::get_default_driver();
  auto devices = lzt::get_devices(driver);

  std::vector<ze_device_handle_t> all_sub_devices;
  for (auto &device : devices) {
    auto sub_devices = lzt::get_ze_sub_devices(device);
    all_sub_devices.insert(all_sub_devices.end(), sub_devices.begin(),
                           sub_devices.end());
  }

  return all_sub_devices;
}

ze_device_handle_t find_device(ze_driver_handle_t &driver,
                               const char *device_id, bool sub_device) {
  ze_device_handle_t device = nullptr;

  for (auto &root_device : lzt::get_devices(driver)) {
    if (sub_device) {
      LOG_INFO << "[] Searching subdevices";

      for (auto &sub_device : lzt::get_ze_sub_devices(root_device)) {
        auto device_properties = lzt::get_device_properties(sub_device);
        if (strncmp(device_id, lzt::to_string(device_properties.uuid).c_str(),
                    ZE_MAX_DEVICE_NAME)) {
          continue;
        } else {
          device = sub_device;
          LOG_INFO << "Found the subdevice";
          break;
        }
      }
      if (device) {
        break;
      }
    } else {
      LOG_INFO << "[] Searching root device";

      auto device_properties = lzt::get_device_properties(root_device);
      if (strncmp(device_id, lzt::to_string(device_properties.uuid).c_str(),
                  ZE_MAX_DEVICE_NAME)) {
        continue;
      }
      device = root_device;
      LOG_INFO << "Found the device";
      break;
    }
  }
  return device;
}

void sort_devices(std::vector<ze_device_handle_t> &devices) {
  std::sort(devices.begin(), devices.end(),
            [](ze_device_handle_t &a, ze_device_handle_t &b) {
              auto device_properties_a = lzt::get_device_properties(a);
              auto device_properties_b = lzt::get_device_properties(b);

              for (int i = (ZE_MAX_DEVICE_UUID_SIZE - 1); i >= 0; i--) {
                if (device_properties_a.uuid.id[i] <
                    device_properties_b.uuid.id[i]) {
                  return true;
                } else if (device_properties_a.uuid.id[i] >
                           device_properties_b.uuid.id[i]) {
                  return false;
                }
              }
              return false;
            });
}

void print_driver_version(ze_driver_handle_t driver) {
  ze_driver_properties_t properties = {};

  properties.stype = ZE_STRUCTURE_TYPE_DRIVER_PROPERTIES;
  properties.pNext = nullptr;
  ze_result_t result = zeDriverGetProperties(driver, &properties);
  if (result) {
    std::runtime_error("zeDriverGetProperties failed: " + to_string(result));
  }
  LOG_TRACE << "Driver version retrieved";
  LOG_INFO << "Driver version: " << properties.driverVersion;
}

void print_driver_overview(const ze_driver_handle_t driver) {
  ze_result_t result = ZE_RESULT_SUCCESS;

  ze_device_properties_t device_properties = {};
  device_properties.stype = ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES;
  device_properties.pNext = nullptr;
  auto devices = get_devices(driver);
  int device_index = 0;
  LOG_INFO << "Device Count: " << devices.size();
  for (auto device : devices) {
    result = zeDeviceGetProperties(device, &device_properties);
    if (result) {
      std::runtime_error("zeDeviceGetDeviceProperties failed: " +
                         to_string(result));
    }
    LOG_TRACE << "Device properties retrieved for device " << device_index;
    LOG_INFO << "Device name: " << device_properties.name;
    device_index++;
  }

  ze_api_version_t api_version;
  result = zeDriverGetApiVersion(driver, &api_version);
  if (result) {
    throw std::runtime_error("zeDriverGetApiVersion failed: " +
                             to_string(result));
  }
  LOG_TRACE << "Driver API version retrieved";

  LOG_INFO << "Driver API version: " << to_string(api_version);
}

void print_driver_overview(const std::vector<ze_driver_handle_t> driver) {
  for (const ze_driver_handle_t driver : driver) {
    print_driver_version(driver);
    print_driver_overview(driver);
  }
}

void print_platform_overview(const std::string context) {
  LOG_INFO << "Platform overview";
  if (context.size() > 0) {
    LOG_INFO << " (Context: " << context << ")";
  }

  const std::vector<ze_driver_handle_t> drivers = get_all_driver_handles();
  LOG_INFO << "Driver Handle count: " << drivers.size();

  print_driver_overview(drivers);
}

void print_platform_overview() { print_platform_overview(""); }

std::vector<uint8_t> load_binary_file(const std::string &file_path) {
  LOG_ENTER_FUNCTION
  LOG_DEBUG << "File path: " << file_path;
  std::ifstream stream(file_path, std::ios::in | std::ios::binary);

  std::vector<uint8_t> binary_file;
  if (!stream.good()) {
    LOG_ERROR << "Failed to load binary file: " << file_path << "error "
              << strerror(errno);

    LOG_EXIT_FUNCTION
    return binary_file;
  }

  size_t length = 0;
  stream.seekg(0, stream.end);
  length = static_cast<size_t>(stream.tellg());
  stream.seekg(0, stream.beg);
  LOG_DEBUG << "Binary file length: " << length;

  binary_file.resize(length);
  stream.read(reinterpret_cast<char *>(binary_file.data()), length);
  LOG_DEBUG << "Binary file loaded";

  LOG_EXIT_FUNCTION
  return binary_file;
}

void save_binary_file(const std::vector<uint8_t> &data,
                      const std::string &file_path) {
  LOG_ENTER_FUNCTION
  LOG_DEBUG << "File path: " << file_path;

  std::ofstream stream(file_path, std::ios::out | std::ios::binary);
  stream.write(reinterpret_cast<const char *>(data.data()),
               size_in_bytes(data));

  LOG_EXIT_FUNCTION
}

uint32_t nextPowerOfTwo(uint32_t value) {
  --value;
  value |= value >> 1;
  value |= value >> 2;
  value |= value >> 4;
  value |= value >> 8;
  value |= value >> 16;
  ++value;
  return value;
}

} // namespace level_zero_tests