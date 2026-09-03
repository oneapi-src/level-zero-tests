/*
 *
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "vulkan_interop/vulkan_library.hpp"

#include "logging/logging.hpp"
#include "system/shared_library.hpp"
#include "utils/utils.hpp"

#include <algorithm>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

using level_zero_tests::to_u32;

namespace {

#ifdef __linux__
constexpr auto VULKAN_LIBRARY_NAME = "libvulkan.so.1";
#else
constexpr auto VULKAN_LIBRARY_NAME = "vulkan-1.dll";
#endif

std::unique_ptr<level_zero_tests::SharedLibrary> vulkan_lib;

VkDebugUtilsMessengerEXT vk_debug_messenger = VK_NULL_HANDLE;

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT *callback_data, void *) {
  switch (message_severity) {
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
    LOG_WARNING << "Vulkan debug callback: " << callback_data->pMessage << '\n';
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
    LOG_ERROR << "Vulkan debug callback: " << callback_data->pMessage << '\n';
    break;
  default:
    LOG_INFO << "Vulkan debug callback: " << callback_data->pMessage << '\n';
  }

  return VK_FALSE;
}

std::string api_version_to_string(uint32_t version) {
  return std::to_string(VK_API_VERSION_MAJOR(version)) + "." +
         std::to_string(VK_API_VERSION_MINOR(version)) + "." +
         std::to_string(VK_API_VERSION_PATCH(version));
}

bool load_instance_level_functions(VkInstance instance) {
#define LOAD_VULKAN_FUNCTION(NAME)                                             \
  NAME = reinterpret_cast<PFN_##NAME>(vkGetInstanceProcAddr(instance, #NAME)); \
  if (NAME == nullptr) {                                                       \
    LOG_WARNING << "Failed to load Vulkan function: " #NAME;                   \
    return false;                                                              \
  }
  FOR_VULKAN_INSTANCE_FUNCTIONS(LOAD_VULKAN_FUNCTION)
#undef LOAD_VULKAN_FUNCTION

  return true;
}

} // namespace

namespace vlk {

bool initialize_loader() {
  try {
    if (!vulkan_lib) {
      vulkan_lib = std::make_unique<level_zero_tests::SharedLibrary>(
          VULKAN_LIBRARY_NAME);
    }
    vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(
        vulkan_lib->get_function_ptr("vkGetInstanceProcAddr"));
  } catch (const std::exception &e) {
    LOG_WARNING << "Failed to load Vulkan library: " << e.what();
    return false;
  }

  vkEnumerateInstanceVersion = reinterpret_cast<PFN_vkEnumerateInstanceVersion>(
      vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceVersion"));

#define LOAD_VULKAN_FUNCTION(NAME)                                             \
  NAME = reinterpret_cast<PFN_##NAME>(vkGetInstanceProcAddr(nullptr, #NAME));  \
  if (NAME == nullptr) {                                                       \
    LOG_WARNING << "Failed to load Vulkan function: " #NAME;                   \
    return false;                                                              \
  }
  FOR_VULKAN_GLOBAL_FUNCTIONS(LOAD_VULKAN_FUNCTION)
#undef LOAD_VULKAN_FUNCTION

  return true;
}

VkInstance create_instance() {
  uint32_t instance_version = VK_API_VERSION_1_0;
  if (vkEnumerateInstanceVersion != nullptr) {
    VK_CHECK(vkEnumerateInstanceVersion(&instance_version));
  }
  if (instance_version < REQUIRED_API_VERSION) {
    throw LztGtestSkipExecutionException(
        "Vulkan instance version " + api_version_to_string(instance_version) +
        " is lower than the required " +
        api_version_to_string(REQUIRED_API_VERSION) + " - skipping.");
  }

  VkApplicationInfo app_info = {};
  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pApplicationName = "lzt_app";
  app_info.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
  app_info.pEngineName = "lzt";
  app_info.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
  app_info.apiVersion = REQUIRED_API_VERSION;

  uint32_t layer_count = 0;
  VK_CHECK(vkEnumerateInstanceLayerProperties(&layer_count, nullptr));
  std::vector<VkLayerProperties> available_layers(layer_count);
  VK_CHECK(vkEnumerateInstanceLayerProperties(&layer_count,
                                              available_layers.data()));

  std::vector<const char *> layers;
  if (std::any_of(available_layers.begin(), available_layers.end(),
                  [](const VkLayerProperties &layer) {
                    return strcmp(layer.layerName,
                                  "VK_LAYER_KHRONOS_validation") == 0;
                  })) {
    layers.push_back("VK_LAYER_KHRONOS_validation");
    LOG_INFO << "Enabled Vulkan validation layer.";
  }

  uint32_t extension_count = 0;
  VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &extension_count,
                                                  nullptr));
  std::vector<VkExtensionProperties> available_extensions(extension_count);
  VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &extension_count,
                                                  available_extensions.data()));

  const bool debug_utils_available =
      std::any_of(available_extensions.begin(), available_extensions.end(),
                  [](const VkExtensionProperties &ext) {
                    return strcmp(ext.extensionName,
                                  VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0;
                  });

  std::vector<const char *> extensions;
  if (debug_utils_available) {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  } else {
    LOG_WARNING << VK_EXT_DEBUG_UTILS_EXTENSION_NAME
                << " is not available; continuing without Vulkan debug "
                   "messages.";
  }

  VkDebugUtilsMessengerCreateInfoEXT dbg_msg_create_info = {};
  dbg_msg_create_info.sType =
      VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  dbg_msg_create_info.messageSeverity =
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  dbg_msg_create_info.messageType =
      VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  dbg_msg_create_info.pfnUserCallback = debug_callback;

  VkInstanceCreateInfo instance_create_info = {};
  instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instance_create_info.pApplicationInfo = &app_info;
  instance_create_info.enabledLayerCount = to_u32(layers.size());
  instance_create_info.ppEnabledLayerNames = layers.data();
  instance_create_info.enabledExtensionCount = to_u32(extensions.size());
  instance_create_info.ppEnabledExtensionNames = extensions.data();
  instance_create_info.pNext =
      debug_utils_available ? &dbg_msg_create_info : nullptr;

  VkInstance instance = VK_NULL_HANDLE;
  const VkResult create_result =
      vkCreateInstance(&instance_create_info, nullptr, &instance);

  if (create_result == VK_ERROR_INCOMPATIBLE_DRIVER) {
    throw LztGtestSkipExecutionException(
        "No Vulkan driver compatible with the required API version is "
        "available - skipping.");
  }

  if (create_result != VK_SUCCESS) {
    throw std::runtime_error("vkCreateInstance failed with: " +
                             std::to_string(create_result));
  }

  // vkDestroyInstance may itself have failed to load, so resolve it directly
  // for cleanup rather than relying on the global pointer.
  const auto destroy_partial_instance = [](VkInstance partial_instance) {
    auto pfn_destroy_instance = reinterpret_cast<PFN_vkDestroyInstance>(
        vkGetInstanceProcAddr(partial_instance, "vkDestroyInstance"));
    if (pfn_destroy_instance != nullptr) {
      pfn_destroy_instance(partial_instance, nullptr);
    }
  };

  if (!load_instance_level_functions(instance)) {
    destroy_partial_instance(instance);
    throw std::runtime_error(
        "Failed to load required Vulkan instance-level functions.");
  }

  if (!load_platform_functions(instance)) {
    destroy_partial_instance(instance);
    throw LztGtestSkipExecutionException(
        "Vulkan external-semaphore export entry point is unavailable - "
        "skipping.");
  }

  if (debug_utils_available) {
    auto pfn_create_debug_messenger =
        reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
    if (pfn_create_debug_messenger != nullptr) {
      pfn_create_debug_messenger(instance, &dbg_msg_create_info, nullptr,
                                 &vk_debug_messenger);
    }
  }

  return instance;
}

void destroy_instance(VkInstance instance) {
  if (instance == VK_NULL_HANDLE) {
    return;
  }

  if (vk_debug_messenger != VK_NULL_HANDLE) {
    auto pfn_destroy_debug_messenger =
        reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
    if (pfn_destroy_debug_messenger != nullptr) {
      pfn_destroy_debug_messenger(instance, vk_debug_messenger, nullptr);
    }
    vk_debug_messenger = VK_NULL_HANDLE;
  }

  vkDestroyInstance(instance, nullptr);
}

std::vector<VkPhysicalDevice> get_physical_devices(VkInstance instance) {
  uint32_t physical_device_count = 0;
  VK_CHECK(
      vkEnumeratePhysicalDevices(instance, &physical_device_count, nullptr));

  std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
  VK_CHECK(vkEnumeratePhysicalDevices(instance, &physical_device_count,
                                      physical_devices.data()));

  return physical_devices;
}

std::vector<VkExtensionProperties>
get_device_extension_properties(VkPhysicalDevice physical_device) {
  uint32_t extensions_count = 0;
  VK_CHECK(vkEnumerateDeviceExtensionProperties(physical_device, nullptr,
                                                &extensions_count, nullptr));

  std::vector<VkExtensionProperties> device_extension_props(extensions_count);
  VK_CHECK(vkEnumerateDeviceExtensionProperties(physical_device, nullptr,
                                                &extensions_count,
                                                device_extension_props.data()));

  return device_extension_props;
}

bool has_timeline_semaphore_support(VkPhysicalDevice physical_device) {
  VkPhysicalDeviceTimelineSemaphoreFeatures timeline_features = {};
  timeline_features.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES;

  VkPhysicalDeviceFeatures2 device_features = {};
  device_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
  device_features.pNext = &timeline_features;

  vkGetPhysicalDeviceFeatures2(physical_device, &device_features);

  return timeline_features.timelineSemaphore == VK_TRUE;
}

uint32_t get_queue_family_index(VkPhysicalDevice physical_device) {
  uint32_t queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count,
                                           nullptr);
  std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count,
                                           queue_families.data());

  for (uint32_t i = 0; i < queue_family_count; ++i) {
    if (queue_families[i].queueFlags &
        (VK_QUEUE_TRANSFER_BIT | VK_QUEUE_GRAPHICS_BIT |
         VK_QUEUE_COMPUTE_BIT)) {
      return i;
    }
  }

  throw LztGtestSkipExecutionException(
      "No Vulkan queue family with transfer support - skipping.");
}

VkDevice create_device(VkPhysicalDevice physical_device,
                       uint32_t queue_family_index) {
  const float queue_priority = 1.0f;
  VkDeviceQueueCreateInfo queue_create_info = {};
  queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queue_create_info.queueFamilyIndex = queue_family_index;
  queue_create_info.queueCount = 1;
  queue_create_info.pQueuePriorities = &queue_priority;

  const std::vector<const char *> extensions = {
      get_platform_external_memory_extension_name(),
      get_platform_external_semaphore_extension_name()};

  const VkPhysicalDeviceTimelineSemaphoreFeatures timeline_features = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES,
      .timelineSemaphore = VK_TRUE};

  VkDeviceCreateInfo device_create_info = {};
  device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  device_create_info.pNext = &timeline_features;
  device_create_info.queueCreateInfoCount = 1;
  device_create_info.pQueueCreateInfos = &queue_create_info;
  device_create_info.enabledExtensionCount = to_u32(extensions.size());
  device_create_info.ppEnabledExtensionNames = extensions.data();

  VkDevice device = VK_NULL_HANDLE;
  VK_CHECK(
      vkCreateDevice(physical_device, &device_create_info, nullptr, &device));

  return device;
}

void destroy_device(VkDevice device) {
  if (device == VK_NULL_HANDLE) {
    return;
  }
  vkDestroyDevice(device, nullptr);
}

VkSemaphore create_export_semaphore(VkDevice device, VkSemaphoreType type) {
  VkSemaphoreCreateInfo semaphore_create_info = {};
  semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkExportSemaphoreCreateInfo export_semaphore_create_info = {};
  export_semaphore_create_info.sType =
      VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO;
  export_semaphore_create_info.handleTypes =
      PLATFORM_EXTERNAL_SEMAPHORE_HANDLE_TYPE;
  semaphore_create_info.pNext = &export_semaphore_create_info;

  VkSemaphoreTypeCreateInfo timeline_semaphore_create_info = {};
  if (type == VK_SEMAPHORE_TYPE_TIMELINE) {
    timeline_semaphore_create_info.sType =
        VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
    timeline_semaphore_create_info.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
    timeline_semaphore_create_info.initialValue = 0;

    // CreateInfo -> ExportInfo -> TimelineInfo
    export_semaphore_create_info.pNext = &timeline_semaphore_create_info;
  }

  VkSemaphore semaphore = VK_NULL_HANDLE;
  VK_CHECK(
      vkCreateSemaphore(device, &semaphore_create_info, nullptr, &semaphore));

  return semaphore;
}

void destroy_semaphore(VkDevice device, VkSemaphore semaphore) {
  if (semaphore == VK_NULL_HANDLE) {
    return;
  }
  vkDestroySemaphore(device, semaphore, nullptr);
}

} // namespace vlk
