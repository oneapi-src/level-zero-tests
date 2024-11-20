#!/usr/bin/env python3
# Copyright (C) 2021-2023 Intel Corporation
# SPDX-License-Identifier: MIT

import re

def assign_test_feature_tag(test_feature: str, test_name: str, test_section: str,):
        test_feature_tag = ""
        if test_section == "Core":
            # Include rules for "more than one" for devices from CONTRIBUTING.md
            if (test_name.find("MemAdvise")!= -1) or \
                    (test_name.find("MultipleRootDevice") != -1) or \
                    (test_name.find("MultipleDevice") != -1) or \
                    (test_name.find("DifferentRootDevice") != -1) or \
                    (test_name.find("DifferentDevice") != -1) or \
                    (test_name.find("TwoRootDevice") != -1) or \
                    (test_name.find("TwoDevice") != -1) or \
                    (test_name.find("RemoteDevice") != -1) or \
                    (test_name.find("MultipleSubDevice") != -1) or \
                    (test_name.find("DifferentSubDevice") != -1) or \
                    (test_name.find("TwoSubDevice") != -1) or \
                    (test_name.find("MemoryPrefetch")!= -1) or \
                    (test_name.find("MemAdvice")!= -1) or \
                    test_feature == "Peer-To-Peer" or \
                    test_feature == "Sub-Devices" or \
                    (test_feature == "Unified Shared Memory" and (test_name.find("SystemMemory") == -1)):
                test_feature_tag = "discrete"
            elif test_feature == "Allocation Residency" or \
                    (test_feature == "Barriers" and test_name.find("MemoryRange")!= -1) or \
                    (test_feature == "Barriers" and test_name.find("System")!= -1) or \
                    (test_feature == "Barriers" and test_name.find("Immediate")!= -1) or \
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
                    (test_feature == "Kernels" and test_name.find("Immediate")!= -1) or \
                    (test_feature == "Driver Handles" and test_name.find("GetExtentionFunction")!= -1) or \
                    (test_feature == "Shared Memory" and test_name.find("Immediate")!= -1) or \
                    (test_feature == "Unified Shared Memory" and test_name.find("SystemMemory")!= -1) or \
                    (test_feature == "Events" and test_name.find("Profiling")!= -1) or \
                    (test_feature == "Events" and test_name.find("Immediate")!= -1) or \
                    (test_feature == "Kernels" and test_name.find("Constants")!= -1) or \
                    (test_feature == "Images") or \
                    (test_feature == "Image Samplers") or \
                    (test_feature == "Mutable Command List") or \
                    (test_name.find("KernelCopyTests") != -1) or \
                    (test_name.find("Thread") != -1) or \
                    (test_name.find("Affinity") != -1) or \
                    (test_name.find("Luid") != -1) or \
                    (re.search('concurrent', test_name, re.IGNORECASE)) or \
                    (re.search('context', test_name, re.IGNORECASE)) or \
                    (re.search('KernelOffset', test_name, re.IGNORECASE)) or \
                    (re.search('TestCopyOnly', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_ContextStatusTest_GivenContextCreateWhenUsingValidHandleThenContextGetStatusReturnsSuccess', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_TimestampsTest_GivenExecutedKernelWhenGettingGlobalTimestampsThenDeviceAndHostTimestampDurationsAreClose', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_TimestampsTest_GivenExecutedKernelWhenGettingGlobalTimestampsOnImmediateCmdListThenDeviceAndHostTimestampDurationsAreClose', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeCommandListAppendMemoryCopyTest_GivenCommandListWithMultipleAppendMemoryCopiesFollowedByResetInLoopThenSuccessIsReturned', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeCommandListAppendMemoryCopyTest_GivenTwoCommandQueuesHavingCommandListsWithScratchSpaceThenSuccessIsReturned', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeCommandListAppendMemoryFillVerificationTests_GivenHostMemoryWhenExecutingAMemoryFillOnImmediateCmdListThenMemoryIsSetCorrectly', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeCommandListAppendMemoryFillVerificationTests_GivenDeviceMemoryWhenExecutingAMemoryFillOnImmediateCmdListThenMemoryIsSetCorrectly', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeCommandListAppendWriteGlobalTimestampTest_GivenCommandListWhenAppendingWriteGlobalTimestampThenSuccessIsReturned', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeCommandListCopyEventTest_GivenSuccessiveMemoryCopiesWithEventWhenExecutingOnDifferentQueuesThenCopiesCompleteSuccessfully', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeCommandListCopyEventTest_GivenSuccessiveMemoryCopiesWithEventWhenExecutingOnDifferentQueuesOnImmediateCmdListsThenCopiesCompleteSuccessfully', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeDeviceGetExternalMemoryProperties_GivenValidDeviceWhenExportingMemoryAsDMABufThenHostCanMMAPBufferContainingValidData', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeDeviceGetExternalMemoryProperties_GivenValidDeviceWhenExportingMemoryAsDMABufOnImmediateCmdListThenHostCanMMAPBufferContainingValidData', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeDeviceGetExternalMemoryProperties_GivenValidDeviceWhenImportingMemoryThenImportedBufferHasCorrectData', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeDeviceGetExternalMemoryProperties_GivenValidDeviceWhenImportingMemoryOnImmediateCmdListThenImportedBufferHasCorrectData', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_DeviceCommandQueueGroupsTest_GivenValidDeviceHandlesWhenRequestingQueueGroupPropertiesMultipleTimesThenPropertiesAreInSameOrder', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_DeviceCommandQueueGroupsTest_GivenValidSubDeviceHandlesWhenRequestingQueueGroupPropertiesMultipleTimesThenPropertiesAreInSameOrder', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_DeviceStatusTest_GivenValidDeviceHandlesWhenRequestingStatusThenSuccessReturned', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_DeviceStatusTest_GivenValidSubDeviceHandlesWhenRequestingStatusThenSuccessReturned', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeDeviceGetExternalMemoryPropertiesTests_GivenValidDeviceWhenRetrievingExternalMemoryPropertiesThenValidPropertiesAreReturned', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeDeviceOrderingTests_GivenPCIOrderingForcedWhenEnumeratingDevicesThenDevicesAreEnumeratedInIncreasingOrderOfTheirBDFAddresses', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeCommandListEventTests_GivenMemoryFillsThatSignalAndWaitWhenExecutingCommandListThenCommandCompletesSuccessfully', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeCommandListEventTests_GivenMemoryFillsThatSignalAndWaitWhenExecutingImmediateCommandListThenCommandCompletesSuccessfully', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeCommandListEventTests_GivenMemoryFillThatWaitsOnEventWhenExecutingCommandListThenCommandWaitsAndCompletesSuccessfully', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeCommandListEventTests_GivenMemoryFillThatWaitsOnEventWhenExecutingImmediateCommandListThenCommandWaitsAndCompletesSuccessfully', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeCommandListEventTests_GivenMemoryCopyThatSignalsEventWhenCompleteWhenExecutingImmediateCommandListThenHostAndGpuReadEventCorrectly', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeCommandListEventTests_GivenMemoryCopyThatWaitsOnEventWhenExecutingCommandListThenCommandWaitsAndCompletesSuccessfully', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeCommandListEventTests_GivenMemoryCopyThatWaitsOnEventWhenExecutingImmediateCommandListThenCommandWaitsAndCompletesSuccessfully', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeCommandListEventTests_GivenMemoryCopyRegionThatWaitsOnEventWhenExecutingCommandListThenCommandWaitsAndCompletesSuccessfully', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeCommandListEventTests_GivenMemoryCopyRegionThatWaitsOnEventWhenExecutingImmediateCommandListThenCommandWaitsAndCompletesSuccessfully', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeCommandListEventTests_GivenMemoryCopyRegionThatSignalsEventWhenCompleteWhenExecutingImmediateCommandListThenHostAndGpuReadEventCorrectly', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeCommandListEventTests_GivenMemoryCopiesWithDependenciesWhenExecutingCommandListThenCommandsCompletesSuccessfully', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeCommandListEventTests_GivenMemoryCopiesWithDependenciesWhenExecutingImmediateCommandListThenCommandsCompletesSuccessfully', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeCommandListAppendMemoryFillTests_GivenDeviceMemorySizeAndValueWhenAppendingMemoryFillOnImmediateCmdListThenSuccessIsReturned', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeCommandListAppendMemoryFillTests_GivenDeviceMemorySizeAndValueWhenAppendingMemoryFillWithHEventOnImmediateCmdListThenSuccessIsReturned', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeCommandListAppendMemoryFillTests_GivenDeviceMemorySizeAndValueWhenAppendingMemoryFillWithWaitEventThenSuccessIsReturned', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeCommandListAppendMemoryFillTests_GivenDeviceMemorySizeAndValueWhenAppendingMemoryFillWithWaitEventOnImmediateCmdListThenSuccessIsReturned', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeCommandListAppendMemoryCopyTests_GivenHostMemoryDeviceHostMemoryAndSizeWhenAppendingMemoryCopyOnImmediateCmdListThenSuccessIsReturned', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeCommandListAppendMemoryCopyTests_GivenHostMemoryDeviceHostMemoryAndSizeWhenAppendingMemoryCopyWithWaitEventThenSuccessIsReturned', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeCommandListAppendMemoryCopyTests_GivenHostMemoryDeviceHostMemoryAndSizeWhenAppendingMemoryCopyWithWaitEventOnImmediateCmdListThenSuccessIsReturned', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeCommandListAppendMemoryCopyTests_GivenHostMemoryDeviceHostMemoryAndSizeWhenAppendingMemoryCopyRegionWithWaitEventThenSuccessIsReturned', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeCommandListAppendMemoryCopyTests_GivenHostMemoryDeviceHostMemoryAndSizeWhenAppendingMemoryCopyRegionWithWaitEventOnImmediateCmdListThenSuccessIsReturned', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeCommandListAppendMemoryCopyTests_GivenHostMemoryDeviceHostMemoryAndSizeWhenAppendingMemoryCopyWithHEventOnImmediateCmdListThenSuccessIsReturned', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeCommandListAppendMemoryCopyWithDataVerificationTests_GivenHostMemoryDeviceMemoryAndSizeWhenAppendingMemoryCopyOnImmediateCmdListThenSuccessIsReturnedAndCopyIsCorrect', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeCommandListAppendMemoryCopyWithDataVerificationTests_GivenDeviceMemoryToDeviceMemoryAndSizeWhenAppendingMemoryCopyOnImmediateCmdListThenSuccessIsReturnedAndCopyIsCorrect', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_SyncTimeoutParams_zeCommandQueueSynchronizeTimeoutTests_GivenTimeoutWhenWaitingForCommandQueueThenWaitForSpecifiedTime', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeKernelLaunchTests_GivenBufferLargerThan4GBWhenExecutingFunctionThenFunctionExecutesSuccessfully', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeKernelLaunchTests_GivenBufferLargerThan4GBWhenExecutingFunctionOnImmediateCmdListThenFunctionExecutesSuccessfully', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeCommandListEventTests_GivenMemoryCopyRegionWithDependenciesWhenExecutingCommandListThenCommandCompletesSuccessfully', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeCommandListEventTests_GivenMemoryCopyRegionWithDependenciesWhenExecutingImmediateCommandListThenCommandCompletesSuccessfully', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeCommandListEventTests_GivenMemorySetThatSignalsEventWhenCompleteWhenExecutingImmediateCommandListThenHostAndGpuReadEventCorrectly', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_IndependentCMDListsOverlappingParameterization_zeTestMixedCMDListsIndependentOverlapping_GivenRegularAndImmCMDListsOnComputeEngineThenCorrectResultsAreReturned', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_IndependentCMDListsOverlappingParameterization_zeTestMixedCMDListsIndependentOverlapping_GivenRegularAndImmCMDListsOnCopyEngineThenCorrectResultsAreReturned', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_IndependentCMDListsOverlappingParameterization_zeTestMixedCMDListsIndependentOverlapping_GivenRegularCMDListOnComputeEngineAndImmCMDListOnCopyEngineThenCorrectResultsAreReturned', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_IndependentCMDListsOverlappingParameterization_zeTestMixedCMDListsIndependentOverlapping_GivenRegularCMDListOnCopyEngineAndImmCMDListOnComputeEngineThenCorrectResultsAreReturned', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_InterdependCMDListsPairSameEngineTypeParameterization_zeTestMixedCMDListsInterdependPairSameEngineType_GivenRegularAndImmCMDListPairWithEventDependenciesAndImmCMDListExecutedFirstThenExecuteSuccessfully', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_InterdependCMDListsPairSameEngineTypeParameterization_zeTestMixedCMDListsInterdependPairSameEngineType_GivenRegularAndImmCMDListPairWithEventDependenciesAndRegularCMDListExecutedFirstThenExecuteSuccessfully', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_InterdependCMDListsPipeliningParameterization_zeTestMixedCMDListsInterdependPipelining_GivenRegularAndImmCMDListsOnComputeEngineThenCorrectResultsAreReturned', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_InterdependCMDListsPipeliningParameterization_zeTestMixedCMDListsInterdependPipelining_GivenRegularAndImmCMDListsOnCopyEngineThenCorrectResultsAreReturned', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_InterdependCMDListsPipeliningParameterization_zeTestMixedCMDListsInterdependPipelining_GivenRegularCMDListOnComputeEngineAndImmCMDListOnCopyEngineThenCorrectResultsAreReturned', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_InterdependCMDListsPipeliningParameterization_zeTestMixedCMDListsInterdependPipelining_GivenRegularCMDListOnCopyEngineAndImmCMDListOnComputeEngineThenCorrectResultsAreReturned', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeModuleProgramTests_GivenModulesWithLinkageDependenciesWhenCreatingThenSuccessIsReturned', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeModuleProgramTests_GivenModulesWithLinkageDependenciesWhenCreatingOnImmediateCmdListThenSuccessIsReturned', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeModuleCreateTests_GivenValidDeviceAndBinaryFileWhenCreatingStatelessKernelModuleThenReturnSuccessfulAndDestroyModule', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeModuleCreateTests_GivenModuleCompiledWithOptimizationsWhenExecutingThenResultIsCorrect', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeModuleCreateTests_GivenModuleCompiledWithOptimizationsWhenExecutingOnImmediateCmdListThenResultIsCorrect', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeKernelScheduleHintsTests_GivenKernelScheduleHintWhenRunningKernelThenResultIsCorrect', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeKernelCreateTests_GivenValidFunctionWhenGettingPreferredGroupSizePropertiesThenReturn', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeIpcMemHandleTests_GivenSameIpcMemoryHandleWhenOpeningIpcMemHandleMultipleTimesThenUniquePointersAreReturned', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeIpcMemHandleCloseTests_GivenValidPointerToDeviceMemoryAllocationBiasCachedWhenClosingIpcHandleThenSuccessIsReturned', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeDriverMemFreeTests_GivenValidSharedMemAllocationWhenFreeingSharedMemoryThenSuccessIsReturned', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeDriverMemFreeTests_GivenValidHostMemAllocationWhenFreeingHostMemoryThenSuccessIsReturned', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeDriverGetLastErrorDescription_GivenValidDriverWhenRetreivingErrorDescriptionThenValidStringIsReturned', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeDriverAllocSharedMemTestVarySizeAndAlignment_zeDriverAllocSharedMemAlignmentTests_GivenSizeAndAlignmentWhenAllocatingDeviceMemoryThenPointerIsAligned', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeDriverAllocHostMemTestVarySizeAndAlignment_zeDriverAllocHostMemAlignmentTests_GivenSizeAndAlignmentWhenAllocatingHostMemoryThenPointerIsAligned', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeDriverAllocDeviceMemTestVarySizeAndAlignment_zeDriverAllocDeviceMemAlignmentTests_GivenSizeAndAlignmentWhenAllocatingDeviceMemoryThenPointerIsAligned', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeDriverAllocDeviceMemTestVarySizeAndLargeAlignment_zeDriverAllocDeviceMemAlignmentTests_GivenSizeAndAlignmentWhenAllocatingDeviceMemoryThenPointerIsAligned', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeDriverAllocHostMemTestVarySizeAndLargeAlignment_zeDriverAllocHostMemAlignmentTests_GivenSizeAndAlignmentWhenAllocatingHostMemoryThenPointerIsAligned', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeDriverAllocSharedMemTestVarySizeAndLargeAlignment_zeDriverAllocSharedMemAlignmentTests_GivenSizeAndAlignmentWhenAllocatingSharedMemoryThenPointerIsAligned', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_IpcMemoryAccessTest_GivenL0MemoryAllocatedInChildProcessWhenUsingL0IPCOnImmediateCmdListThenParentProcessReadsMemoryCorrectly', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_IpcMemoryAccessTest_GivenL0MemoryAllocatedInChildProcessBiasCachedWhenUsingL0IPCThenParentProcessReadsMemoryCorrectly', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_IpcMemoryAccessTest_GivenL0MemoryAllocatedInChildProcessBiasCachedWhenUsingL0IPCOnImmediateCmdListThenParentProcessReadsMemoryCorrectly', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_IpcMemoryAccessSubDeviceTest_GivenL0PhysicalMemoryAllocatedReservedInParentProcessWhenUsingL0IPCThenChildProcessReadsMemoryCorrectlyUsingSubDeviceQueue', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_IpcMemoryAccessSubDeviceTest_GivenL0PhysicalMemoryAllocatedReservedInParentProcessWhenUsingL0IPCOnImmediateCmdListThenChildProcessReadsMemoryCorrectlyUsingSubDeviceQueue', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeDeviceGetMemoryPropertiesTests_GivenValidDeviceWhenRetrievingMemoryPropertiesThenValidExtPropertiesAreReturned', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeKernelCreateTests_GivenValidFunctionWhenGettingSourceAttributeThenReturnAttributeString', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeKernelGetNameTests_GivenKernelGetNameCorrectNameIsReturned', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeKernelMaxGroupSize_GivenKernelGetMaxGroupSize', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeModuleCreateTests_GivenModuleGetPropertiesReturnsValidNonZeroProperties', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeDriverAllocSharedMemTestVarySizeAndAlignment_zeDriverAllocSharedMemAlignmentTests_GivenSizeAndAlignmentWhenAllocatingSharedMemoryThenPointerIsAligned', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeEventSignalScopeParameterizedTest_zeEventSignalScopeTests_GivenDifferentEventSignalScopeFlagsThenAppendSignalEventIsSuccessful', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_EventIndependenceParameterization_zeEventCommandQueueAndCommandListIndependenceTests_GivenCommandQueueAndCommandListDestroyedThenSynchronizationOnAllEventsAreSuccessful', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_SyncTimeoutParams_zeEventHostSynchronizeTimeoutTests_GivenTimeoutWhenWaitingForEventThenWaitForSpecifiedTime', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeDeviceGetModulePropertiesTests_GivenValidDeviceWhenRetrievingFloatAtomicPropertiesThenValidPropertiesReturned', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeDevicePciGetPropertiesTests_GivenValidDeviceWhenRetrievingPciSpeedPropertiesThenValidPropertiesAreReturned', test_name, re.IGNORECASE)) or \
                    (re.search('L0_CTS_zeDeviceP2PBandwidthExpProperties_GivenValidDevicesWhenRetrievingP2PBandwidthPropertiesThenValidPropertiesAreReturned', test_name, re.IGNORECASE)) or \
                    (test_name.find("ParamAppendMemCopy")!= -1) or \
                    (test_name.find("ParamSharedSystemMemCopy")!= -1) or \
                    (test_name.find("zeVirtualMemoryTests")!= -1) or \
                    (test_name.find("Cooperative")!= -1) or \
                    (test_name.find("zeMemFreeExtTests")!= -1) or \
                    (test_name.find("zeMemFreeExtMultipleTests")!= -1) or \
                    (test_name.find("PutIpcMemoryAccessTest")!= -1) or \
                    (test_name.find("zePutIpcMemHandleTests")!= -1) or \
                    (test_name.find("AtomicAccessTests")!= -1) or \
                    (test_name.find("PutIpcMemoryAccessSubDeviceTest")!= -1) or \
                    (re.search('L0_CTS_IpcMemoryAccessTest_GivenL0Physical', test_name, re.IGNORECASE)) or \
                    (re.search('GivenVpuOnlyFlagThenGpuOnlyFlagWhenInitializingDriverThenSuccessOrUninitIsReturnedAndAtLeastOneDriverHandleIsReturned', test_name, re.IGNORECASE)) or \
                    (re.search('GivenGpuOnlyFlagThenVpuOnlyFlagWhenInitializingDriverThenSuccessOrUninitIsReturnedAndAtLeastOneDriverHandleIsReturned', test_name, re.IGNORECASE)) or \
                    (re.search('GivenVpuOnlyFlagWhenInitializingDriverThenSuccessOrUninitIsReturned', test_name, re.IGNORECASE)) or \
                    (re.search('GivenValidDeviceWhenImportingMemoryWithNTHandleThenImportedBufferHasCorrectData', test_name, re.IGNORECASE)) or \
                    (re.search('GivenValidDeviceWhenImportingMemoryWithNTHandleOnImmediateCmdListThenImportedBufferHasCorrectData', test_name, re.IGNORECASE)) or \
                    (re.search('GivenValidDeviceWhenImportingMemoryWithKMTHandleThenImportedBufferHasCorrectData', test_name, re.IGNORECASE)) or \
                    (re.search('GivenValidDeviceWhenImportingMemoryWithKMTHandleOnImmediateCmdListThenImportedBufferHasCorrectData', test_name, re.IGNORECASE)) or \
                    (re.search('GivenValidDeviceWhenExportingMemoryWithD3DTextureThenResourceSuccessfullyExported', test_name, re.IGNORECASE)) or \
                    (re.search('GivenValidDeviceWhenExportingMemoryWithD3DTextureKmtThenResourceSuccessfullyExported', test_name, re.IGNORECASE)) or \
                    (re.search('GivenValidDeviceWhenExportingMemoryWithD3D12HeapThenResourceSuccessfullyExported', test_name, re.IGNORECASE)) or \
                    (re.search('GivenValidDeviceWhenExportingMemoryWithD3D12ResourceThenResourceSuccessfullyExported', test_name, re.IGNORECASE)) or \
                    (re.search('GivenMaxMemoryFillPatternSizeForEachCommandQueueGroupWhenAppendingMemoryFillThenSuccessIsReturned', test_name, re.IGNORECASE)) or \
                    (re.search('GivenImmediateCommandListAndMaxMemoryFillPatternSizeForEachCommandQueueGroupWhenAppendingMemoryFillThenSuccessIsReturned', test_name, re.IGNORECASE)) or \
                    (re.search('SetAndGetAccessTypeForSharedAllocation', test_name, re.IGNORECASE)) or \
                    (re.search('GivenMultipleProcessesUsingMultipleSubDevicesThenKernelIsStressedAndExecuteSuccessfull', test_name, re.IGNORECASE)) or \
                    (re.search('GivenInOrderCommandListWhenAppendLaunchKernelInstructionCounterEventThenVerifyImmediateExecution', test_name, re.IGNORECASE)) or \
                    (re.search('GivenOutOfOrderRegularCommandListWhenAppendLaunchKernelInstructionCounterEventThenVerifyWaitForEvent', test_name, re.IGNORECASE)) or \
                    (re.search('GivenOutOfOrderImmediateCommandListWhenAppendLaunchKernelInstructionCounterEventThenVerifyWaitForEvent', test_name, re.IGNORECASE)) or \
                    (re.search('GivenInOrderMixedCommandListWhenAppendLaunchKernelInstructionCounterEventThenVerifyImmediateExecution', test_name, re.IGNORECASE)) or \
                    (re.search('zeCheckMiscFunctionsTestsInstantiate_zeCheckMiscFunctionsTests_GivenFunctionInKernelWhenLaunchingKernelsThenFunctionWorksCorrectly', test_name, re.IGNORECASE)) or \
                    re.search('fabric', test_name, re.IGNORECASE) or \
                    re.search('zeInitDrivers', test_name, re.IGNORECASE):
                test_feature_tag = "advanced"
            else:
                test_feature_tag = "basic"
        elif test_section == "Tools":
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
    elif test_binary == "test_sysman_frequency_zesinit":
        test_feature = "SysMan Frequency"    
    elif (re.search('test_init_sysman', test_binary, re.IGNORECASE)):
        test_feature = "SysMan Init"
    elif test_binary == "test_sysman_pci":
        test_feature = "SysMan PCIe"
    elif test_binary == "test_sysman_pci_zesinit":
        test_feature = "SysMan PCIe"    
    elif (re.search('power', test_name, re.IGNORECASE)):
        test_feature = "SysMan Power"
    elif (re.search('standby', test_name, re.IGNORECASE)):
        test_feature = "SysMan Standby"
    elif test_binary == "test_sysman_led":
        test_feature = "SysMan LEDs"
    elif test_binary == "test_sysman_led_zesinit":
        test_feature = "SysMan LEDs"    
    elif test_binary == "test_sysman_memory":
        test_feature = "SysMan Memory"
    elif test_binary == "test_sysman_memory_zesinit":
        test_feature = "SysMan Memory"    
    elif test_binary == "test_sysman_engine":
        test_feature = "SysMan Engines"
    elif test_binary == "test_sysman_engine_zesinit":
        test_feature = "SysMan Engines"    
    elif test_binary == "test_sysman_temperature":
        test_feature = "SysMan Temperature"
    elif test_binary == "test_sysman_temperature_zesinit":
        test_feature = "SysMan Temperature"
    elif test_binary == "test_sysman_psu":
        test_feature = "SysMan Power Supplies"
    elif test_binary == "test_sysman_psu_zesinit":
        test_feature = "SysMan Power Supplies"    
    elif test_binary == "test_sysman_fan":
        test_feature = "SysMan Fans"
    elif test_binary == "test_sysman_fan_zesinit":
        test_feature = "SysMan Fans"    
    elif test_binary == "test_sysman_ras":
        test_feature = "SysMan Reliability & Stability"
    elif test_binary == "test_sysman_ras_zesinit":
        test_feature = "SysMan Reliability & Stability"    
    elif test_binary == "test_sysman_fabric":
        test_feature = "SysMan Fabric"
    elif test_binary == "test_sysman_fabric_zesinit":
        test_feature = "SysMan Fabric"    
    elif test_binary == "test_sysman_diagnostics":
        test_feature = "SysMan Diagnostics"
    elif test_binary == "test_sysman_diagnostics_zesinit":
        test_feature = "SysMan Diagnostics"    
    elif test_binary == "test_sysman_device" and test_name.find("Reset") != -1:
        test_feature = "SysMan Device Reset"
    elif test_binary == "test_sysman_device_zesinit" and test_name.find("Reset") != -1:
        test_feature = "SysMan Device Reset"    
    elif test_binary == "test_sysman_device":
        test_feature = "SysMan Device Properties"
    elif test_binary == "test_sysman_device_zesinit":
        test_feature = "SysMan Device Properties"
    elif test_binary == "test_sysman_device_helper_zesinit":
        test_feature = "SysMan Device Properties"
    elif test_binary == "test_sysman_device_hierarchy_helper":
        test_feature = "SysMan Device Properties"
    elif test_binary == "test_sysman_device_hierarchy_helper_zesinit":
        test_feature = "SysMan Device Properties"    
    elif test_binary == "test_sysman_events":
        test_feature = "SysMan Events"
    elif test_binary == "test_sysman_events_zesinit":
        test_feature = "SysMan Events"    
    elif test_binary == "test_sysman_overclocking":
        test_feature = "SysMan Frequency"
    elif test_binary == "test_sysman_overclocking_zesinit":
        test_feature = "SysMan Frequency"    
    elif test_binary == "test_sysman_scheduler":
        test_feature = "SysMan Scheduler"
    elif test_binary == "test_sysman_scheduler_zesinit":
        test_feature = "SysMan Scheduler"    
    elif test_binary == "test_sysman_firmware":
        test_feature = "SysMan Firmware"
    elif test_binary == "test_sysman_firmware_zesinit":
        test_feature = "SysMan Firmware"    
    elif (re.search('performance', test_name, re.IGNORECASE)):
        test_feature = "SysMan Perf Profiles"
    elif test_binary == "test_metric":
        test_feature = "Metrics"
    elif (re.search('debug', test_binary, re.IGNORECASE)):
        test_feature = "Program Debug"
    elif test_binary == "test_sysman_ecc":
        test_feature = "SysMan ECC"
    elif test_binary == "test_sysman_ecc_zesinit":
        test_feature = "SysMan ECC"    
    if test_feature == "None":
        print("ERROR: test case " + test_name + " has no assigned feature\n", file=sys.stderr)
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
            test_section = "Tools"
            return test_feature, test_section
        if test_binary == "test_stress_atomics":
            test_feature = "Atomics"
        if test_binary == "test_stress_memory_allocation":
            test_feature = "Device Memory"
        if test_binary == "test_stress_commands_overloading":
            test_feature = "Events"
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
        if test_binary == "test_memory" and test_name.find("zeVirtualMemoryTests") != -1:
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
        if test_binary == "test_ipc" or test_binary == "test_ipc_memory" or test_binary == "test_ipc_put_handle":
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
        if test_binary == "test_fabric":
            test_feature = "Fabric"
        if test_binary == "test_mutable_cmdlist":
            test_feature = "Mutable Command List"
        if test_feature == "None":
            print("ERROR: test case " + test_name + " has no assigned feature\n",  file=sys.stderr)
            exit(-1)

        return test_feature, test_section

