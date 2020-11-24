# testSysmanPsu

Test Suite for validating information about the Psu modules. There are specific CTS for different Psu APIs.

# zetSysmanPsuGet
For this API there are four test cases.
* GivenComponentCountZeroWhenRetrievingSysmanHandlesThenNonZeroCountIsReturned :- Test case checks whether API returns non zero count value for number of Psu handles, when zero is passed as second parameter and nullptr is passed as third parameter.
* GivenComponentCountZeroWhenRetrievingSysmanHandlesThenNotNullPsuHandlesAreReturned :- Test case checks whether Psu handles retrieved are valid or not, when actual count value is passed as second parameter and nullptr is passed as third parameter.
* GivenInvalidComponentCountWhenRetrievingSysmanHandlesThenActualComponentCountIsUpdated :- Test case checks whether count value is updated to its actual value, when illegal count value is passed as second argument.
* GivenValidComponentCountWhenCallingApiTwiceThenSimilarPsuHandlesReturned :- Test checks whether API returns consistent values by retrieving array of Psu handles twice and comparing them.

# zetSysmanPsuGetProperties
For this API there are two test cases.
* GivenValidPsuHandleWhenRetrievingPsuPropertiesThenValidPropertiesAreReturned :- Test case checks whether API is returning success or not and properties returned are valid values and not incorrect out-of-bound ones.
* GivenValidPsuHandleWhenRetrievingPsuPropertiesThenExpectSamePropertiesReturnedTwice :- Test checks whether API returns consistent values by retrieving Psu properties twice and comparing them.
# zetSysmanPsuGetState
For this there is one test case.
* GivenValidPsuHandleWhenGettingPsuStateThenValidStatesAreReturned :- Test case checks whether states API is retuning success and states returned are valid values and not incorrect out-of-bound ones.

