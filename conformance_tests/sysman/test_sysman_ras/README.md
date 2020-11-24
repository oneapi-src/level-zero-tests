# testSysmanRAS

## Description

This test suite is for checking sysman operations for Reliability,Accessibility and servicablity. the tests are run per device, per RAS handle.

### zetSysmanRasGet

the following test cases validate the zetSysmanRasGet() API.
* GivenComponentCountZeroWhenRetrievingSysmanHandlesThenNonZeroCountIsReturned :-
Test case checks if the zetSysmanRasGet() API returns success and the correct, non-zero values for number of handles available.

### zetSysmanRasGetProperties

the following tests validate zetSysmanRasGetProperties() API
* GivenValidRASHandleWhenRetrievingRASPropertiesThenExpectSamePropertiesReturnedTwice :-
 Test checks whether the zetSysmanRasGetProperties() returns success properties are valid and consistent on subsequent calls.

### zetSysmanRasGetConfig

the Following tests validate zetSysmanRasGetConfig() API.
* GivenValidHandleWhenRetrievingConfigThenUpdatedConfigIsReturned :- Test Checks if the zetSysmanRasGetConfig() API returns successfully and validates the config that is returned.

### zetSysmanRasSetConfig

The Following tests validate zetSysmanRasSetConfig() API.
* GivenValidHandleAndSettingNewConfigWhenRetrievingConfigThenUpdatedConfigIsReturned:- Test checks if zetSysmanRasSetConfig() API returns sucessfully and sets the new config, this is validated by calling zetSysmanRasGetConfig() and checking the validity of the updated config.

### zetSysmanRasGetState
* GivenValidHandleWhenRetrievingStateThenValidStateIsReturned:- Test checks if zetSysmanRasGetState() API  returns successfully the RAS error details and validate the same.

* GivenValidHandleWhenRetrievingStateAfterClearThenUpdatedStateIsReturned:- Test Checks the API's clear function where the updated state is expected to be changed after the state is cleared by passing 1 as the clear variable value.
