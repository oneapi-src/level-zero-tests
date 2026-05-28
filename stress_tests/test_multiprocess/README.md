# test_multiprocess

## Description
Stress test suite verifying that multiple concurrent processes can initialize and use the Level Zero driver simultaneously without interference.

Each test spawns N child processes (default: 48) using `fork`/`waitpid` on POSIX or `CreateProcess`/`WaitForMultipleObjects` on Windows. Results are collected via Boost.Interprocess shared memory. A child is considered successful when it sets its slot in shared memory and exits with code 0.

---

**GivenNChildProcessesWhenEachRunsZeInitAndDevicePropertiesThenAllSucceed**
* Purpose
Verify that N concurrent processes can each independently initialize the Level Zero driver and query device properties without errors.
* Procedure
Spawn N child processes. Each child calls `zeInit`, obtains the default driver and device, and queries `ze_device_properties_t`.
* Expected results
All child processes exit with code 0 and set their success flag in shared memory.

---

**GivenNChildProcessesWhenKernelExecutionAndHostEventSynchronizedSetThenNotTimeoutsAndErrorsOnImmediateCommandList**
* Purpose
Verify that N concurrent processes can each execute a GPU kernel using an immediate command list without timeouts or errors.
* Procedure
Spawn N child processes. Each child initializes Level Zero, allocates shared USM buffers (256 × uint32), fills the source buffer with `0xDEADBEEF`, dispatches kernel `simple_test` from `test_multiprocess.spv` via an immediate command list, synchronizes with a host-visible event (timeout: 60 s), and verifies the destination buffer.
* Expected results
All child processes exit with code 0 and set their success flag in shared memory.

---

**GivenNChildProcessesWhenKernelExecutionAndHostEventSynchronizedSetThenNotTimeoutsAndErrorsOnRegisteredCommandList**
* Purpose
Verify that N concurrent processes can each execute a GPU kernel using a regular (registered) command list without timeouts or errors.
* Procedure
Same as the immediate command list test, but the kernel is dispatched via a regular command list that is closed and submitted through `execute_and_sync_command_bundle`.
* Expected results
All child processes exit with code 0 and set their success flag in shared memory.


