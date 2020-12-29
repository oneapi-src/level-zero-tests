# test_sysman_events

## Description

This test suite is for validating different events provided in sysman.. There are specific CTS for different events.

### zetSysmanEventGet

the following test cases validate the zetSysmanEventGet() API.
* GivenValidSysmanHandleWhenRetrievingEventsHandleThenValidEventHandleIsReturned :-
Test case checks if the zetSysmanEventGet() API returns success and the valid event handle is available.

* GivenValidSysmanHandleWhenGettingEventHandleTwiceThenSimilarEventHandlesReturned :-
Test case checks whether we are getting consistent event handles between successive calls to zetSysmanEventGet.

### zetSysmanEventGetConfig
### zetSysmanEventSetConfig
### zetSysmanEventGetState
### zetSysmanEventListen

Please note that all the below test cases will wait infinitely for events. So, until they receive event, test will be blocked.
To validate above APIs we are requesting sysman to trigger various types of supported events. Thus effectively all above APIs are validating in various test cases. Test case details are as follows:-
* GivenValidEventHandleWhenListeningTemperatureEventsForCriticalOrThresholdTempThenEventsAreTriggered :-
This test case validates ZET_SYSMAN_EVENT_TYPE_TEMP_CRITICAL, ZET_SYSMAN_EVENT_TYPE_TEMP_THRESHOLD1 events. Events are triggered by setting threshold in temperature config.

* GivenValidEventHandleWhenListeningEventForDeviceResetByDriverThenEventsAreTriggeredForDeviceReset :-
This test case validates ZET_SYSMAN_EVENT_TYPE_DEVICE_RESET event. Event will be triggered when Device is about to be reset by the driver. User needs to be root, in order to execute this test.

* GivenValidEventHandleWhenListeningEventForDeviceEnteringDeepSleepStateThenEventsAreTriggeredForDeviceSleep
This test case validates ZET_SYSMAN_EVENT_TYPE_DEVICE_SLEEP_STATE_ENTER event. Event will be triggered when Device is about to enter a deep sleep state.

* GivenValidEventHandleWhenListeningEventForDeviceExitingDeepSleepStateThenEventsAreTriggeredForDeviceSleepExit
This test case validates ZET_SYSMAN_EVENT_TYPE_DEVICE_SLEEP_STATE_EXIT event. Evenet will be triggered when Device is about to exit a deep sleep state.

* GivenValidEventHandleWhenListeningEventForFrequencyThrottlingThenEventsAreTriggeredForFrequencyThrottling
This test case validates ZET_SYSMAN_EVENT_TYPE_FREQ_THROTTLED event. Event will be triggered when Frequency starts being throttled.

* GivenValidEventHandleWhenListeningEventForCrossingEnergyThresholdThenEventsAreTriggeredForAccordingly
This test case validates ZET_SYSMAN_EVENT_TYPE_ENERGY_THRESHOLD_CROSSED event. Event will be triggered when Energy consumption threshold is reached.

* GivenValidEventHandleWhenListeningEventForHealthofDeviceMemoryChangeThenCorrespondingEventsReceived
This test case validates ZET_SYSMAN_EVENT_TYPE_MEM_HEALTH event. Event will be triggered when Health of device memory changes

* GivenValidEventHandleWhenListeningEventForHealthofFabricPortChangeThenCorrespondingEventsReceived
This test case validates ZET_SYSMAN_EVENT_TYPE_FABRIC_PORT_HEALTH event. Event will be triggered when Health of fabric port changes

* GivenValidEventHandleWhenListeningEventForCrossingTotalRASErrorsThresholdThenEventsAreTriggeredForAccordingly
This test case validates ZET_SYSMAN_EVENT_TYPE_RAS_CORRECTABLE_ERRORS and ZET_SYSMAN_EVENT_TYPE_RAS_UNCORRECTABLE_ERRORS event. Event will be triggered when RAS correctable/uncorrectable errors cross thresholds.

*
GivenValidDeviceHandleWhenListeningForAListOfEventsThenEventRegisterAPIReturnsProperErrorCodeInCaseEventsAreInvalid
This test case checks for valid events and returns proper error code in case events are invalid.
