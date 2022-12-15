# test_debug

## Description

Test Suite for oneAPI Level Zero Debug capability.
this test validates the following scenarios:

- Attach and detach
- Event notifications
- Run control
- Thread memory access

for Debug APIs.

## Environment Variables

- `ZET_ENABLE_PROGRAM_DEBUGGING` - set to `1` to enable debug mode

  This is done in the helper application that the test suite runs to verify the debug APIs.

- `LZT_DEBUG_ONE_MODULE_EVENT_PER_KERNEL` - set to `1` to indicate that the driver generates a `ZET_DEBUG_EVENT_TYPE_MODULE_LOAD`  for each kernel within the modules used by the application in the test. set to `0` to indicate that the driver generates 1 `ZET_DEBUG_EVENT_TYPE_MODULE_LOAD`  for the entire module used by the application in the test.

    The default behaviour is to assume 1 event generated per kernel within a module.