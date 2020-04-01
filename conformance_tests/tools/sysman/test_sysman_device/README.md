#testSysmanDevice

This test suite is for validating device APIs provided in sysman.. There are specific CTS for different device  APIs. Ensure that this test should run with root user only otherwise test will fail

#zetSysmanDeviceReset
For this API there is two test cases.
* GivenValidDeviceWhenResettingSysmanDeviceThenSysmanDeviceResetIsSucceded :- Test case checks whether API returns success or not when we request sysmanDeviceReset. If user is root user then only device reset will be succeded else if it is non-root user then it will return ZE_RESULT_ERROR_INSUFFICIENT_PERMISSIONS

* GivenValidDeviceWhenResettingSysmanDeviceNnumberOfTimesThenSysmanDeviceResetAlwaysSucceded :- Test checks when N number of times calling "zetSysmanDeviceReset" should succeed N number of times.

#zetSysmanDeviceGetProperties
For this API there are two test cases.
* GivenValidDiagHandleWhenRetrievingDiagPropertiesThenValidPropertiesAreReturned :- Test case checks whether properties API is returning success and properties returned are valid values and not some garbage out of bound value.
* GivenValidDiagHandleWhenRetrievingDiagPropertiesThenExpectSamePropertiesReturnedTwice :- Test checks whether API returns consistent values by retrieving diagnostic properties twice and comparing them.

