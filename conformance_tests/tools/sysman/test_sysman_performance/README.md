#TestSysmanPerformance

This test suite is for checking sysman operations on Performance module. There are specific CTS for different Performance module APIs.
#zesDeviceEnumPerformanceFactorDomains
For this API there are four test cases.
* GivenComponentCountZeroWhenRetrievingPerformanceHandlesThenNonZeroCountIsReturned :- Test case checks whether API returns non zero count value for number of Performance handles, when zero is passed as second parameter and nullptr is passed as third parameter.
* GivenComponentCountZeroWhenRetrievingSysmanPerformanceThenNotNullPerformanceHandlesAreReturned :- Test case checks whether performance handles retrieved are valid or not, when actual count value is passed as second parameter and nullptr is passed as third parameter.
* GivenInvalidComponentCountWhenRetrievingPerformanceHandlesThenActualComponentCountIsUpdated :- Test case checks whether count value is updated to its actual value, when illegal count value is passed as second argument.
* GivenValidComponentCountWhenCallingApiTwiceThenSimilarPerformanceHandlesReturned :- Test checks whether API returns consistent values by retrieving array of performance handles twice and comparing them.
#zesPerformanceFactorGetProperties
For this API there are two test cases.
* GivenValidPerformanceHandleWhenRetrievingPerformancePropertiesThenValidPropertiesAreReturned :- Test case checks whether API is returning success or not and properties returned are valid values and not incorrect out-of-bound ones.
* GivenValidPerformanceHandleWhenRetrievingPerformancePropertiesThenExpectSamePropertiesReturnedTwice :- Test checks whether API returns consistent values by retrieving performance properties twice and comparing them.
#zesPerformanceFactorGetConfig
For this API there is one test case.
* GivenValidPerformanceHandleWhenGettingPerformanceConfigurationThenValidPerformanceFactorIsReturned :- Test case checks whether API is returning success or not and if the factor value passed by SetConfig is matching or not.
#zesPerformanceFactorSetConfig
For this API there is one test case.
* GivenValidPerformanceHandleWhenSettingPerformanceConfigurationThenSuccessIsReturned :- Test case checks whether API is returning success or not and verifies if the factor value is in valid range.

