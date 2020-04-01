# test_sysman_diagnostics

## Description

This test suite is for validating diagnostics APIs provided in sysman.. There are specific CTS for different diagnostics APIs.

### zetSysmanDiagnosticsGet
For this API there are four test cases.
* GivenComponentCountZeroWhenRetrievingDiagnosticsHandlesThenNonZeroCountIsReturned :- Test case checks whether API returns non zero count value for number of handles for available diagnostic test suites that can be run., when zero is passed as second parameter and nullptr is passed as third parameter.
* GivenComponentCountZeroWhenRetrievingDiagnosticsHandlesThenNotNullDiagnosticsHandlesAreReturned :- Test case checks whether diagnostic handles retrieved are valid or not, when actual count value is passed as second parameter and nullptr is passed as third parameter.
* GivenInvalidComponentCountWhenRetrievingDiagnosticsHandlesThenActualComponentCountIsUpdated :- Test case checks whether count value is updated to its actual value, when illegal count value is passed as second argument.
* GivenValidComponentCountWhenCallingApiTwiceThenSimilarDiagHandlesReturned :- Test checks whether API returns consistent values by retrieving array of diagnostic handles twice and comparing them.

### zetSysmanDiagnosticsGetProperties
For this API there are two test cases.
* GivenValidDiagHandleWhenRetrievingDiagPropertiesThenValidPropertiesAreReturned :- Test case checks whether properties API is returning success and properties returned are valid values and not some garbage out of bound value.
* GivenValidDiagHandleWhenRetrievingDiagPropertiesThenExpectSamePropertiesReturnedTwice :- Test checks whether API returns consistent values by retrieving diagnostic properties twice and comparing them.

### zetSysmanDiagnosticsGetTests
For this API there is one test case.
* GivenValidDiagHandleWhenRetrievingDiagTestsThenExpectValidTestsToBeReturned :- In this test case, first we are trying to retrieve individual tests that can be run separately, in  zet_diag_test_t. Then we are validating each element of this  zet_diag_test_t.

### zetSysmanDiagnosticsRunTests
For this there are two test case.
* GivenValidDiagTestsWhenRunningAllDiagTestsThenExpectTestsToRunFine :- Test case checks whether all tests could be run in the diagnostic test suite.
* GivenValidDiagTestsWhenRunningIndividualDiagTestsThenExpectTestsToRunFine :- Test case checks whether individual tests could be run. Those test details are retrieved from zetSysmanDiagnosticsGetTests API.
