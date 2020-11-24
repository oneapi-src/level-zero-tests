# test_frequency
This test suite is for checking sysman frequency functionality.There are specific CTS for different frequency API's.

## Description

### zetSysmanFrequencyGet   
For this API there are four test cases.
* GivenComponentCountZeroWhenRetrievingSysmanHandlesThenNonZeroCountIsReturned :- Test case checks whether API returns non zero count value for number of frequency handles, when zero is passed as second parameter and nullptr is passed as third parameter.
* GivenComponentCountZeroWhenRetrievingSysmanHandlesThenNotNullFrequencyHandlesAreReturned :- Test case checks whether frequency handles retrieved are valid or not, when actual count value is passed as second parameter and nullptr is passed as third parameter.
* GivenComponentCountWhenRetrievingSysmanHandlesThenActualComponentCountIsUpdated :- Test case checks whether count value is updated to its actual value, when illegal count value is passed as second argument. 
* GivenValidComponentCountWhenCallingApiTwiceThenSimilarFrequencyHandlesReturned :- Test checks whether API returns consistent values by retrieving array of frequency handles twice and comparing them.  

### zetSysmanFrequencyGetState
For this API there are two test cases.
* GivenValidDeviceWhenRetrievingFreqStateThenValidFreqStatesAreReturned :- Test case gets the current frequency state and validates it. It does so by first getting valid frequency range that hardware frequency management can request as well as frequency properties of given domain. For given frequency state frequency values such as actual,  maximum frequency supported under the current TDP conditions, efficient minimum frequency and current frequency request are compared against values of frequency range and properties.
* GivenValidFreqRangeWhenRetrievingFreqStateThenValidFreqStatesAreReturned :- Test gets list of available frequencies, then out of all these frequencies first two values are set as min and max respectively using set_range function. Then current state is retrieved and validated.

### zetSysmanFrequencyGetAvailableClocks
For this API there are three test cases.
* GivenValidFrequencyHandleWhenRetrievingAvailableClocksThenSuccessAndSameValuesAreReturnedTwice :- Test case gets list of all available hardware clock frequencies for the frequency domain and checks whether API returns same values twice.
* GivenValidFrequencyHandleWhenRetrievingAvailableClocksThenPositiveAndValidValuesAreReturned :- Test case checks whether all the values are greater than zero and are in ascending order.
* GivenClocksCountWhenRetrievingAvailableClocksThenActualCountIsUpdated :- Test case checks whether count value is updated to its actual value, when illegal count value is passed as second argument, where count value is number of available frequencies for given domain.

### zetSysmanFrequencyGetProperties
For this API there are three test cases.
* GivenValidFrequencyHandleWhenRequestingDeviceGPUTypeThenExpectCanControlPropertyToBeTrue :- Test case checks whether hardware block that given frequency domain controls is of GPU type or memory type, if it is GPU type then software can control the frequency of this domain assuming the user has permissions 
* GivenValidFrequencyHandleWhenRequestingFrequencyPropertiesThenExpectPositiveFrequencyRangeAndSteps :- Test case checks validity of minimum hardware clock frequency, maximum non-overclock hardware clock frequency and minimum step-size for clock frequencies for given frequency domain. 
* GivenSameFrequencyHandleWhenRequestingFrequencyPropertiesThenExpectSamePropertiesOnMultipleCalls :- Test case checks whether API returns consistent values on multiple calls.

### zetSysmanFrequencyGetRange
For this API there are three test cases.
* GivenValidFrequencyCountWhenRequestingFrequencyHandleThenExpectzetSysmanFrequencyGetRangeToReturnSuccessOnMultipleCalls :- Test case checks whether API returns success on multiple calls.
* GivenSameFrequencyHandleWhenRequestingFrequencyRangeThenExpectSameRangeOnMultipleCalls :- Test case checks whether API return same values on multiple calls.
* GivenValidFrequencyCountWhenRequestingFrequencyHandleThenExpectzetSysmanFrequencyGetRangeToReturnValidFrequencyRanges :- Test case checks the validity of frequency range by comparing against minimum and maximum hardware clock frequencies. 

### zetSysmanFrequencySetRange
For this API there is single test case.
* GivenValidFrequencyRangeWhenRequestingSetFrequencyThenExpectUpdatedFrequencyInGetFrequencyCall :- Test case sets the frequency range from list of available frequencies and then validates the updated values.

### zetSysmanFrequencyGetThrottleTime
For this API there is single test case.
* GivenValidFrequencyHandleThenCheckForThrottling :- Test case checks for throttling, first sustained power limit is set to min value, then throttle time before throttling event is calculated. Two threads run in parallel one for giving load to GPU so that it can be throttled and another one for recording throttling event. Once throttling event is recorded, throttle time is calculated. Throttle time before throttling event and after throttling event are compared and sustained power limit is set to initial value.
