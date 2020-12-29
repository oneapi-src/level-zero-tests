/*
 *
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"

#include "logging/logging.hpp"
#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"

namespace lzt = level_zero_tests;

#include <thread>
#include <level_zero/ze_api.h>
#include <level_zero/zes_api.h>

namespace {

class EventsTest : public lzt::SysmanCtsClass {
public:
  ze_driver_handle_t hDriver;
  uint32_t timeout;
  EventsTest() {
    hDriver = lzt::get_default_driver();
    timeout = 10000;
  }
  ~EventsTest() {}
};

void register_unknown_event(zes_device_handle_t device,
                            zes_event_type_flags_t events) {
  EXPECT_EQ(ZE_RESULT_ERROR_INVALID_ENUMERATION,
            zesDeviceEventRegister(device, events));
}

TEST_F(
    EventsTest,
    GivenValidEventHandleWhenListeningTemperatureEventsForCriticalOrThresholdTempThenEventsAreTriggered) {
  for (auto device : devices) {
    uint32_t numTempSensors = 0;
    auto tempHandles = lzt::get_temp_handles(device, numTempSensors);
    if (numTempSensors == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }
    for (auto tempHandle : tempHandles) {
      auto tempProperties = lzt::get_temp_properties(tempHandle);
      auto tempConfig = lzt::get_temp_config(tempHandle);
      auto tempLimit = lzt::get_temp_state(tempHandle);
      if (tempProperties.isCriticalTempSupported) {
        tempConfig.enableCritical = true;
      }
      if (tempProperties.isThreshold1Supported) {
        tempConfig.threshold1.enableHighToLow = false;
        tempConfig.threshold1.enableLowToHigh = true;
        // Setting threshold couple of degree above the current temperature
        tempConfig.threshold1.threshold = tempLimit + 2;
        tempConfig.threshold2.enableHighToLow = false;
        tempConfig.threshold2.enableLowToHigh = false;
      }
      if ((tempProperties.isCriticalTempSupported == false) ||
          (tempProperties.isThreshold1Supported == false)) {
        // continue, as HW is not supporting the events
        continue;
      }
      ASSERT_EQ(ZE_RESULT_SUCCESS,
                lzt::set_temp_config(tempHandle, tempConfig));
      zes_event_type_flags_t setEvents = ZES_EVENT_TYPE_FLAG_TEMP_CRITICAL |
                                         ZES_EVENT_TYPE_FLAG_TEMP_THRESHOLD1;
    }
  }
  // If we registered to receive events on any devices, start listening now
  uint32_t numDeviceEvents = 0;
  std::vector<zes_event_type_flags_t> events(devices.size(), 0);
  LOG_INFO
      << "Listening for Critical or threshold cross temperature events ...";
  ASSERT_EQ(ZE_RESULT_SUCCESS,
            lzt::listen_event(hDriver, timeout, devices.size(), devices.data(),
                              &numDeviceEvents, events.data()));
  for (uint32_t i = 0; i < devices.size(); i++) {
    if (events[i] & ZES_EVENT_TYPE_FLAG_TEMP_CRITICAL) {
      LOG_INFO << "Event received as device crossed critical temperature";
    } else if (events[i] & ZES_EVENT_TYPE_FLAG_TEMP_THRESHOLD1) {
      LOG_INFO << "Event received as device crossed threshold1 temperature";
    } else if (events[i] != 0) {
      LOG_INFO << "Spurious event for temperature received";
      FAIL();
    }
  }
}
TEST_F(
    EventsTest,
    GivenValidEventHandleWhenListeningEventForDeviceResetRequiredByDriverThenEventsAreTriggeredForDeviceResetRequired) {
  for (auto device : devices) {
    lzt::register_event(device, ZES_EVENT_TYPE_FLAG_DEVICE_RESET_REQUIRED);
  }
  // If we registered to receive events on any devices, start listening now
  uint32_t numDeviceEvents = 0;
  std::vector<zes_event_type_flags_t> events(devices.size(), 0);
  LOG_INFO << "Listening for device reset required event ...";
  ASSERT_EQ(ZE_RESULT_SUCCESS,
            lzt::listen_event(hDriver, timeout, devices.size(), devices.data(),
                              &numDeviceEvents, events.data()));
  for (uint32_t i = 0; i < devices.size(); i++) {
    if (events[i] & ZES_EVENT_TYPE_FLAG_DEVICE_RESET_REQUIRED) {
      LOG_INFO << "Event received as device reset is required";
    } else if (events[i] != 0) {
      LOG_INFO << "Spurious event for device reset required received";
      FAIL();
    }
  }
}

TEST_F(
    EventsTest,
    GivenValidEventHandleWhenListeningEventForDeviceDetachThenEventsAreTriggeredForDeviceDetach) {
  for (auto device : devices) {
    lzt::register_event(device, ZES_EVENT_TYPE_FLAG_DEVICE_DETACH);
  }
  // If we registered to receive events on any devices, start listening now
  uint32_t numDeviceEvents = 0;
  std::vector<zes_event_type_flags_t> events(devices.size(), 0);
  LOG_INFO << "Listening for device being detached ...";
  ASSERT_EQ(ZE_RESULT_SUCCESS,
            lzt::listen_event(hDriver, timeout, devices.size(), devices.data(),
                              &numDeviceEvents, events.data()));
  for (uint32_t i = 0; i < devices.size(); i++) {
    if (events[i] & ZES_EVENT_TYPE_FLAG_DEVICE_DETACH) {
      LOG_INFO << "Event received as device is detached";
    } else if (events[i] != 0) {
      LOG_INFO << "Spurious event for device being detached is received";
      FAIL();
    }
  }
}

TEST_F(
    EventsTest,
    GivenValidEventHandleWhenListeningEventForDeviceAttachThenEventsAreTriggeredForDeviceAttach) {
  for (auto device : devices) {
    lzt::register_event(device, ZES_EVENT_TYPE_FLAG_DEVICE_ATTACH);
  }
  // If we registered to receive events on any devices, start listening now
  uint32_t numDeviceEvents = 0;
  std::vector<zes_event_type_flags_t> events(devices.size(), 0);
  LOG_INFO << "Listening for device being attached ...";
  ASSERT_EQ(ZE_RESULT_SUCCESS,
            lzt::listen_event(hDriver, timeout, devices.size(), devices.data(),
                              &numDeviceEvents, events.data()));
  for (uint32_t i = 0; i < devices.size(); i++) {
    if (events[i] & ZES_EVENT_TYPE_FLAG_DEVICE_ATTACH) {
      LOG_INFO << "Event received as device is detached";
    } else if (events[i] != 0) {
      LOG_INFO << "Spurious event for device being attached is received";
      FAIL();
    }
  }
}

TEST_F(
    EventsTest,
    GivenValidEventHandleWhenListeningEventForDeviceEnteringDeepSleepStateThenEventsAreTriggeredForDeviceSleep) {
  for (auto device : devices) {
    lzt::register_event(device, ZES_EVENT_TYPE_FLAG_DEVICE_SLEEP_STATE_ENTER);
  }
  // If we registered to receive events on any devices, start listening now
  uint32_t numDeviceEvents = 0;
  std::vector<zes_event_type_flags_t> events(devices.size(), 0);
  LOG_INFO << "Listening for device entering into deep sleep state event  ...";
  ASSERT_EQ(ZE_RESULT_SUCCESS,
            lzt::listen_event(hDriver, timeout, devices.size(), devices.data(),
                              &numDeviceEvents, events.data()));
  for (uint32_t i = 0; i < devices.size(); i++) {
    if (events[i] & ZES_EVENT_TYPE_FLAG_DEVICE_SLEEP_STATE_ENTER) {
      LOG_INFO << "Event received as device entered into deep sleep state";
    } else if (events[i] != 0) {
      LOG_INFO << "Spurious event for device deep sleep enter received";
      FAIL();
    }
  }
}

TEST_F(
    EventsTest,
    GivenValidEventHandleWhenListeningEventForDeviceExitingDeepSleepStateThenEventsAreTriggeredForDeviceSleepExit) {
  for (auto device : devices) {
    lzt::register_event(device, ZES_EVENT_TYPE_FLAG_DEVICE_SLEEP_STATE_EXIT);
  }
  // If we registered to receive events on any devices, start listening now
  uint32_t numDeviceEvents = 0;
  std::vector<zes_event_type_flags_t> events(devices.size(), 0);
  LOG_INFO << "Listening for device exitiing into deep sleep state event  ...";
  ASSERT_EQ(ZE_RESULT_SUCCESS,
            lzt::listen_event(hDriver, timeout, devices.size(), devices.data(),
                              &numDeviceEvents, events.data()));
  for (uint32_t i = 0; i < devices.size(); i++) {
    if (events[i] & ZES_EVENT_TYPE_FLAG_DEVICE_SLEEP_STATE_EXIT) {
      LOG_INFO << "Event received as device exited into deep sleep state";
    } else if (events[i] != 0) {
      LOG_INFO << "Spurious event for device deep sleep exit received";
      FAIL();
    }
  }
}

TEST_F(
    EventsTest,
    GivenValidEventHandleWhenListeningEventForFrequencyThrottlingThenEventsAreTriggeredForFrequencyThrottling) {
  for (auto device : devices) {
    lzt::register_event(device, ZES_EVENT_TYPE_FLAG_FREQ_THROTTLED);
  }
  // If we registered to receive events on any devices, start listening now
  uint32_t numDeviceEvents = 0;
  std::vector<zes_event_type_flags_t> events(devices.size(), 0);
  LOG_INFO << "Listening for event after frequency starts throttling  ...";
  ASSERT_EQ(ZE_RESULT_SUCCESS,
            lzt::listen_event(hDriver, timeout, devices.size(), devices.data(),
                              &numDeviceEvents, events.data()));
  for (uint32_t i = 0; i < devices.size(); i++) {
    if (events[i] & ZES_EVENT_TYPE_FLAG_FREQ_THROTTLED) {
      LOG_INFO << "Event received as frequency starts being throttled";
    } else if (events[i] != 0) {
      LOG_INFO << "Spurious event for frequency throttling received";
      FAIL();
    }
  }
}

TEST_F(
    EventsTest,
    GivenValidEventHandleWhenListeningEventForCrossingEnergyThresholdThenEventsAreTriggeredForAccordingly) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto powerHandles = lzt::get_power_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }
    for (auto powerHandle : powerHandles) {
      auto energyThreshold = lzt::get_power_energy_threshold(powerHandle);
      // Aim to receive event for energy threshold after setting energy
      // threshold 25% more than current threshold
      double threshold =
          energyThreshold.threshold + 0.25 * (energyThreshold.threshold);
      lzt::set_power_energy_threshold(powerHandle, threshold);
      lzt::register_event(device, ZES_EVENT_TYPE_FLAG_ENERGY_THRESHOLD_CROSSED);
    }
  }
  // If we registered to receive events on any devices, start listening now
  uint32_t numDeviceEvents = 0;
  std::vector<zes_event_type_flags_t> events(devices.size(), 0);
  LOG_INFO << "Listening for event after crossing energy threshold ...";
  ASSERT_EQ(ZE_RESULT_SUCCESS,
            lzt::listen_event(hDriver, timeout, devices.size(), devices.data(),
                              &numDeviceEvents, events.data()));
  for (uint32_t i = 0; i < devices.size(); i++) {
    if (events[i] & ZES_EVENT_TYPE_FLAG_ENERGY_THRESHOLD_CROSSED) {
      LOG_INFO << "Event received as Energy consumption threshold crossed";
    } else if (events[i] != 0) {
      LOG_INFO << "Spurious event for energy threshold received";
      FAIL();
    }
  }
}

TEST_F(
    EventsTest,
    GivenValidEventHandleWhenListeningEventForHealthofDeviceMemoryChangeThenCorrespondingEventsReceived) {
  for (auto device : devices) {
    lzt::register_event(device, ZES_EVENT_TYPE_FLAG_MEM_HEALTH);
  }
  // If we registered to receive events on any devices, start listening now
  uint32_t numDeviceEvents = 0;
  std::vector<zes_event_type_flags_t> events(devices.size(), 0);
  LOG_INFO << "Listening for event if device memory health changes  ...";
  ASSERT_EQ(ZE_RESULT_SUCCESS,
            lzt::listen_event(hDriver, timeout, devices.size(), devices.data(),
                              &numDeviceEvents, events.data()));
  for (uint32_t i = 0; i < devices.size(); i++) {
    if (events[i] & ZES_EVENT_TYPE_FLAG_MEM_HEALTH) {
      LOG_INFO << "Event received as change in device memory health occurred";
    } else if (events[i] != 0) {
      LOG_INFO << "Spurious event for change in device memory health received";
      FAIL();
    }
  }
}

TEST_F(
    EventsTest,
    GivenValidEventHandleWhenListeningEventForHealthofFabricPortChangeThenCorrespondingEventsReceived) {
  for (auto device : devices) {
    lzt::register_event(device, ZES_EVENT_TYPE_FLAG_FABRIC_PORT_HEALTH);
  }
  // If we registered to receive events on any devices, start listening now
  uint32_t numDeviceEvents = 0;
  std::vector<zes_event_type_flags_t> events(devices.size(), 0);
  LOG_INFO << "Listening for event if fabric port health changes  ...";
  ASSERT_EQ(ZE_RESULT_SUCCESS,
            lzt::listen_event(hDriver, timeout, devices.size(), devices.data(),
                              &numDeviceEvents, events.data()));
  for (uint32_t i = 0; i < devices.size(); i++) {
    if (events[i] & ZES_EVENT_TYPE_FLAG_FABRIC_PORT_HEALTH) {
      LOG_INFO << "Event received as change in fabric port health occurred";
    } else if (events[i] != 0) {
      LOG_INFO << "Spurious event for change in fabric port health received";
      FAIL();
    }
  }
}

TEST_F(
    EventsTest,
    GivenValidEventHandleWhenListeningEventForPciLinkHealthChangesThenCorrespondingEventsReceived) {
  for (auto device : devices) {
    lzt::register_event(device, ZES_EVENT_TYPE_FLAG_PCI_LINK_HEALTH);
  }
  // If we registered to receive events on any devices, start listening now
  uint32_t numDeviceEvents = 0;
  std::vector<zes_event_type_flags_t> events(devices.size(), 0);
  LOG_INFO << "Listening for event if pci link health changes  ...";
  ASSERT_EQ(ZE_RESULT_SUCCESS,
            lzt::listen_event(hDriver, timeout, devices.size(), devices.data(),
                              &numDeviceEvents, events.data()));
  for (uint32_t i = 0; i < devices.size(); i++) {
    if (events[i] & ZES_EVENT_TYPE_FLAG_PCI_LINK_HEALTH) {
      LOG_INFO << "Event received as change in pci link health occurred";
    } else if (events[i] != 0) {
      LOG_INFO << "Spurious event for change in pci link health received";
      FAIL();
    }
  }
}

TEST_F(
    EventsTest,
    GivenValidEventHandleWhenListeningEventForCrossingTotalRASErrorsThresholdThenEventsAreTriggeredForAccordingly) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto rasHandles = lzt::get_ras_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }
    for (auto rasHandle : rasHandles) {
      auto props = lzt::get_ras_properties(rasHandle);
      auto config = lzt::get_ras_config(rasHandle);
      // Aim to receive event for RAS errors after setting RAS threshold 20%
      // more than current threshold
      if (1.20 * (config.totalThreshold) < UINT64_MAX) {
        config.totalThreshold = 1.20 * (config.totalThreshold);
      }

      lzt::set_ras_config(rasHandle, config);

      zes_event_type_flags_t setEvent = 0;
      switch (props.type) {
      case ZES_RAS_ERROR_TYPE_CORRECTABLE:
        setEvent = ZES_EVENT_TYPE_FLAG_RAS_CORRECTABLE_ERRORS;
        break;
      case ZES_RAS_ERROR_TYPE_UNCORRECTABLE:
        setEvent = ZES_EVENT_TYPE_FLAG_RAS_UNCORRECTABLE_ERRORS;
        break;
      default:
        break;
      }
      lzt::register_event(device, setEvent);
    }
  }

  // If we registered to receive events on any devices, start listening now
  uint32_t numDeviceEvents = 0;
  std::vector<zes_event_type_flags_t> events(devices.size(), 0);
  LOG_INFO << "Listening for event if pci link health changes  ...";
  ASSERT_EQ(ZE_RESULT_SUCCESS,
            lzt::listen_event(hDriver, timeout, devices.size(), devices.data(),
                              &numDeviceEvents, events.data()));
  for (uint32_t i = 0; i < devices.size(); i++) {
    if (events[i] & ZES_EVENT_TYPE_FLAG_RAS_CORRECTABLE_ERRORS) {
      LOG_INFO << "Event received RAS correctable errors threshold crossed";
    } else if (events[i] & ZES_EVENT_TYPE_FLAG_RAS_UNCORRECTABLE_ERRORS) {
      LOG_INFO
          << "Event received as RAS uncorrectable errors threshold crossed";
    } else if (events[i] != 0) {
      LOG_INFO << "Spurious event for RAS errors received";
      FAIL();
    }
  }
}

TEST_F(
    EventsTest,
    GivenValidDeviceHandleWhenListeningForAListOfEventsThenEventRegisterAPIReturnsProperErrorCodeInCaseEventsAreInvalid) {
  for (auto device : devices) {
    zes_event_type_flags_t events = ZES_EVENT_TYPE_FLAG_FORCE_UINT32;
    register_unknown_event(device, events);
  }
}

} // namespace
