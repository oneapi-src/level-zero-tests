# testSysmanDriver

## Description

This test suite is for validating driver Extension APIs provided in Sysman. Ensure that the test executable is run with
root user permissions otherwise test will fail.

### zesDriverGetExtensionProperties

* `GivenValidDriverHandleWhileRetrievingExtensionPropertiesThenValidExtensionPropertiesIsReturned`:
  Test case checks whether zesDriverGetExtensionProperties API returns success and also retrieves valid Extension properties.
  If the feature is not supported then the test will be skipped.

### zesDriverGetExtensionFunctionAddress

* `GivenValidDriverHandleWhileRetrievingExtensionFunctionAddressThenValidAddressIsReturned`:
  Test case checks whether zesDriverGetExtensionFunctionAddress API returns success and 
  retrieves valid function pointer for all vendor-specific or experimental extensions Sysman API.
