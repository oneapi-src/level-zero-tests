/*
 *
 * Copyright (C) 2020-2023 Intel Corporation
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

#ifdef USE_ZESINIT
class EventsZesTest : public lzt::ZesSysmanCtsClass {
public:
  ze_driver_handle_t hDriver;
  uint32_t timeout;
  uint64_t timeoutEx;
  EventsZesTest() {
    hDriver = lzt::get_default_zes_driver();
    timeout = 10000;
    timeoutEx = 10000;
  }
  ~EventsZesTest() {}
};
#define EVENTS_TEST EventsZesTest
#else // USE_ZESINIT
class EventsTest : public lzt::SysmanCtsClass {
public:
  ze_driver_handle_t hDriver;
  uint32_t timeout;
  uint64_t timeoutEx;
  EventsTest() {
    hDriver = lzt::get_default_driver();
    timeout = 10000;
    timeoutEx = 10000;
  }
  ~EventsTest() {}
};
#define EVENTS_TEST EventsTest
#endif // USE_ZESINIT

void register_unknown_event(zes_device_handle_t device,
                            zes_event_type_flags_t events) {
  EXPECT_EQ(ZE_RESULT_ERROR_INVALID_ENUMERATION,
            zesDeviceEventRegister(device, events));
}

TEST_F(
    EVENTS_TEST,
    GivenValidEventHandleWhenListeningTemperatureEventsForCriticalOrThresholdTempThenEventsAreTriggered) {
  for (auto device : devices) {
    uint32_t num_temp_sensors = 0;
    auto temp_handles = lzt::get_temp_handles(device, num_temp_sensors);
    if (num_temp_sensors == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }
    for (auto temp_handle : temp_handles) {
      auto temp_properties = lzt::get_temp_properties(temp_handle);
      auto temp_config = lzt::get_temp_config(temp_handle);
      auto tempLimit = lzt::get_temp_state(temp_handle);
      if (temp_properties.isCriticalTempSupported) {
        temp_config.enableCritical = true;
      }
      if (temp_properties.isThreshold1Supported) {
        temp_config.threshold1.enableHighToLow = false;
        temp_config.threshold1.enableLowToHigh = true;
        // Setting threshold couple of degree above the current temperature
        temp_config.threshold1.threshold = tempLimit + 2;
        temp_config.threshold2.enableHighToLow = false;
        temp_config.threshold2.enableLowToHigh = false;
      }
      if ((temp_properties.isCriticalTempSupported == false) ||
          (temp_properties.isThreshold1Supported == false)) {
        // continue, as HW is not supporting the events
        continue;
      }
      ASSERT_EQ(ZE_RESULT_SUCCESS,
                lzt::set_temp_config(temp_handle, temp_config));
      zes_event_type_flags_t setEvents = ZES_EVENT_TYPE_FLAG_TEMP_CRITICAL |
                                         ZES_EVENT_TYPE_FLAG_TEMP_THRESHOLD1 |
                                         ZES_EVENT_TYPE_FLAG_TEMP_THRESHOLD2;
    }
  }
  // If we registered to receive events on any devices, start listening now
  uint32_t num_device_events = 0;
  std::vector<zes_event_type_flags_t> events(devices.size(), 0);
  LOG_INFO
      << "Listening for Critical or threshold cross temperature events ...";
  ASSERT_EQ(ZE_RESULT_SUCCESS,
            lzt::listen_event(hDriver, timeout, devices.size(), devices.data(),
                              &num_device_events, events.data()));
  for (uint32_t i = 0; i < devices.size(); i++) {
    if (events[i] & ZES_EVENT_TYPE_FLAG_TEMP_CRITICAL) {
      LOG_INFO << "Event received as device crossed critical temperature";
    } else if (events[i] & ZES_EVENT_TYPE_FLAG_TEMP_THRESHOLD1) {
      LOG_INFO << "Event received as device crossed threshold1 temperature";
    } else if (events[i] & ZES_EVENT_TYPE_FLAG_TEMP_THRESHOLD2) {
      LOG_INFO << "Event received as device crossed threshold2 temperature";
    } else if (events[i] != 0) {
      LOG_INFO << "Spurious event for temperature received";
      FAIL();
    }
  }
}
TEST_F(
    EVENTS_TEST,
    GivenValidEventHandleWhenListeningEventForDeviceResetRequiredByDriverThenEventsAreTriggeredForDeviceResetRequired) {
  for (auto device : devices) {
    lzt::register_event(device, ZES_EVENT_TYPE_FLAG_DEVICE_RESET_REQUIRED);
  }
  // If we registered to receive events on any devices, start listening now
  uint32_t num_device_events = 0;
  std::vector<zes_event_type_flags_t> events(devices.size(), 0);
  LOG_INFO << "Listening for device reset required event ...";
  ASSERT_EQ(ZE_RESULT_SUCCESS,
            lzt::listen_event(hDriver, timeout, devices.size(), devices.data(),
                              &num_device_events, events.data()));
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
    EVENTS_TEST,
    GivenValidEventHandleWhenListeningEventExForDeviceResetRequiredByDriverThenEventsAreTriggeredForDeviceResetRequired) {
  for (auto device : devices) {
    lzt::register_event(device, ZES_EVENT_TYPE_FLAG_DEVICE_RESET_REQUIRED);
  }
  // If we registered to receive events on any devices, start listening now
  uint32_t num_device_events = 0;
  std::vector<zes_event_type_flags_t> events(devices.size(), 0);
  LOG_INFO << "Listening for device reset required event ...";
  ASSERT_EQ(ZE_RESULT_SUCCESS,
            lzt::listen_eventEx(hDriver, timeoutEx, devices.size(),
                                devices.data(), &num_device_events,
                                events.data()));
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
    EVENTS_TEST,
    GivenValidEventHandleWhenListeningEventForDeviceDetachThenEventsAreTriggeredForDeviceDetach) {
  for (auto device : devices) {
    lzt::register_event(device, ZES_EVENT_TYPE_FLAG_DEVICE_DETACH);
  }
  // If we registered to receive events on any devices, start listening now
  uint32_t num_device_events = 0;
  std::vector<zes_event_type_flags_t> events(devices.size(), 0);
  LOG_INFO << "Listening for device being detached ...";
  ASSERT_EQ(ZE_RESULT_SUCCESS,
            lzt::listen_event(hDriver, timeout, devices.size(), devices.data(),
                              &num_device_events, events.data()));
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
    EVENTS_TEST,
    GivenValidEventHandleWhenListeningEventExForDeviceDetachThenEventsAreTriggeredForDeviceDetach) {
  for (auto device : devices) {
    lzt::register_event(device, ZES_EVENT_TYPE_FLAG_DEVICE_DETACH);
  }
  // If we registered to receive events on any devices, start listening now
  uint32_t num_device_events = 0;
  std::vector<zes_event_type_flags_t> events(devices.size(), 0);
  LOG_INFO << "Listening for device being detached ...";
  ASSERT_EQ(ZE_RESULT_SUCCESS,
            lzt::listen_eventEx(hDriver, timeoutEx, devices.size(),
                                devices.data(), &num_device_events,
                                events.data()));
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
    EVENTS_TEST,
    GivenValidEventHandleWhenListeningEventForDeviceAttachThenEventsAreTriggeredForDeviceAttach) {
  for (auto device : devices) {
    lzt::register_event(device, ZES_EVENT_TYPE_FLAG_DEVICE_ATTACH);
  }
  // If we registered to receive events on any devices, start listening now
  uint32_t num_device_events = 0;
  std::vector<zes_event_type_flags_t> events(devices.size(), 0);
  LOG_INFO << "Listening for device being attached ...";
  ASSERT_EQ(ZE_RESULT_SUCCESS,
            lzt::listen_event(hDriver, timeout, devices.size(), devices.data(),
                              &num_device_events, events.data()));
  for (uint32_t i = 0; i < devices.size(); i++) {
    if (events[i] & ZES_EVENT_TYPE_FLAG_DEVICE_ATTACH) {
      LOG_INFO << "Event received as device is detached";
    } else if (events[i] != 0) {
      LOG_INFO << "Spurious event for device being attached is received";
      FAIL();
    }
  }
}

static void eventListenThread(ze_driver_handle_t hDriver,
                              std::vector<ze_device_handle_t> devices,
                              uint32_t *numEventsGenerated) {
  auto timeout = 5000;
  // If we registered to receive events on any devices, start listening now
  uint32_t numDeviceEvents = std::numeric_limits<int32_t>::max();
  std::vector<zes_event_type_flags_t> events(devices.size(), 0);
  LOG_INFO << "Listening for device being detached ...";
  lzt::listen_eventEx(hDriver, timeout, devices.size(), devices.data(),
                      &numDeviceEvents, events.data());
  *numEventsGenerated = numDeviceEvents;
}

TEST_F(
    EVENTS_TEST,
    GivenValidEventHandleAndBlockedForDeviceDetachEventWhenEventRegisterIsCalledWithNoEventsThenEventListenIsUnBlocked) {
  uint32_t numEventsGenerated = std::numeric_limits<uint32_t>::max();
  for (auto device : devices) {
    lzt::register_event(device, ZES_EVENT_TYPE_FLAG_DEVICE_DETACH);
  }
  // Launch thread to listen to registered events
  std::thread listenThread(eventListenThread, hDriver, devices,
                           &numEventsGenerated);
  // Wait till zesDriverEventListen API is called.
  std::this_thread::sleep_for(std::chrono::seconds(2));
  // Now clear events to unblock listener
  for (auto device : devices) {
    lzt::register_event(device, 0);
  }
  // measure time bewteen clearing events to listening thread completion to
  // detect timeout and unblocking of listen due to clearing of events
  bool listen_timedout = false;
  auto start_time = std::chrono::high_resolution_clock::now();
  listenThread.join();
  auto end_time = std::chrono::high_resolution_clock::now();
  auto elapsed_time_microsec =
      std::chrono::duration<long double, std::chrono::microseconds::period>(
          end_time - start_time)
          .count();
  if ((elapsed_time_microsec / 1000) >= 2000) {
    listen_timedout = true;
  }
  EXPECT_EQ(listen_timedout, false);
  EXPECT_EQ(numEventsGenerated, 0);
}

TEST_F(
    EVENTS_TEST,
    GivenValidEventHandleWhenListeningEventExForDeviceAttachThenEventsAreTriggeredForDeviceAttach) {
  for (auto device : devices) {
    lzt::register_event(device, ZES_EVENT_TYPE_FLAG_DEVICE_ATTACH);
  }
  // If we registered to receive events on any devices, start listening now
  uint32_t num_device_events = 0;
  std::vector<zes_event_type_flags_t> events(devices.size(), 0);
  LOG_INFO << "Listening for device being attached ...";
  ASSERT_EQ(ZE_RESULT_SUCCESS,
            lzt::listen_eventEx(hDriver, timeoutEx, devices.size(),
                                devices.data(), &num_device_events,
                                events.data()));
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
    EVENTS_TEST,
    GivenValidEventHandleWhenListeningEventForDeviceEnteringDeepSleepStateThenEventsAreTriggeredForDeviceSleep) {
  for (auto device : devices) {
    lzt::register_event(device, ZES_EVENT_TYPE_FLAG_DEVICE_SLEEP_STATE_ENTER);
  }
  // If we registered to receive events on any devices, start listening now
  uint32_t num_device_events = 0;
  std::vector<zes_event_type_flags_t> events(devices.size(), 0);
  LOG_INFO << "Listening for device entering into deep sleep state event  ...";
  ASSERT_EQ(ZE_RESULT_SUCCESS,
            lzt::listen_event(hDriver, timeout, devices.size(), devices.data(),
                              &num_device_events, events.data()));
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
    EVENTS_TEST,
    GivenValidEventHandleWhenListeningEventExForDeviceEnteringDeepSleepStateThenEventsAreTriggeredForDeviceSleep) {
  for (auto device : devices) {
    lzt::register_event(device, ZES_EVENT_TYPE_FLAG_DEVICE_SLEEP_STATE_ENTER);
  }
  // If we registered to receive events on any devices, start listening now
  uint32_t num_device_events = 0;
  std::vector<zes_event_type_flags_t> events(devices.size(), 0);
  LOG_INFO << "Listening for device entering into deep sleep state event  ...";
  ASSERT_EQ(ZE_RESULT_SUCCESS,
            lzt::listen_eventEx(hDriver, timeoutEx, devices.size(),
                                devices.data(), &num_device_events,
                                events.data()));
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
    EVENTS_TEST,
    GivenValidEventHandleWhenListeningEventForDeviceExitingDeepSleepStateThenEventsAreTriggeredForDeviceSleepExit) {
  for (auto device : devices) {
    lzt::register_event(device, ZES_EVENT_TYPE_FLAG_DEVICE_SLEEP_STATE_EXIT);
  }
  // If we registered to receive events on any devices, start listening now
  uint32_t num_device_events = 0;
  std::vector<zes_event_type_flags_t> events(devices.size(), 0);
  LOG_INFO << "Listening for device exitiing into deep sleep state event  ...";
  ASSERT_EQ(ZE_RESULT_SUCCESS,
            lzt::listen_event(hDriver, timeout, devices.size(), devices.data(),
                              &num_device_events, events.data()));
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
    EVENTS_TEST,
    GivenValidEventHandleWhenListeningEventExForDeviceExitingDeepSleepStateThenEventsAreTriggeredForDeviceSleepExit) {
  for (auto device : devices) {
    lzt::register_event(device, ZES_EVENT_TYPE_FLAG_DEVICE_SLEEP_STATE_EXIT);
  }
  // If we registered to receive events on any devices, start listening now
  uint32_t num_device_events = 0;
  std::vector<zes_event_type_flags_t> events(devices.size(), 0);
  LOG_INFO << "Listening for device exitiing into deep sleep state event  ...";
  ASSERT_EQ(ZE_RESULT_SUCCESS,
            lzt::listen_eventEx(hDriver, timeoutEx, devices.size(),
                                devices.data(), &num_device_events,
                                events.data()));
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
    EVENTS_TEST,
    GivenValidEventHandleWhenListeningEventForFrequencyThrottlingThenEventsAreTriggeredForFrequencyThrottling) {
  for (auto device : devices) {
    lzt::register_event(device, ZES_EVENT_TYPE_FLAG_FREQ_THROTTLED);
  }
  // If we registered to receive events on any devices, start listening now
  uint32_t num_device_events = 0;
  std::vector<zes_event_type_flags_t> events(devices.size(), 0);
  LOG_INFO << "Listening for event after frequency starts throttling  ...";
  ASSERT_EQ(ZE_RESULT_SUCCESS,
            lzt::listen_event(hDriver, timeout, devices.size(), devices.data(),
                              &num_device_events, events.data()));
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
    EVENTS_TEST,
    GivenValidEventHandleWhenListeningEventExForFrequencyThrottlingThenEventsAreTriggeredForFrequencyThrottling) {
  for (auto device : devices) {
    lzt::register_event(device, ZES_EVENT_TYPE_FLAG_FREQ_THROTTLED);
  }
  // If we registered to receive events on any devices, start listening now
  uint32_t num_device_events = 0;
  std::vector<zes_event_type_flags_t> events(devices.size(), 0);
  LOG_INFO << "Listening for event after frequency starts throttling  ...";
  ASSERT_EQ(ZE_RESULT_SUCCESS,
            lzt::listen_eventEx(hDriver, timeoutEx, devices.size(),
                                devices.data(), &num_device_events,
                                events.data()));
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
    EVENTS_TEST,
    GivenValidEventHandleWhenListeningEventForCrossingEnergyThresholdThenEventsAreTriggeredForAccordingly) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto power_handles = lzt::get_power_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }
    for (auto power_handle : power_handles) {
      zes_energy_threshold_t energy_threshold = {};
      auto status =
          lzt::get_power_energy_threshold(power_handle, &energy_threshold);
      if (status == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
        continue;
      }
      EXPECT_EQ(status, ZE_RESULT_SUCCESS);
      // Aim to receive event for energy threshold after setting energy
      // threshold 25% more than current threshold
      double threshold =
          energy_threshold.threshold + 0.25 * (energy_threshold.threshold);
      lzt::set_power_energy_threshold(power_handle, threshold);
      lzt::register_event(device, ZES_EVENT_TYPE_FLAG_ENERGY_THRESHOLD_CROSSED);
    }
  }
  // If we registered to receive events on any devices, start listening now
  uint32_t num_device_events = 0;
  std::vector<zes_event_type_flags_t> events(devices.size(), 0);
  LOG_INFO << "Listening for event after crossing energy threshold ...";
  ASSERT_EQ(ZE_RESULT_SUCCESS,
            lzt::listen_event(hDriver, timeout, devices.size(), devices.data(),
                              &num_device_events, events.data()));
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
    EVENTS_TEST,
    GivenValidEventHandleWhenListeningEventExForCrossingEnergyThresholdThenEventsAreTriggeredForAccordingly) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto power_handles = lzt::get_power_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }
    for (auto power_handle : power_handles) {
      zes_energy_threshold_t energy_threshold = {};
      auto status =
          lzt::get_power_energy_threshold(power_handle, &energy_threshold);
      if (status == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE) {
        continue;
      }
      EXPECT_EQ(status, ZE_RESULT_SUCCESS);
      // Aim to receive event for energy threshold after setting energy
      // threshold 25% more than current threshold
      double threshold =
          energy_threshold.threshold + 0.25 * (energy_threshold.threshold);
      lzt::set_power_energy_threshold(power_handle, threshold);
      lzt::register_event(device, ZES_EVENT_TYPE_FLAG_ENERGY_THRESHOLD_CROSSED);
    }
  }
  // If we registered to receive events on any devices, start listening now
  uint32_t num_device_events = 0;
  std::vector<zes_event_type_flags_t> events(devices.size(), 0);
  LOG_INFO << "Listening for event after crossing energy threshold ...";
  ASSERT_EQ(ZE_RESULT_SUCCESS,
            lzt::listen_eventEx(hDriver, timeoutEx, devices.size(),
                                devices.data(), &num_device_events,
                                events.data()));
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
    EVENTS_TEST,
    GivenValidEventHandleWhenListeningEventForHealthofDeviceMemoryChangeThenCorrespondingEventsReceived) {
  for (auto device : devices) {
    lzt::register_event(device, ZES_EVENT_TYPE_FLAG_MEM_HEALTH);
  }
  // If we registered to receive events on any devices, start listening now
  uint32_t num_device_events = 0;
  std::vector<zes_event_type_flags_t> events(devices.size(), 0);
  LOG_INFO << "Listening for event if device memory health changes  ...";
  ASSERT_EQ(ZE_RESULT_SUCCESS,
            lzt::listen_event(hDriver, timeout, devices.size(), devices.data(),
                              &num_device_events, events.data()));
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
    EVENTS_TEST,
    GivenValidEventHandleWhenListeningEventExForHealthofDeviceMemoryChangeThenCorrespondingEventsReceived) {
  for (auto device : devices) {
    lzt::register_event(device, ZES_EVENT_TYPE_FLAG_MEM_HEALTH);
  }
  // If we registered to receive events on any devices, start listening now
  uint32_t num_device_events = 0;
  std::vector<zes_event_type_flags_t> events(devices.size(), 0);
  LOG_INFO << "Listening for event if device memory health changes  ...";
  ASSERT_EQ(ZE_RESULT_SUCCESS,
            lzt::listen_eventEx(hDriver, timeoutEx, devices.size(),
                                devices.data(), &num_device_events,
                                events.data()));
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
    EVENTS_TEST,
    GivenValidEventHandleWhenListeningEventForHealthofFabricPortChangeThenCorrespondingEventsReceived) {
  for (auto device : devices) {
    lzt::register_event(device, ZES_EVENT_TYPE_FLAG_FABRIC_PORT_HEALTH);
  }
  // If we registered to receive events on any devices, start listening now
  uint32_t num_device_events = 0;
  std::vector<zes_event_type_flags_t> events(devices.size(), 0);
  LOG_INFO << "Listening for event if fabric port health changes  ...";
  ASSERT_EQ(ZE_RESULT_SUCCESS,
            lzt::listen_event(hDriver, timeout, devices.size(), devices.data(),
                              &num_device_events, events.data()));
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
    EVENTS_TEST,
    GivenValidEventHandleWhenListeningEventExForHealthofFabricPortChangeThenCorrespondingEventsReceived) {
  for (auto device : devices) {
    lzt::register_event(device, ZES_EVENT_TYPE_FLAG_FABRIC_PORT_HEALTH);
  }
  // If we registered to receive events on any devices, start listening now
  uint32_t num_device_events = 0;
  std::vector<zes_event_type_flags_t> events(devices.size(), 0);
  LOG_INFO << "Listening for event if fabric port health changes  ...";
  ASSERT_EQ(ZE_RESULT_SUCCESS,
            lzt::listen_eventEx(hDriver, timeoutEx, devices.size(),
                                devices.data(), &num_device_events,
                                events.data()));
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
    EVENTS_TEST,
    GivenValidEventHandleWhenListeningEventForPciLinkHealthChangesThenCorrespondingEventsReceived) {
  for (auto device : devices) {
    lzt::register_event(device, ZES_EVENT_TYPE_FLAG_PCI_LINK_HEALTH);
  }
  // If we registered to receive events on any devices, start listening now
  uint32_t num_device_events = 0;
  std::vector<zes_event_type_flags_t> events(devices.size(), 0);
  LOG_INFO << "Listening for event if pci link health changes  ...";
  ASSERT_EQ(ZE_RESULT_SUCCESS,
            lzt::listen_event(hDriver, timeout, devices.size(), devices.data(),
                              &num_device_events, events.data()));
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
    EVENTS_TEST,
    GivenValidEventHandleWhenListeningEventExForPciLinkHealthChangesThenCorrespondingEventsReceived) {
  for (auto device : devices) {
    lzt::register_event(device, ZES_EVENT_TYPE_FLAG_PCI_LINK_HEALTH);
  }
  // If we registered to receive events on any devices, start listening now
  uint32_t num_device_events = 0;
  std::vector<zes_event_type_flags_t> events(devices.size(), 0);
  LOG_INFO << "Listening for event if pci link health changes  ...";
  ASSERT_EQ(ZE_RESULT_SUCCESS,
            lzt::listen_eventEx(hDriver, timeoutEx, devices.size(),
                                devices.data(), &num_device_events,
                                events.data()));
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
    EVENTS_TEST,
    GivenValidEventHandleWhenListeningEventForCrossingTotalRASErrorsThresholdThenEventsAreTriggeredForAccordingly) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto ras_handles = lzt::get_ras_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }
    for (auto ras_handle : ras_handles) {
      auto props = lzt::get_ras_properties(ras_handle);
      auto config = lzt::get_ras_config(ras_handle);
      // Aim to receive event for RAS errors after setting RAS threshold 20%
      // more than current threshold
      if (1.20 * (config.totalThreshold) < UINT64_MAX) {
        config.totalThreshold = 1.20 * (config.totalThreshold);
      }

      lzt::set_ras_config(ras_handle, config);

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
  uint32_t num_device_events = 0;
  std::vector<zes_event_type_flags_t> events(devices.size(), 0);
  LOG_INFO << "Listening for event if pci link health changes  ...";
  ASSERT_EQ(ZE_RESULT_SUCCESS,
            lzt::listen_event(hDriver, timeout, devices.size(), devices.data(),
                              &num_device_events, events.data()));
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
    EVENTS_TEST,
    GivenValidEventHandleWhenListeningEventExForCrossingTotalRASErrorsThresholdThenEventsAreTriggeredForAccordingly) {
  for (auto device : devices) {
    uint32_t count = 0;
    auto ras_handles = lzt::get_ras_handles(device, count);
    if (count == 0) {
      FAIL() << "No handles found: "
             << _ze_result_t(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE);
    }
    for (auto ras_handle : ras_handles) {
      auto props = lzt::get_ras_properties(ras_handle);
      auto config = lzt::get_ras_config(ras_handle);
      // Aim to receive event for RAS errors after setting RAS threshold 20%
      // more than current threshold
      if (1.20 * (config.totalThreshold) < UINT64_MAX) {
        config.totalThreshold = 1.20 * (config.totalThreshold);
      }

      lzt::set_ras_config(ras_handle, config);

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
  uint32_t num_device_events = 0;
  std::vector<zes_event_type_flags_t> events(devices.size(), 0);
  LOG_INFO << "Listening for event if pci link health changes  ...";
  ASSERT_EQ(ZE_RESULT_SUCCESS,
            lzt::listen_eventEx(hDriver, timeoutEx, devices.size(),
                                devices.data(), &num_device_events,
                                events.data()));
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

} // namespace
