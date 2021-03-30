#testSysmanTemperature

This test suite is for checking sysman operations on Temperature module. There are specific CTS for different Temperature module APIs.
#zetSysmanTemperatureGet
For this API there are four test cases.
* GivenComponentCountZeroWhenRetrievingTemsThenNonZeroCountIsReturned :- Test case checks whether API returns non zero count value for number of Temperature handles, when zero is passed as second parameter and nullptr is passed as third parameter.
* GivenComponentCountZeroWhenRetrievingTemsThenNotNullTemsAreReturned :- Test case checks whether temperature handles retrieved are valid or not, when actual count value is passed as second parameter and nullptr is passed as third parameter.
* GivenInvalidComponentCountWhenRetrievingTemsThenActualComponentCountIsUpdated :- Test case checks whether count value is updated to its actual value, when illegal count value is passed as second argument.
* GivenValidComponentCountWhenCallingApiTwiceThenSimilarTemsReturned :- Test checks whether API returns consistent values by retrieving array of temperature handles twice and comparing them.

#zetSysmanTemperatureGetProperties
For this API there are two test cases.
* GivenValidTemWhenRetrievingTempPropertiesThenValidPropertiesAreReturned :- Test case checks whether API is returning success or not and properties returned are valid values and not incorrect out-of-bound ones.
* GivenValidTemWhenRetrievingTempPropertiesThenExpectSamePropertiesReturnedTwice :- Test checks whether API returns consistent values by retrieving temperature properties twice and comparing them.
#zetSysmanTemperatureGetState
For this API there is one test case.
* GivenValidTemWhenGettingTempStateThenValidStatesAreReturned :- Test case checks whether states API is returning success or not and states returned are valid values and not incorrect out-of-bound ones.
#zetSysmanTemperatureGetconfig
For this API there is one test case.
* GivenValidTemWhenRetrievingTempConfigurationThenValidTempConfigurationIsReturned :- Test case checks whether API is returning success or not and returned values of configuration parameters are valid or not.
#zetSysmanTemperatureSetconfig
For this API there is one test case. Ensure that this test should run with root user only otherwise test will fail.
* GivenValidTemWhenSettingTempConfigurationThenExpectzetSysmanTemperatureSetConfigFollowedByzetSysmanTemperatureGetConfigToMatch :- Test case first records initial values of configuration parameters, then sets the value of parameters, after setting values test reads those values back and then verifies them.

