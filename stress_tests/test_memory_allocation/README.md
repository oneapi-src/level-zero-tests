# test_memory_allocation

## Description
The stress test suite to verify device memory 
allocation types near maxMemAllocSize limit.
The test is trying to enforce more demending
conditions to check if there is no any hangs
or memory corruption.
The test iterates over different allocation 
types and different user allocation request.
Additionaly cases are prepared to manually
limit maxMemAllocSize.
If Users requests less single allocation size
then more kernel dispatches is calculated.
Max allaction size request will generate
only one kernel dispatch.
Finally output memory patters are verified
on host side.


