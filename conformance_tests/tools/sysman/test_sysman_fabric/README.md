# test_sysman_fabric

## Description

This test suite is for checking sysman operations on fabric ports. There are specific CTS for different fabric port API's.

### zetSysmanFabricPortGet
For this API there are four test cases.
* GivenComponentCountZeroWhenRetrievingSysmanHandlesThenNonZeroCountIsReturned :- Test case checks whether API returns non zero count value for number of fabricPort handles, when zero is passed as second parameter and nullptr is passed as third parameter.
* GivenComponentCountZeroWhenRetrievingSysmanHandlesThenNotNullFabricPortHandlesAreReturned :- Test case checks whether fabricPort handles retrieved are valid or not, when actual count value is passed as second parameter and nullptr is passed as third parameter.
* GivenInvalidComponentCountWhenRetrievingSysmanHandlesThenActualComponentCountIsUpdated :- Test case checks whether count value is updated to its actual value, when illegal count value is passed as second argument.
* GivenValidComponentCountWhenCallingApiTwiceThenSimilarFabricPortHandlesReturned :- Test checks whether API returns consistent values by retrieving array of fabricPort handles twice and comparing them. 

### zetSysmanFabricPortGetProperties
For this API there are two test cases.
* GivenValidFabricPortHandleWhenRetrievingFabricPortPropertiesThenValidPropertiesAreReturned :- Test case checks whether properties API is retuning success and properties returned are valid values and not some garbage out of bound value.
* GivenValidFabricPortHandleWhenRetrievingFabricPortPropertiesThenExpectSamePropertiesReturnedTwice :- Test checks whether API returns consistent values by retrieving fabricPort properties twice and comparing them.

### zetSysmanFabricPortGetConfig
#zetSysmanFabricPortSetConfig
For both of these APIs there is one test case.
* GivenValidFabricPortHandleWhenSettingPortConfigThenGetSamePortConfig :- In this test case, first we are getting already/default set config value. And then we change that config value. And then we set that config, and finally again get config. Now test case will pass if the config value retrieved in the end is similar to the one that is set.

### zetSysmanFabricPortGetState
For this there is one test case.
* GivenValidFabricPortHandleWhenGettingPortStateThenValidStatesAreReturned :- Test case checks whether states API is retuning success and states returned are valid values and not some garbage out of bound value.

### zetSysmanFabricPortGetThroughput
For this there is one test case.
* GivenValidFabricPortHandleWhenGettingPortThroughputThenValidThroughputAreReturned :- Test case checks whether throughput API is retuning success and throughput returned are valid values and not some garbage out of bound value.
