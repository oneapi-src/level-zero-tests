# testSysmanFrequencyOverclocking

Test Suite for validating sysman frequency overclocking.

# zetSysmanFrequencyOcGetCapabilities
For this API there are two test cases.
* GivenValidFrequencyHandlesWhenGettingOverclockingCapabilitiesThenSuccessIsReturned :- Test case checks whether API is returning success or not and overclocking capabilities returned are valid values and not incorrect out-of-bound ones.
* GivenValidFrequencyHandlesWhenCallingApiTwiceThenSimilarOverclockingCapabilitiesAreReturned :- Test checks whether API returns consistent values by retrieving frequency overclocking capabilities twice and comparing them.
# zetSysmanFrequencyOcGetConfig
For this API there is one test case
* GivenValidFrequencyHandlesWhenGettingOverclockingConfigurationThenSuccessIsReturned :- Test case checks whether API is returning success or not and returned values of configuration parameters are valid or not.
# zetSysmanFrequencyOcSetConfig
For this API there are two test cases
* GivenValidFrequencyHandlesWhenSettingOverclockingConfigThenExpectzetSysmanFrequencyOcSetConfigFollowedByzetSysmanFrequencyOcGetConfigToMatch :- Test case first records initial values of configuration parameters, then sets the value of parameters, after setting values test reads those values back and then verifies them. 
* GivenValidFrequencyHandlesWhenSettingOverclockingConfigToOffThenExpectzetSysmanFrequencyOcSetConfigFollowedByzetSysmanFrequencyOcGetConfigToMatch :- Test case turns the overclocking mode and then verifies it.
# zetSysmanFrequencyOcGetIccMax
* GivenValidFrequencyHandlesWhenGettingCurrentLimitThenSuccessIsReturned :- Test case gets the maximum current limit value for given frequency domain.
# zetSysmanFrequencyOcSetIccMax
* GivenValidFrequencyHandlesWhenSettingCurrentLimitThenExpectzetSysmanFrequencyOcSetIccMaxFollowedByzetSysmanFrequencyOcGetIccMaxToReturnSuccess :- Test case sets the value of maximum current limit and then verifies it.
# zetSysmanFrequencyOcGetTjMax
* GivenValidFrequencyHandlesWhenGettingTemperatureLimitThenSuccessIsReturned :- Test case gets the maximum temperature limit value for given frequency domain.
# zetSysmanFrequencyOcSetTjMax
* GivenValidFrequencyHandlesWhenSettingTemperatureLimitThenExpectzetSysmanFrequencyOcSetTjMaxFollowedByzetSysmanFrequencyOcGetTjMaxToReturnSuccess :- Test case sets the value of maximum temperature limit and then verifies it.

