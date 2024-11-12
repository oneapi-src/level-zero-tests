# testSysmanInit

This test suite is for validating sysman init function works correctly both with and without core init

Run each of the test as a separate process with Gtest filter (Reason : Init is done only once per process)