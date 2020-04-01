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
#include <level_zero/zet_api.h>

namespace {

void child_thread_function(ze_device_handle_t device) {
  // Ensure that before actually resetting the device, we have started to listen
  // to events. so sleep for some time and then do reset
  std::this_thread::sleep_for(std::chrono::milliseconds(30));
  lzt::sysman_device_reset(device);
}

class EventsTest : public lzt::SysmanCtsClass {
public:
  ze_driver_handle_t hDriver;
  EventsTest() { hDriver = lzt::get_default_driver(); }
  ~EventsTest() {}
};

TEST_F(
    EventsTest,
    GivenValidSysmanHandleWhenRetrievingEventsHandleThenValidEventHandleIsReturned) {
  for (auto device : devices) {
    auto hEvent = lzt::get_event_handle(device);
  }
}

TEST_F(
    EventsTest,
    GivenValidSysmanHandleWhenGettingEventHandleTwiceThenSimilarEventHandlesReturned) {
  for (auto device : devices) {
    auto eventHandleInitial = lzt::get_event_handle(device);
    ASSERT_FALSE(eventHandleInitial == NULL);

    auto eventHandleLater = lzt::get_event_handle(device);
    ASSERT_FALSE(eventHandleLater == NULL);
    EXPECT_EQ(eventHandleInitial, eventHandleLater);
  }
}

TEST_F(
    EventsTest,
    GivenValidEventHandleWhenListeningTemperatureEventsForCriticalOrThresholdTempThenEventsAreTriggered) {
  for (auto device : devices) {
    auto hEvent = lzt::get_event_handle(device);
    ASSERT_FALSE(hEvent == NULL);
    uint32_t numTempSensors = 0;
    auto tempHandles = lzt::get_temp_handles(device, numTempSensors);
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
        // return from the test, as HW is not supporting the events
        return;
      }
      ASSERT_EQ(ZE_RESULT_SUCCESS,
                lzt::set_temp_config(tempHandle, tempConfig));
      zet_event_config_t eventConfig;
      eventConfig.registered = ZET_SYSMAN_EVENT_TYPE_TEMP_CRITICAL |
                               ZET_SYSMAN_EVENT_TYPE_TEMP_THRESHOLD1;
      lzt::set_event_config(hEvent, eventConfig);

      // If we registered to receive events on any devices, start listening now
      uint32_t events;
      LOG_INFO
          << "Listening for Critical or threshold cross temperature events ...";
      ASSERT_EQ(ZE_RESULT_SUCCESS,
                lzt::listen_event(hDriver, ZET_EVENT_WAIT_INFINITE, 1, &hEvent,
                                  &events));
      ASSERT_EQ(ZE_RESULT_SUCCESS, lzt::get_event_state(hEvent, true, events));
      if (events & ZET_SYSMAN_EVENT_TYPE_TEMP_CRITICAL) {
        LOG_INFO << "Event received as device crossed critical temperature";
      } else if (events & ZET_SYSMAN_EVENT_TYPE_TEMP_THRESHOLD1) {
        LOG_INFO << "Event received as device crossed threshold1 temperature";
      } else {
        LOG_INFO << "Spurious event for temperature received";
        FAIL();
      }
    }
  }
}

TEST_F(
    EventsTest,
    GivenValidEventHandleWhenListeningEventForDeviceResetByDriverThenEventsAreTriggeredForDeviceReset) {
  for (auto device : devices) {
    auto hEvent = lzt::get_event_handle(device);
    ASSERT_FALSE(hEvent == NULL);
    zet_event_config_t eventConfig;
    eventConfig.registered = ZET_SYSMAN_EVENT_TYPE_DEVICE_RESET;
    lzt::set_event_config(hEvent, eventConfig);

    std::thread child_thread(child_thread_function, device);
    // If we registered to receive events on any devices, start listening now
    uint32_t events;
    LOG_INFO << "Listening for Device Reset events ...";
    ASSERT_EQ(ZE_RESULT_SUCCESS,
              lzt::listen_event(hDriver, ZET_EVENT_WAIT_INFINITE, 1, &hEvent,
                                &events));

    ASSERT_EQ(ZE_RESULT_SUCCESS, lzt::get_event_state(hEvent, true, events));
    if (events & ZET_SYSMAN_EVENT_TYPE_DEVICE_RESET) {
      LOG_INFO << "Event received as device got reset";
    } else {
      LOG_INFO << "Spurious event for device reset received";
      FAIL();
    }
  }
}

TEST_F(
    EventsTest,
    GivenValidEventHandleWhenListeningEventForDeviceEnteringDeepSleepStateThenEventsAreTriggeredForDeviceSleep) {
  for (auto device : devices) {
    auto hEvent = lzt::get_event_handle(device);
    ASSERT_FALSE(hEvent == NULL);
    zet_event_config_t eventConfig;
    eventConfig.registered = ZET_SYSMAN_EVENT_TYPE_DEVICE_SLEEP_STATE_ENTER;
    lzt::set_event_config(hEvent, eventConfig);

    // If we registered to receive events on any devices, start listening now
    uint32_t events;
    LOG_INFO
        << "Listening for device entering into deep sleep state event  ...";
    ASSERT_EQ(ZE_RESULT_SUCCESS,
              lzt::listen_event(hDriver, ZET_EVENT_WAIT_INFINITE, 1, &hEvent,
                                &events));
    ASSERT_EQ(ZE_RESULT_SUCCESS, lzt::get_event_state(hEvent, true, events));
    if (events & ZET_SYSMAN_EVENT_TYPE_DEVICE_SLEEP_STATE_ENTER) {
      LOG_INFO << "Event received as device entered into deep sleep state";
    } else {
      LOG_INFO << "Spurious event for device deep sleep enter received";
      FAIL();
    }
  }
}

TEST_F(
    EventsTest,
    GivenValidEventHandleWhenListeningEventForDeviceExitingDeepSleepStateThenEventsAreTriggeredForDeviceSleepExit) {
  for (auto device : devices) {
    auto hEvent = lzt::get_event_handle(device);
    ASSERT_FALSE(hEvent == NULL);
    zet_event_config_t eventConfig;
    eventConfig.registered = ZET_SYSMAN_EVENT_TYPE_DEVICE_SLEEP_STATE_EXIT;
    lzt::set_event_config(hEvent, eventConfig);

    // If we registered to receive events on any devices, start listening now
    uint32_t events;
    LOG_INFO << "Listening for device exiting from deep sleep state event  ...";
    ASSERT_EQ(ZE_RESULT_SUCCESS,
              lzt::listen_event(hDriver, ZET_EVENT_WAIT_INFINITE, 1, &hEvent,
                                &events));
    ASSERT_EQ(ZE_RESULT_SUCCESS, lzt::get_event_state(hEvent, true, events));
    if (events & ZET_SYSMAN_EVENT_TYPE_DEVICE_SLEEP_STATE_EXIT) {
      LOG_INFO << "Event received as device exited from deep sleep state";
    } else {
      LOG_INFO << "Spurious event for device deep sleep exit received";
      FAIL();
    }
  }
}

TEST_F(
    EventsTest,
    GivenValidEventHandleWhenListeningEventForFrequencyThrottlingThenEventsAreTriggeredForFrequencyThrottling) {
  for (auto device : devices) {
    auto hEvent = lzt::get_event_handle(device);
    ASSERT_FALSE(hEvent == NULL);
    zet_event_config_t eventConfig;
    eventConfig.registered = ZET_SYSMAN_EVENT_TYPE_FREQ_THROTTLED;
    lzt::set_event_config(hEvent, eventConfig);

    // If we registered to receive events on any devices, start listening now
    uint32_t events;
    LOG_INFO << "Listening for event after frequency starts throttling  ...";
    ASSERT_EQ(ZE_RESULT_SUCCESS,
              lzt::listen_event(hDriver, ZET_EVENT_WAIT_INFINITE, 1, &hEvent,
                                &events));
    ASSERT_EQ(ZE_RESULT_SUCCESS, lzt::get_event_state(hEvent, true, events));
    if (events & ZET_SYSMAN_EVENT_TYPE_FREQ_THROTTLED) {
      LOG_INFO << "Event received as frequency starts being throttled";
    } else {
      LOG_INFO << "Spurious event for frequency throttling received";
      FAIL();
    }
  }
}

TEST_F(
    EventsTest,
    GivenValidEventHandleWhenListeningEventForCrossingEnergyThresholdThenEventsAreTriggeredForAccordingly) {
  for (auto device : devices) {
    auto hEvent = lzt::get_event_handle(device);
    ASSERT_FALSE(hEvent == NULL);
    uint32_t count = 0;
    auto powerHandles = lzt::get_power_handles(device, count);
    for (auto powerHandle : powerHandles) {
      auto energyThreshold = lzt::get_power_energy_threshold(powerHandle);
      // Aim to receive event for energy threshold after setting energy
      // threshold 25% more than current threshold
      double threshold =
          energyThreshold.threshold + 0.25 * (energyThreshold.threshold);
      lzt::set_power_energy_threshold(powerHandle, threshold);
      zet_event_config_t eventConfig;
      eventConfig.registered = ZET_SYSMAN_EVENT_TYPE_ENERGY_THRESHOLD_CROSSED;
      lzt::set_event_config(hEvent, eventConfig);

      // If we registered to receive events on any devices, start listening now
      uint32_t events;
      LOG_INFO << "Listening for event after crossing energy threshold ...";
      ASSERT_EQ(ZE_RESULT_SUCCESS,
                lzt::listen_event(hDriver, ZET_EVENT_WAIT_INFINITE, 1, &hEvent,
                                  &events));
      ASSERT_EQ(ZE_RESULT_SUCCESS, lzt::get_event_state(hEvent, true, events));
      if (events & ZET_SYSMAN_EVENT_TYPE_ENERGY_THRESHOLD_CROSSED) {
        LOG_INFO << "Event received as Energy consumption threshold crossed";
      } else {
        LOG_INFO << "Spurious event for energy threshold received";
        FAIL();
      }
    }
  }
}

TEST_F(
    EventsTest,
    GivenValidEventHandleWhenListeningEventForHealthofDeviceMemoryChangeThenCorrespondingEventsReceived) {
  for (auto device : devices) {
    auto hEvent = lzt::get_event_handle(device);
    ASSERT_FALSE(hEvent == NULL);
    zet_event_config_t eventConfig;
    eventConfig.registered = ZET_SYSMAN_EVENT_TYPE_MEM_HEALTH;
    lzt::set_event_config(hEvent, eventConfig);

    // If we registered to receive events on any devices, start listening now
    uint32_t events;
    LOG_INFO << "Listening for event if device memory health changes  ...";
    ASSERT_EQ(ZE_RESULT_SUCCESS,
              lzt::listen_event(hDriver, ZET_EVENT_WAIT_INFINITE, 1, &hEvent,
                                &events));
    ASSERT_EQ(ZE_RESULT_SUCCESS, lzt::get_event_state(hEvent, true, events));
    if (events & ZET_SYSMAN_EVENT_TYPE_MEM_HEALTH) {
      LOG_INFO << "Event received as change in device memory health occurred";
    } else {
      LOG_INFO << "Spurious event for change in device memory health received";
      FAIL();
    }
  }
}

TEST_F(
    EventsTest,
    GivenValidEventHandleWhenListeningEventForHealthofFabricPortChangeThenCorrespondingEventsReceived) {
  for (auto device : devices) {
    auto hEvent = lzt::get_event_handle(device);
    ASSERT_FALSE(hEvent == NULL);
    zet_event_config_t eventConfig;
    eventConfig.registered = ZET_SYSMAN_EVENT_TYPE_FABRIC_PORT_HEALTH;
    lzt::set_event_config(hEvent, eventConfig);

    // If we registered to receive events on any devices, start listening now
    uint32_t events;
    LOG_INFO << "Listening for event if fabric port health changes  ...";
    ASSERT_EQ(ZE_RESULT_SUCCESS,
              lzt::listen_event(hDriver, ZET_EVENT_WAIT_INFINITE, 1, &hEvent,
                                &events));
    ASSERT_EQ(ZE_RESULT_SUCCESS, lzt::get_event_state(hEvent, true, events));
    if (events & ZET_SYSMAN_EVENT_TYPE_FABRIC_PORT_HEALTH) {
      LOG_INFO << "Event received as change in fabric port health occurred";
    } else {
      LOG_INFO << "Spurious event for change in fabric port health received";
      FAIL();
    }
  }
}

TEST_F(
    EventsTest,
    GivenValidEventHandleWhenListeningEventForCrossingTotalRASErrorsThresholdThenEventsAreTriggeredForAccordingly) {
  for (auto device : devices) {
    auto hEvent = lzt::get_event_handle(device);
    ASSERT_FALSE(hEvent == NULL);
    uint32_t count = 0;
    auto rasHandles = lzt::get_ras_handles(device, count);
    for (auto rasHandle : rasHandles) {
      auto props = lzt::get_ras_properties(rasHandle);
      if (props.type != ZET_RAS_ERROR_TYPE_CORRECTABLE ||
          props.type != ZET_RAS_ERROR_TYPE_UNCORRECTABLE) {
        continue;
      }
      auto config = lzt::get_ras_config(rasHandle);
      // Aim to receive event for RAS errors after setting RAS threshold 20%
      // more than current threshold
      config.totalThreshold = 1.20 * (config.totalThreshold);
      lzt::set_ras_config(rasHandle, config);

      zet_event_config_t eventConfig;
      switch (props.type) {
      case ZET_RAS_ERROR_TYPE_CORRECTABLE:
        eventConfig.registered = ZET_SYSMAN_EVENT_TYPE_RAS_CORRECTABLE_ERRORS;
        break;
      case ZET_RAS_ERROR_TYPE_UNCORRECTABLE:
        eventConfig.registered = ZET_SYSMAN_EVENT_TYPE_RAS_UNCORRECTABLE_ERRORS;
        break;
      default:
        break;
        ;
      }
      lzt::set_event_config(hEvent, eventConfig);

      // If we registered to receive events on any devices, start listening now
      uint32_t events;
      LOG_INFO
          << "Listening for RAS correctable/uncorrectable errors event ...";
      ASSERT_EQ(ZE_RESULT_SUCCESS,
                lzt::listen_event(hDriver, ZET_EVENT_WAIT_INFINITE, 1, &hEvent,
                                  &events));
      ASSERT_EQ(ZE_RESULT_SUCCESS, lzt::get_event_state(hEvent, true, events));
      if (events & ZET_SYSMAN_EVENT_TYPE_RAS_CORRECTABLE_ERRORS) {
        LOG_INFO << "Event received RAS correctable errors threshold crossed";
      } else if (events & ZET_SYSMAN_EVENT_TYPE_RAS_UNCORRECTABLE_ERRORS) {
        LOG_INFO
            << "Event received as RAS uncorrectable errors threshold crossed";
      } else {
        LOG_INFO << "Spurious event for RAS errors received";
        FAIL();
      }
    }
  }
}

} // namespace