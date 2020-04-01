# test_device_memory_overcommit

## Description
Conformance test suite which tests device memory
allocation to demonstrate that
when one allocates more device memory than the
device has physically, the device
could effectively page memory onto and off of the
device, without that memory being corrupted.

A memory pattern test is used to test for
memory corruption.
