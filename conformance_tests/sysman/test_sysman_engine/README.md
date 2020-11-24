# testSysmanEngine

## Description

This test suite is for checking sysman operations for monitoring the activity of one or more engines combined into an Engine Group. the tests are run per device, per Engine handle.

### zetSysmanEngineGet

the following test cases validate the zetSysmanEngineGet() API.
* GivenComponentCountZeroWhenRetrievingSysmanEngineHandlesThenNonZeroCountIsReturned :-
Test case checks if the zetSysmanEngineGet() API returns success and the correct, non-zero values for number of handles available.

* GivenComponentCountZeroWhenRetrievingSysmanEngineHandlesThenNotNullEngineHandlesAreReturned :-
Test case checks initally zetSysmanEngineGet() API return the non-zero count and then calling zetSysmanEngineGet() will return non-Null handles of equal to count.

* GivenInvalidComponentCountWhenRetrievingSysmanEngineHandlesThenActualComponentCountIsUpdated :-
Test case calls zetSysmanEngineGet() and returns some non-zero count and even if you corrupt the count value and call zetSysmanEngineGet(), it will update the count varialbe with correct value.

* GivenValidComponentCountWhenCallingApiTwiceThenSimilarEngineHandlesReturned :-
Test case calls zetSysmanEngine with valid count then it will check whether the same handles will be returned on subsequent calls. 

### zetSysmanEngineGetProperties

the following tests validate zetSysmanEngineGetProperties() API

* GivenValidEngineHandleWhenRetrievingEnginePropertiesThenValidPropertiesAreReturned :-
 Test checks whether the zetSysmanEngineGetProperties() returns success and properties are valid 


* GivenValidEngineHandleWhenRetrievingEnginePropertiesThenExpectSamePropertiesReturnedTwice
 and consistent on subsequent calls and consistent on subsequent calls.

### zetSysmanEngineGetActivity

 The following test validate zetSysmanEngineGetActivity() API

* GivenValidEngineHandleWhenRetrievingEngineActivityStatsThenValidStatsIsReturned :-
   zetSysmanEngineGetActivity() returns successfully  and Validate  the activity stats are the same.
