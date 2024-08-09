# testSysmanDriver

## Description

This test suite is for validating driver Extension APIs provided in Sysman. 

### zesDriverGetExtensionProperties

* `GivenValidDriverHandleWhileRetrievingExtensionPropertiesThenValidExtensionPropertiesIsReturned`:
  Test case checks whether zesDriverGetExtensionProperties API returns success and also retrieves valid Extension properties.
  If the feature is not supported then the test will be skipped.

### zesDriverGetExtensionFunctionAddress

* `GivenValidDriverHandleWhileRetrievingExtensionFunctionAddressThenValidAddressIsReturned`:
  Test case checks whether zesDriverGetExtensionFunctionAddress API returns success and 
  retrieves valid function pointer for all vendor-specific or experimental extensions Sysman API's which are listed in the L0 specification.
