# oneAPI Level Zero Test Report Generator

`run_test_report.py` provides a method to execute the oneAPI Level Zero Conformance & Negative Tests split by gtest filters in order to generate test reports showing the health of a Level Zero configuration. These test reports have both preset filters to allow for quickly testing basic, advanced, discrete, tools, stress, & negative features of Level Zero in addtion to allowing for custom filters of the test content.

**NOTE: this script requires the tests to be written with the google test framework. If a given binary does not have this framework, it will be skipped.**

`level_zero_report_utils.py` is as python module which provides utility functions used by the `run_test_report.py` script for processing each test into categories of features and test types for oneAPI Level Zero.

**Prerequisites:**
 * python 3
 * oneAPI Level Zero Loader
 * oneAPI Level Zero Driver
 * oneAPI Level Zero Tests (Compiled Conformance, Layer, & Negative tests)

## Arguments for `run_test_report.py`
### Required Arguments
 * --binary_dir BINARY_DIR
   * Directory which contains the oneAPI Level Zero test host binaries and SPVs
     * **NOTE: This argument is required in order to have the correct path to the oneAPI Level Zero Tests to run for the report.**
### Optional Arguments
 * -h, --help
   * Display usage help for `run_test_report.py`
 * --run_test_types RUN_TEST_TYPES
   * List of Test types to include Comma Separated: basic,advanced,discrete,tools,negative,stress,all
   * `basic` = Standardized tests for verifying Core Level Zero functionality
   * `advanced` = Tests spanning features not commonly used or expanded Standardized tests
   * `discrete` = Tests spanning features designed for discrete accelerator devices
   * `tools` = Tests spanning all oneAPI Level Zero Tools
   * `negative` = Tests exercising support for handling negative/invalid user input, expected to be run with a validation layer ie ZE_ENABLE_VALIDATION_LAYER=1
   * `stress` = Tests tests which exercise the maximum limits of a given oneAPI Level Zero Driver
   * `all` = Run all test types
     * **NOTE: all sets all types**
     * **NOTE: Default is `basic` if this argument is unset**
 * --run_test_features RUN_TEST_FEATURES
   * List of Test Features to include Comma Separated: barrier,... NOTE: each test has one Feature assigned, it is the "primary" feature being tested
   * **oneAPI Level Zero Core Features**
     * `Driver Handles`
     * `Device Handling`
     * `Barriers`
     * `Command Lists`
     * `Images`
     * `Command Queues`
     * `Fences`
     * `Device Memory`
     * `Host Memory`
     * `Shared Memory`
     * `Events`
     * `Kernels`
     * `Image Samplers`
     * `Sub-Devices`
     * `Allocation Residency`
     * `Inter-Process Communication`
     * `Peer-To-Peer`
     * `Unified Shared Memory`
     * `Thread Safety Support`
   * **oneAPI Level Zero Tools Features**
     * `Program Instrumentation`
     * `API Tracing`
     * `SysMan Frequency`
     * `SysMan Device Properties`
     * `SysMan PCIe`
     * `SysMan Power`
     * `SysMan Standby`
     * `SysMan LEDs`
     * `SysMan Memory`
     * `SysMan Engines`
     * `SysMan Temperature`
     * `SysMan Power Supplies`
     * `SysMan Fans`
     * `SysMan Reliability & Stability`
     * `SysMan Fabric`
     * `SysMan Diagnostics`
     * `SysMan Device Reset`
     * `SysMan Device Properties`
     * `SysMan Events`
     * `SysMan Frequency`
     * `SysMan Scheduler`
     * `SysMan Firmware`
     * `SysMan Perf Profiles`
     * `Metrics`
     * `Program Debug`
 * --run_test_regex RUN_TEST_REGEX
   * Regular Expression to filter tests that match either in the test name or filter ex:GivenBarrier*
 * --exclude_features EXCLUDE_FEATURES
   * List of Test Features to exclude Comma Separated: barrier,...
   * see --run_test_features RUN_TEST_FEATURES for list of features possible
 * --exclude_regex EXCLUDE_REGEX
   * Regular Expression to exclude tests that match either in the name or filter: GivenBarrier*
 * --test_run_timeout TEST_RUN_TIMEOUT
   * Adjust the timeout for test binary execution, this is for each call to the binary with the filter
 * --log_prefix LOG_PREFIX
   * Change the prefix name for the results such that the output is `<prefix>_results.csv` & `<prefix>_failure_log.txt`
* --export_test_plan EXPORT_TEST_PLAN
   * Export the Test Plan generated thru the filters provided as `<name>.csv`
* --import_test_plan IMPORT_TEST_PLAN
   * Import a Test Plan generated previously thru this tool as `<name>.csv`

## Execution
 * **Required: Ensure the oneAPI Level Zero Loader Library & your desired oneAPI Level Zero Driver are in your library path before execution.**
 * `python3 scripts/run_test_report.py <options>`
 * **Example:** `python3 scripts/run_test_report.py --run_test_type="basic" --binary_dir build/out/`

## Output

example command line output:

    Level Zero Test Report Generator

    Executing Tests which match Test Type(s):basic

    Generated Test List

    Running:101 Tests
    <----------------------------------------------------------------------------------------------------->

    Generating Test Report

    Completed Test Report, see results in level_zero_tests_results.csv

    Overall Results| Total Tests: 101 Passed Tests: 51 Failed Tests: 50 Skipped Tests: 0 Pass Rate: 50%


    Tests Failed, see verbose error results in level_zero_tests_failure_log.txt

example Test Report CSV:

    Name,Feature,Test Type,Result
    L0_CTS_zeCommandListAppendBarrierTests_GivenEmptyCommandListWhenAppendingBarrierThenSuccessIsReturned,Barriers,basic,PASSED
    L0_CTS_zeEventPoolCommandListAppendBarrierTests_GivenEmptyCommandListWhenAppendingBarrierWithEventThenSuccessIsReturned,Barriers,basic,PASSED
    L0_CTS_zeEventPoolCommandListAppendBarrierTests_GivenEmptyCommandListWhenAppendingBarrierWithEventsThenSuccessIsReturned,Barriers,basic,PASSED
    L0_CTS_zeEventPoolCommandListAppendBarrierTests_GivenEmptyCommandListWhenAppendingBarrierWithSignalEventAndWaitEventsThenSuccessIsReturned,Barriers,basic,PASSED
    L0_CTS_zeBarrierEventSignalingTests_GivenEventSignaledWhenCommandListAppendingBarrierThenHostDetectsEventSuccessfully,Barriers,basic,PASSED
    L0_CTS_zeBarrierEventSignalingTests_GivenCommandListAppendingBarrierWaitsForEventsWhenHostAndCommandListSendSignalsThenCommandListExecutesSuccessfully,Barriers,basic,PASSED

    ,,,,Total Passed,Total Failed,Total Skipped,Pass Rate
    ,,,,6,0,0,100%

example Test Failure Report:

    L0_CTS_zeDriverGetDriverVersionTests_GivenZeroVersionWhenGettingDriverVersionThenNonZeroVersionIsReturned FAILED
    [info] Driver version: 0

    Note: Google Test filter = *GivenZeroVersionWhenGettingDriverVersionThenNonZeroVersionIsReturned*

    [==========] Running 1 test from 1 test case.

    [----------] Global test environment set-up.

    [----------] 1 test from zeDriverGetDriverVersionTests

    [ RUN      ] zeDriverGetDriverVersionTests.GivenZeroVersionWhenGettingDriverVersionThenNonZeroVersionIsReturned

    level_zero_tests/utils/test_harness/src/test_harness_driver.cpp:42: Failure

    Expected: (driverVersion) != (0), actual: 0 vs 0

    [  FAILED  ] zeDriverGetDriverVersionTests.GivenZeroVersionWhenGettingDriverVersionThenNonZeroVersionIsReturned (1 ms)

    [----------] 1 test from zeDriverGetDriverVersionTests (1 ms total)



    [----------] Global test environment tear-down

    [==========] 1 test from 1 test case ran. (1 ms total)

    [  PASSED  ] 0 tests.

    [  FAILED  ] 1 test, listed below:

    [  FAILED  ] zeDriverGetDriverVersionTests.GivenZeroVersionWhenGettingDriverVersionThenNonZeroVersionIsReturned

