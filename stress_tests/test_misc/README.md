# test_misc

## Description
The stress test suite to verify different scenarios

**zeRunDriverIn57bAddressSpace**
* Purpose
Driver is able to run in 57 bit address space (5-level paging)
* Procedure
Reserve all possible Virtual Address space (48bits) and run driver init and simple test.
* Expected results
Driver should be initialized and test return pass.


