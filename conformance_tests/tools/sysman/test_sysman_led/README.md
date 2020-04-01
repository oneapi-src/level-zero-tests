# testSysmanLed

Test Suite for validating information about the Led modules. There are specific CTS for different Led APIs.

# zetSysmanLedGet
For this API there are four test cases.
* GivenComponentCountZeroWhenRetrievingSysmanHandlesThenNonZeroCountIsReturned :- Test case checks whether API returns non zero count value for number of Led handles, when zero is passed as second parameter and nullptr is passed as third parameter.
* GivenComponentCountZeroWhenRetrievingSysmanHandlesThenNotNullLedHandlesAreReturned :- Test case checks whether Led handles retrieved are valid or not, when actual count value is passed as second parameter and nullptr is passed as third parameter.
* GivenInvalidComponentCountWhenRetrievingSysmanHandlesThenActualComponentCountIsUpdated :- Test case checks whether count value is updated to its actual value, when illegal count value is passed as second argument.
* GivenValidComponentCountWhenCallingApiTwiceThenSimilarLedHandlesReturned :- Test checks whether API returns consistent values by retrieving array of Led handles twice and comparing them.

# zetSysmanLedGetProperties
For this API there are two test cases.
* GivenValidLedHandleWhenRetrievingLedPropertiesThenValidPropertiesAreReturned :- Test case checks whether API is retuning success or not and properties returned are valid values and not incorrect out-of-bound ones.
* GivenValidLedHandleWhenRetrievingLedPropertiesThenExpectSamePropertiesReturnedTwice :- Test checks whether API returns consistent values by retrieving Led properties twice and comparing them.
# zetSysmanLedGetState
For this API there is one test case.
* GivenValidLedHandleWhenGettingLedStateThenValidStatesAreReturned :- Test case checks whether states API is returning success and states returned are valid values and not incorrect out-of-bound ones..
# zetSysmanLedSetState
For this API there are two test cases. Ensure that this test should run with root user only otherwise test will fail.
* GivenValidLedHandleWhenSettingLedStateThenExpectzetSysmanLedSetStateFollowedByzetSysmanLedGetStateToMatch :- Test case records initial state, then sets new values to the current state, then it reads state and verifes that changes are reflected or not.
* GivenValidLedHandleWhenSettingLedStateToOffThenExpectzetSysmanLedSetStateFollowedByzetSysmanLedGetStateToMatch :- Test case sets the LED's to off state and verifies whether changes are reflected or not.

