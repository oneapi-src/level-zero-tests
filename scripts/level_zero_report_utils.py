#!/usr/bin/env python3
# Copyright (C) 2021 Intel Corporation
# SPDX-License-Identifier: MIT

import re

def assign_test_feature_tag(test_feature: str, test_name: str, test_section: str,):
        test_feature_tag = ""
        if test_section == "Core":
            if test_feature == "Allocation Residency" or \
                    (test_feature == "Barriers" and test_name.find("MemoryRange")!= -1) or \
                    (test_feature == "Barriers" and test_name.find("System")!= -1) or \
                    (test_feature == "Command Lists" and test_name.find("Immediate")!= -1) or \
                    (test_feature == "Images" and test_name.find("Immediate")!= -1) or \
                    (test_feature == "Command Lists" and test_name.find("CreateImmediate")!= -1) or \
                    (test_feature == "Command Queues" and test_name.find("Flag")!= -1) or \
                    (test_feature == "Command Queues" and test_name.find("Priority")!= -1) or \
                    (test_feature == "Device Handling" and test_name.find("Cache")!= -1) or \
                    (test_feature == "Device Handling" and test_name.find("ImageProperties")!= -1) or \
                    (test_feature == "Inter-Process Communication" and test_name.find("event")!= -1) or \
                    (test_feature == "Inter-Process Communication" and test_name.find("Event")!= -1) or \
                    (test_feature == "Kernels" and test_name.find("Indirect")!= -1) or \
                    (test_feature == "Kernels" and test_name.find("Attributes")!= -1) or \
                    (test_feature == "Kernels" and test_name.find("Names")!= -1) or \
                    (test_feature == "Kernels" and test_name.find("Cache")!= -1) or \
                    (test_feature == "Kernels" and test_name.find("GlobalPointer")!= -1) or \
                    (test_feature == "Kernels" and test_name.find("FunctionPointer")!= -1) or \
                    (test_feature == "Driver Handles" and test_name.find("GetExtentionFunction")!= -1) or \
                    (test_feature == "Unified Shared Memory" and test_name.find("SystemMemory")!= -1) or \
                    (test_feature == "Events" and test_name.find("Profiling")!= -1) or \
                    (test_feature == "Kernels" and test_name.find("Constants")!= -1) or \
                    (test_feature == "Images") or \
                    (test_feature == "Image Samplers") or \
                    (test_name.find("KernelCopyTests") != -1) or \
                    (test_name.find("Thread") != -1) or \
                    (test_name.find("Affinity") != -1) or \
                    (re.search('concurrent', test_name, re.IGNORECASE)) or \
                    (re.search('context', test_name, re.IGNORECASE)) or \
                    (re.search('KernelOffset', test_name, re.IGNORECASE)) or \
                    (re.search('TestCopyOnly', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_TimestampsTest_GivenExecutedKernelWhenGettingGlobalTimestampsThenDeviceAndHostTimestampDurationsAreClose', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeCommandListAppendMemoryCopyTest_GivenCommandListWithMultipleAppendMemoryCopiesFollowedByResetInLoopThenSuccessIsReturned', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeCommandListAppendMemoryCopyTest_GivenTwoCommandQueuesHavingCommandListsWithScratchSpaceThenSuccessIsReturned', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeCommandListAppendWriteGlobalTimestampTest_GivenCommandListWhenAppendingWriteGlobalTimestampThenSuccessIsReturned', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeCommandListCopyEventTest_GivenSuccessiveMemoryCopiesWithEventWhenExecutingOnDifferentQueuesThenCopiesCompleteSuccessfully', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeDeviceGetExternalMemoryProperties_GivenValidDeviceWhenExportingMemoryAsDMABufThenHostCanMMAPBufferContainingValidData', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeDeviceGetExternalMemoryProperties_GivenValidDeviceWhenImportingMemoryThenImportedBufferHasCorrectData', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeDeviceGetExternalMemoryPropertiesTests_GivenValidDeviceWhenRetrievingExternalMemoryPropertiesThenValidPropertiesAreReturned', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeCommandListEventTests_GivenMemoryFillsThatSignalAndWaitWhenExecutingCommandListThenCommandCompletesSuccessfully', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeCommandListEventTests_GivenMemoryFillThatWaitsOnEventWhenExecutingCommandListThenCommandWaitsAndCompletesSuccessfully', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeCommandListEventTests_GivenMemoryCopyThatWaitsOnEventWhenExecutingCommandListThenCommandWaitsAndCompletesSuccessfully', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeCommandListEventTests_GivenMemoryCopyRegionThatWaitsOnEventWhenExecutingCommandListThenCommandWaitsAndCompletesSuccessfully', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeCommandListEventTests_GivenMemoryCopiesWithDependenciesWhenExecutingCommandListThenCommandsCompletesSuccessfully', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeCommandListAppendMemoryFillTests_GivenDeviceMemorySizeAndValueWhenAppendingMemoryFillWithWaitEventThenSuccessIsReturned', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeCommandListAppendMemoryCopyTests_GivenHostMemoryDeviceHostMemoryAndSizeWhenAppendingMemoryCopyWithWaitEventThenSuccessIsReturned', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeCommandListAppendMemoryCopyTests_GivenHostMemoryDeviceHostMemoryAndSizeWhenAppendingMemoryCopyRegionWithWaitEventThenSuccessIsReturned', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeKernelLaunchTests_GivenBufferLargerThan4GBWhenExecutingFunctionThenFunctionExecutesSuccessfully', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeCommandListEventTests_GivenMemoryCopyRegionWithDependenciesWhenExecutingCommandListThenCommandCompletesSuccessfully', test_name, re.IGNORECASE)) or \
                    (test_name.find("Cooperative")!= -1):
                test_feature_tag = "advanced"
            elif (test_name.find("MemAdvise")!= -1) or \
                    (test_name.find("MultiDevice")!= -1) or \
                    (test_name.find("MemoryPrefetch")!= -1) or \
                    (test_name.find("MemAdvice")!= -1) or \
                    (test_name.find("MultipleRootDevices") != -1) or \
                    (test_name.find("MultipleSubDevices") != -1) or \
                    test_feature == "Peer-To-Peer" or \
                    test_feature == "Sub-Devices" or \
                    (test_feature == "Unified Shared Memory" and (test_name.find("SystemMemory") == -1)):
                test_feature_tag = "discrete"
            else:
                test_feature_tag = "basic"
        elif test_section == "Tool":
            test_feature_tag = "tools"
        elif test_section == "Negative":
            test_feature_tag = "negative"
        elif test_section == "Stress":
            test_feature_tag = "stress"
        return test_feature_tag

def assign_tool_test_feature(test_binary: str, test_name: str):
    test_feature = "None"
    if test_binary == "test_pin":
        test_feature = "Program Instrumentation"
    elif (re.search('tracing', test_name, re.IGNORECASE)):
        test_feature = "API Tracing"
    elif test_binary == "test_sysman_frequency":
        test_feature = "SysMan Frequency"
    elif test_binary == "test_sysman_init":
        test_feature = "SysMan Device Properties"
    elif test_binary == "test_sysman_pci":
        test_feature = "SysMan PCIe"
    elif (re.search('power', test_name, re.IGNORECASE)):
        test_feature = "SysMan Power"
    elif (re.search('standby', test_name, re.IGNORECASE)):
        test_feature = "SysMan Standby"
    elif test_binary == "test_sysman_led":
        test_feature = "SysMan LEDs"
    elif test_binary == "test_sysman_memory":
        test_feature = "SysMan Memory"
    elif test_binary == "test_sysman_engine":
        test_feature = "SysMan Engines"
    elif test_binary == "test_sysman_temperature":
        test_feature = "SysMan Temperature"
    elif test_binary == "test_sysman_psu":
        test_feature = "SysMan Power Supplies"
    elif test_binary == "test_sysman_fan":
        test_feature = "SysMan Fans"
    elif test_binary == "test_sysman_ras":
        test_feature = "SysMan Reliability & Stability"
    elif test_binary == "test_sysman_fabric":
        test_feature = "SysMan Fabric"
    elif test_binary == "test_sysman_diagnostics":
        test_feature = "SysMan Diagnostics"
    elif test_binary == "test_sysman_device" and test_name.find("Reset") != -1:
        test_feature = "SysMan Device Reset"
    elif test_binary == "test_sysman_device":
        test_feature = "SysMan Device Properties"
    elif test_binary == "test_sysman_events":
        test_feature = "SysMan Events"
    elif test_binary == "test_sysman_overclocking":
        test_feature = "SysMan Frequency"
    elif test_binary == "test_sysman_scheduler":
        test_feature = "SysMan Scheduler"
    elif test_binary == "test_sysman_firmware":
        test_feature = "SysMan Firmware"
    elif (re.search('performance', test_name, re.IGNORECASE)):
        test_feature = "SysMan Perf Profiles"
    elif test_binary == "test_metric":
        test_feature = "Metrics"
    elif (re.search('debug', test_binary, re.IGNORECASE)):
        test_feature = "Program Debug"
    if test_feature == "None":
        print("ERROR: test case " + test_name + " has no assigned feature\n")
    return test_feature

def assign_test_feature(test_binary: str, test_name: str):
        test_feature = "None"
        test_section = "Core"
        if (test_binary == "test_pin") \
            or (re.search('tracing', test_name, re.IGNORECASE)) \
            or (re.search('sysman', test_binary, re.IGNORECASE)) \
            or (re.search('debug', test_binary, re.IGNORECASE)) \
            or (test_binary == "test_metric"):
            test_feature = assign_tool_test_feature(test_binary, test_name)
            test_section = "Tool"
            return test_feature, test_section
        if (re.search('stress', test_name, re.IGNORECASE)):
            test_section = "Stress"
            return test_feature, test_section
        if (test_binary == "test_driver") \
            or (test_binary == "test_driver_errors"):
            test_feature = "Driver Handles"
        if test_binary == "test_driver_init_flag_gpu":
            test_feature = "Driver Handles"
        if test_binary == "test_driver_init_flag_none":
            test_feature = "Driver Handles"
        if (test_binary == "test_device") \
            or (test_binary == "test_device_errors"):
            test_feature = "Device Handling"
        if test_binary == "test_barrier":
            test_feature = "Barriers"
        if (test_binary == "test_cmdlist") \
            or (test_binary == "test_cmdlist_errors"):
            test_feature = "Command Lists"
        if test_binary == "test_cmdlist" and test_name.find("ImageCopy") != -1:
            test_feature = "Images"
        if (test_binary == "test_cmdqueue") \
            or (test_binary == "test_cmdqueue_errors"):
            test_feature = "Command Queues"
        if test_binary == "test_fence":
            test_feature = "Fences"
        if test_binary == "test_memory" and test_name.find("DeviceMem") != -1:
            test_feature = "Device Memory"
        if test_binary == "test_memory" and test_name.find("HostMem") != -1:
            test_feature = "Host Memory"
        if test_binary == "test_copy" and test_name.find("HostMem") != -1:
            test_feature = "Host Memory"
        if test_binary == "test_copy" and test_name.find("DeviceMem") != -1:
            test_feature = "Device Memory"
        if test_binary == "test_memory" and test_name.find("SharedMem")!= -1:
            test_feature = "Shared Memory"
        if test_binary == "test_copy" and test_name.find("SharedMem")!= -1:
            test_feature = "Shared Memory"
        if test_binary == "test_copy" and test_name.find("MemoryFill") != -1 \
            and test_name.find("SharedMem")== -1 \
            and test_name.find("HostMem")== -1:
            test_feature = "Device Memory"
        if test_binary == "test_event":
            test_feature = "Events"
        if test_binary == "test_copy" and test_name.find("SignalsEvent")!= -1:
            test_feature = "Events"
        if (test_binary == "test_module") \
            or (test_binary == "test_module_errors"):
            test_feature = "Kernels"
        if test_binary == "test_sampler":
            test_feature = "Image Samplers"
        if test_binary == "test_device" and test_name.find("Sub") != -1:
            test_feature = "Sub-Devices"
        if test_name.find("SubDevice") != -1:
            test_feature = "Sub-Devices"
        if test_binary == "test_residency":
            test_feature = "Allocation Residency"
        if test_binary == "test_ipc" or test_binary == "test_ipc_memory":
            test_feature = "Inter-Process Communication"
        if test_binary == "test_event" and test_name.find("IpcEventHandle")!= -1:
            test_feature = "Inter-Process Communication"
        if test_binary == "test_event" and test_name.find("IPCFlags")!= -1:
            test_feature = "Inter-Process Communication"
        if test_name.find("DriverGetIPCPropertiesTests") != -1:
            test_feature = "Inter-Process Communication"
        if test_binary == "test_device" and test_name.find("P2PProperties")!= -1:
            test_feature = "Peer-To-Peer"
        if test_binary == "test_device" and test_name.find("CanAccessPeer")!= -1:
            test_feature = "Peer-To-Peer"
        if test_binary.find("p2p")!= -1:
            test_feature = "Peer-To-Peer"
        if test_binary == "test_usm":
            test_feature = "Unified Shared Memory"
        if test_binary == "test_memory" and test_name.find("SystemMemory")!= -1:
            test_feature = "Unified Shared Memory"
        if test_binary == "test_memory" and test_name.find("MemAccess")!= -1:
            test_feature = "Unified Shared Memory"
        if test_binary == "test_memory_overcommit":
            test_feature = "Unified Shared Memory"
        if test_binary == "test_image":
            test_feature = "Images"
        if test_binary == "test_copy" and test_name.find("ImageCopy")!= -1:
            test_feature = "Images"
        if test_name.find("Thread") != -1:
            test_feature = "Thread Safety Support"
        if test_name.find("MemoryCopies") != -1:
            test_feature = "Device Memory"
        if test_feature == "None" and (re.search('copy', test_name, re.IGNORECASE)) != -1:
            test_feature = "Device Memory"
        if test_name.find("GetMemAllocPropertiesTestVaryFlags") != -1:
            test_feature = "Device Memory"
        if test_binary == "test_copy" and test_name.find("MemAdvice")!= -1:
            test_feature = "Device Memory"
        if test_binary == "test_copy" and test_name.find("MemAdvise")!= -1:
            test_feature = "Device Memory"
        if test_binary == "test_copy" and test_name.find("MemoryPrefetch")!= -1:
            test_feature = "Device Memory"
        if test_binary == "test_copy" and test_name.find("KernelCopyTests")!= -1:
            test_feature = "Kernels"
        if test_name.find("zeMemGetAllocPropertiesTestVaryFlagsAndSizeAndAlignment")!= -1:
            test_feature = "Device Handling"
        if test_name.find("L0_CTS_MultiProcessTests_GivenMultipleProcessesUsingMultipleDevicesKernelsExecuteCorrectly")!= -1:
            test_feature = "Kernels"
        if test_feature == "None":
            print("ERROR: test case " + test_name + " has no assigned feature\n")
            exit(-1)

        return test_feature, test_section

def create_test_name(suite_name: str, test_binary: str, line: str):
        test_section = "None"
        updated_suite_name = suite_name.split('/')  # e.g., 'GivenXWhenYThenZ'
        test_suite_name = updated_suite_name[0]
        if (len(updated_suite_name) > 1):
            test_suite_name += "_" + updated_suite_name[1]
        parameterized_case_name = line.split()[0]  # e.g., 'GivenXWhenYThenZ/1  # GetParam() = (0)' or just 'GivenXWhenYThenZ' if not parameterized
        case_name = parameterized_case_name.split('/')[0]  # e.g., 'GivenXWhenYThenZ'
        if (test_binary.find("_errors") != -1):
            test_name = "L0_NEG_" + test_suite_name + "_" + case_name
            test_section = "Negative"
        else:
            test_name = "L0_CTS_" + test_suite_name + "_" + case_name

        return test_name, case_name, test_section