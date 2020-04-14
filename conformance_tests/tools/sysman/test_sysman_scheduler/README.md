# testSysmanScheduler

## Description

This test suite is for checking sysman operations on Scheduler module.

### zetSysmanSchedulerGetCurrentMode

* GivenValidSysmanHandleWhenRetrievingCurrentModeThenSuccessIsReturned :- The test case retrieves current scheduler mode and checks the validity of the returned mode.

* GivenValidSysmanHandleWhenRetrievingCurrentModeTwiceThenSameModeIsReturned :- The test case calls API twice and checks whether same mode is returned or not.

### zetSysmanSchedulerGetTimeoutModeProperties

* GivenValidSysmanHandleWhenRetrievingSchedulerTimeOutPropertiesThenSuccessIsReturned :- Test case checks whether API is returning success or not and properties returned for time out mode are valid values and not incorrect out-of-bound ones.

* GivenValidSysmanHandleWhenRetrievingSchedulerTimeOutPropertiesTwiceThenSamePropertiesAreReturned :- The test case calls API twice and checks whether same properties returned or not.

### zetSysmanSchedulerGetTimesliceModeProperties

* GivenValidSysmanHandleWhenRetrievingSchedulerTimeSlicePropertiesThenSuccessIsReturned :- Test case checks whether API is returning success or not and properties returned for time slice mode are valid values and not incorrect out-of-bound ones.

* GivenValidSysmanHandleWhenRetrievingSchedulerTimeSlicePropertiesTwiceThenSamePropertiesAreReturned :- The test case calls API twice and checks whether same properties returned or not.

### zetSysmanSchedulerSetTimeoutMode

* GivenValidSysmanHandleWhenSettingSchedulerTimeOutModeThenSuccessIsReturned :- The test case changes the scheduler mode to time out mode and then verifies it.

### zetSysmanSchedulerSetTimesliceMode

* GivenValidSysmanHandleWhenSettingSchedulerTimeSliceModeThenSuccessIsReturned :- The test case changes the scheduler mode to time slice mode and then verifies it.

### zetSysmanSchedulerSetExclusiveMode

* GivenValidSysmanHandleWhenSettingSchedulerExclusiveModeThenSuccesseReturned :- The test case changes the scheduler mode to exclusive mode and then verifies it. 

### zetSysmanSchedulerSetComputeUnitDebugMode

* GivenSysmanHandlesWhenSettingSchedulerComputeUnitDebugModeThenSuccessIsReturned :- The test case changes the scheduler mode to compute unit debug mode and then verifies it.
