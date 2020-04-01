# testSysmanFan

This test suite is for checking sysman operations on Fan module. There are specific CTS for different Fan module APIs.
# zetSysmanFanGet
For this API there are four test cases.
* GivenComponentCountZeroWhenRetrievingSysmanHandlesThenNonZeroCountIsReturned :- Test case checks whether API returns non zero count value for number of fan handles, when zero is passed as second parameter and nullptr is passed as third parameter.
* GivenComponentCountZeroWhenRetrievingSysmanHandlesThenNotNullFanHandlesAreReturned :- Test case checks whether fan handles retrieved are valid or not, when actual count value is passed as second parameter and nullptr is passed as third parameter.
* GivenInvalidComponentCountWhenRetrievingSysmanHandlesThenActualComponentCountIsUpdated :- Test case checks whether count value is updated to its actual value, when illegal count value is passed as second argument.
* GivenValidComponentCountWhenCallingApiTwiceThenSimilarFanHandlesReturned :- Test checks whether API returns consistent values by retrieving array of Fan handles twice and comparing them.

# zetSysmanFanGetProperties
For this API there are two test cases.
* GivenValidFanHandleWhenRetrievingFanPropertiesThenValidPropertiesAreReturned :- Test case checks whether API is returning success or not and properties returned are valid values and not incorrect out-of-bound ones.
* GivenValidFanHandleWhenRetrievingFanPropertiesThenExpectSamePropertiesReturnedTwice :- Test checks whether API returns consistent values by retrieving Fan properties twice and comparing them.
# zetSysmanFanGetState
For this API there is one test case.
* GivenValidFanHandleWhenGettingFanStateThenValidStatesAreReturned :- Test case checks whether states API is returning success or not and states returned are valid values and not incorrect out-of-bound ones.
#zetSysmanFanGetconfig
For this API there is one test case
* GivenValidFanHandleWhenRetrievingFanConfigurationThenValidFanConfigurationIsReturned :- Test case checks whether API is returning success or not and returned values of configuration parameters are valid or not.
# zetSysmanFanSetconfig
* GivenValidFanHandleWhenSettingFanConfigurationThenExpectzetSysmanFanSetConfigFollowedByzetSysmanFanGetConfigToMatch :- Test case first records initial values of configuration parameters, then sets the value of parameters, after setting values test reads those values back and then verifies them. 
