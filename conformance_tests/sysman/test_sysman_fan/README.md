# testSysmanFan

This test suite is for checking sysman operations on Fan module. There are specific CTS for different Fan module APIs.
# zesDeviceEnumFans
For this API there are four test cases.
* GivenComponentCountZeroWhenRetrievingSysmanHandlesThenNonZeroCountIsReturned :- Test case checks whether API returns non zero count value for number of fan handles, when zero is passed as second parameter and nullptr is passed as third parameter.
* GivenComponentCountZeroWhenRetrievingSysmanHandlesThenNotNullFanHandlesAreReturned :- Test case checks whether fan handles retrieved are valid or not, when actual count value is passed as second parameter and nullptr is passed as third parameter.
* GivenInvalidComponentCountWhenRetrievingSysmanHandlesThenActualComponentCountIsUpdated :- Test case checks whether count value is updated to its actual value, when illegal count value is passed as second argument.
* GivenValidComponentCountWhenCallingApiTwiceThenSimilarFanHandlesReturned :- Test checks whether API returns consistent values by retrieving array of Fan handles twice and comparing them.

# zesFanGetProperties
For this API there are two test cases.
* GivenValidFanHandleWhenRetrievingFanPropertiesThenValidPropertiesAreReturned :- Test case checks whether API is returning success or not and properties returned are valid values and not incorrect out-of-bound ones.
* GivenValidFanHandleWhenRetrievingFanPropertiesThenExpectSamePropertiesReturnedTwice :- Test checks whether API returns consistent values by retrieving Fan properties twice and comparing them.
# zesFanGetState
For this API there is one test case.
* GivenValidFanHandleWhenGettingFanStateThenValidStatesAreReturned :- Test case checks whether states API is returning success or not and states returned are valid values and not incorrect out-of-bound ones.
# zesFanGetConfig
For this API there is one test case
* GivenValidFanHandleWhenRetrievingFanConfigurationThenValidFanConfigurationIsReturned :- Test case checks whether API is returning success or not and returned values of configuration parameters are valid or not.
# zesFanSetDefaultMode
* GivenValidFanHandleWhenSettingFanToDefaultModeThenValidFanConfigurationIsReturned :- Test case sets fan mode to ZES_FAN_SPEED_MODE_DEFAULT, after setting values test reads those values back and then verifies them using configuration mode. 
# zesFanSetFixedSpeedMode
* GivenValidFanHandleWhenSettingFanToFixedSpeedModeThenValidFanConfigurationIsReturned :- Test case sets the fan to rotate at a fixed speed, after setting values test reads those values back and then verifies them using configuration mode. 
# zesFanSetSpeedTableMode
* GivenValidFanHandleWhenSettingFanToSpeedTableModeThenValidFanConfigurationIsReturned :- Test case sets the fan to adjust speed based on a temperature/speed table, after setting values test reads those values back and then verifies them using configuration mode. 