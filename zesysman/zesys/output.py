#!/usr/bin/env python3
# Copyright (C) 2020 Intel Corporation
# SPDX-License-Identifier: MIT

#
# ZE string conversions
#

from . import state
from . import zes_wrap

def resultString(result):
    results = { zes_wrap.ZE_RESULT_SUCCESS : "SUCCESS",
                zes_wrap.ZE_RESULT_NOT_READY : "NOT_READY",
                zes_wrap.ZE_RESULT_ERROR_DEVICE_LOST : "ERROR_DEVICE_LOST",
                zes_wrap.ZE_RESULT_ERROR_OUT_OF_HOST_MEMORY : "ERROR_OUT_OF_HOST_MEMORY",
                zes_wrap.ZE_RESULT_ERROR_OUT_OF_DEVICE_MEMORY : "ERROR_OUT_OF_DEVICE_MEMORY",
                zes_wrap.ZE_RESULT_ERROR_MODULE_BUILD_FAILURE : "ERROR_MODULE_BUILD_FAILURE",
                zes_wrap.ZE_RESULT_ERROR_MODULE_LINK_FAILURE : "ERROR_LINK_FAILURE",
                zes_wrap.ZE_RESULT_ERROR_INSUFFICIENT_PERMISSIONS : "ERROR_INSUFFICIENT_PERMISSIONS",
                zes_wrap.ZE_RESULT_ERROR_NOT_AVAILABLE : "ERROR_NOT_AVAILABLE",
                zes_wrap.ZE_RESULT_ERROR_DEPENDENCY_UNAVAILABLE : "ERROR_DEPENDENCY_UNAVAILABLE",
                zes_wrap.ZE_RESULT_ERROR_UNINITIALIZED : "ERROR_UNINITIALIZED",
                zes_wrap.ZE_RESULT_ERROR_UNSUPPORTED_VERSION : "ERROR_UNSUPPORTED_VERSION",
                zes_wrap.ZE_RESULT_ERROR_UNSUPPORTED_FEATURE : "ERROR_UNSUPPORTED_FEATURE",
                zes_wrap.ZE_RESULT_ERROR_INVALID_ARGUMENT : "ERROR_INVALID_ARGUMENT",
                zes_wrap.ZE_RESULT_ERROR_INVALID_NULL_HANDLE : "ERROR_INVALID_NULL_HANDLE",
                zes_wrap.ZE_RESULT_ERROR_HANDLE_OBJECT_IN_USE : "ERROR_HANDLE_OBJECT_IN_USE",
                zes_wrap.ZE_RESULT_ERROR_INVALID_NULL_POINTER : "ERROR_INVALID_NULL_POINTER",
                zes_wrap.ZE_RESULT_ERROR_INVALID_SIZE : "ERROR_INVALID_SIZE",
                zes_wrap.ZE_RESULT_ERROR_UNSUPPORTED_SIZE : "ERROR_UNSUPPORTED_SIZE",
                zes_wrap.ZE_RESULT_ERROR_UNSUPPORTED_ALIGNMENT : "ERROR_UNSUPPORTED_ALIGNMENT",
                zes_wrap.ZE_RESULT_ERROR_INVALID_SYNCHRONIZATION_OBJECT : "ERROR_INVALID_SYNCHRONIZATION_OBJECT",
                zes_wrap.ZE_RESULT_ERROR_INVALID_ENUMERATION : "ERROR_INVALID_ENUMERATION",
                zes_wrap.ZE_RESULT_ERROR_UNSUPPORTED_ENUMERATION : "ERROR_UNSUPPORTED_ENUMERATION",
                zes_wrap.ZE_RESULT_ERROR_UNSUPPORTED_IMAGE_FORMAT : "ERROR_UNSUPPORTED_IMAGE_FORMAT",
                zes_wrap.ZE_RESULT_ERROR_INVALID_NATIVE_BINARY : "ERROR_INVALID_NATIVE_BINARY",
                zes_wrap.ZE_RESULT_ERROR_INVALID_GLOBAL_NAME : "ERROR_INVALID_GLOBAL_NAME",
                zes_wrap.ZE_RESULT_ERROR_INVALID_KERNEL_NAME : "ERROR_INVALID_KERNEL_NAME",
                zes_wrap.ZE_RESULT_ERROR_INVALID_FUNCTION_NAME : "ERROR_INVALID_FUNCTION_NAME",
                zes_wrap.ZE_RESULT_ERROR_INVALID_GROUP_SIZE_DIMENSION : "ERROR_INVALID_GROUP_SIZE_DIMENSION",
                zes_wrap.ZE_RESULT_ERROR_INVALID_GLOBAL_WIDTH_DIMENSION : "ERROR_INVALID_GLOBAL_WIDTH_DIMENSION",
                zes_wrap.ZE_RESULT_ERROR_INVALID_KERNEL_ARGUMENT_INDEX : "ERROR_INVALID_KERNEL_ARGUMENT_INDEX",
                zes_wrap.ZE_RESULT_ERROR_INVALID_KERNEL_ARGUMENT_SIZE : "ERROR_INVALID_KERNEL_ARGUMENT_SIZE",
                zes_wrap.ZE_RESULT_ERROR_INVALID_KERNEL_ATTRIBUTE_VALUE : "ERROR_INVALID_KERNEL_ATTRIBUTE_VALUE",
                zes_wrap.ZE_RESULT_ERROR_INVALID_MODULE_UNLINKED : "ERROR_MODULE_MUST_BE_UNLINKED",
                zes_wrap.ZE_RESULT_ERROR_INVALID_COMMAND_LIST_TYPE : "ERROR_INVALID_COMMAND_LIST_TYPE",
                zes_wrap.ZE_RESULT_ERROR_OVERLAPPING_REGIONS : "ERROR_OVERLAPPING_REGIONS",
                zes_wrap.ZE_RESULT_ERROR_UNKNOWN : "ERROR_UNKNOWN" }
    return results.get(result, "UNKNOWN")

def fmtknown(value, fmt="%.1f", unknown="?"):
    if value < 0:
        return unknown
    else:
        return value

def knownvalue(value, unknown="?"):
    if value < 0:
        return unknown
    else:
        return value

def ipcPropertiesString(ipcProps):
    if not ipcProps:
        return "None"

    ipcProperties = [(zes_wrap.ZE_IPC_PROPERTY_FLAG_MEMORY, "Memory allocations"),
                     (zes_wrap.ZE_IPC_PROPERTY_FLAG_EVENT_POOL, "Events")]

    propStrings = []
    for p, s in ipcProperties:
        if ipcProps & p:
            ipcProps ^= p
            propStrings.append(s)
    if ipcProps != 0:
        propStrings.append("Unknown property")

    return ", ".join(propStrings)

def versionString(version):
    return str(version >> 16) + "." + str(version & 0xffff)

def deviceTypeString(devType):
    devTypes = { zes_wrap.ZE_DEVICE_TYPE_GPU : "GPU",
                 zes_wrap.ZE_DEVICE_TYPE_FPGA : "FPGA" }
    return devTypes.get(devType, "Unknown")

def resetReasonsString(reasons):
    if not reasons:
        return "Not required"

    resetReasons = [(zes_wrap.zes_wrap.ZES_RESET_REASON_FLAG_WEDGED, "Hardware is wedged"),
                    (zes_wrap.zes_wrap.ZES_RESET_REASON_FLAG_REPAIR, "Required for in-field repairs")]
    reasonStrings = []
    for r, s in reasonReasons:
        if reasons & r:
            reasons ^= r
            reasonStrings.append(s)
    if reasons != 0:
        reasonStrings.append("Unknown reset reason")

    return ", ".join(reasonStrings)

def repairStatusString(repairStatus):
    repairStatuses = { zes_wrap.zes_wrap.ZES_REPAIR_STATUS_UNSUPPORTED : "Unsupported",
                       zes_wrap.zes_wrap.ZES_REPAIR_STATUS_NOT_PERFORMED : "Unrepaired",
                       zes_wrap.zes_wrap.ZES_REPAIR_STATUS_PERFORMED : "Repaired" }
    return repairStatuses.get(repairStatus, "Unknown")

def tempTypeString(tempType):
    tempTypes = { zes_wrap.ZES_TEMP_SENSORS_GLOBAL : "Overall",
                  zes_wrap.ZES_TEMP_SENSORS_GPU : "GPU",
                  zes_wrap.ZES_TEMP_SENSORS_MEMORY : "Memory",
                  zes_wrap.ZES_TEMP_SENSORS_GLOBAL_MIN : "Overall_Min",
                  zes_wrap.ZES_TEMP_SENSORS_GPU_MIN : "GPU_Min",
                  zes_wrap.ZES_TEMP_SENSORS_MEMORY_MIN : "Memory_Min" }
    return tempTypes.get(tempType, "Unknown")

def freqTypeString(freqType):
    freqTypes = { zes_wrap.ZES_FREQ_DOMAIN_GPU : "GPU",
                  zes_wrap.ZES_FREQ_DOMAIN_MEMORY : "Memory" }
    return freqTypes.get(freqType, "Unknown")

def ocModeString(mode):
    ocModes = { zes_wrap.ZES_OC_MODE_OFF : "Off",
                zes_wrap.ZES_OC_MODE_OVERRIDE : "Override",
                zes_wrap.ZES_OC_MODE_INTERPOLATIVE : "Interpolative",
                zes_wrap.ZES_OC_MODE_FIXED : "Fixed" }
    return ocModes.get(mode, "Unknown")

def engTypeString(engType):
    engTypes = { zes_wrap.ZES_ENGINE_GROUP_ALL : "AllEngines",
                 zes_wrap.ZES_ENGINE_GROUP_COMPUTE_ALL : "AllComputeEngines",
                 zes_wrap.ZES_ENGINE_GROUP_MEDIA_ALL : "AllMediaEngines",
                 zes_wrap.ZES_ENGINE_GROUP_COPY_ALL : "AllCopyEngines",
                 zes_wrap.ZES_ENGINE_GROUP_COMPUTE_SINGLE : "ComputeEngine",
                 zes_wrap.ZES_ENGINE_GROUP_RENDER_SINGLE : "RenderEngine",
                 zes_wrap.ZES_ENGINE_GROUP_MEDIA_DECODE_SINGLE : "MediaDecodeEngine",
                 zes_wrap.ZES_ENGINE_GROUP_MEDIA_ENCODE_SINGLE : "MediaEncodeEngine",
                 zes_wrap.ZES_ENGINE_GROUP_COPY_SINGLE : "CopyEngine" }
    return engTypes.get(engType, "Unknown")

def memTypeString(memType):
    memTypes = { zes_wrap.ZES_MEM_TYPE_HBM : "HBM",
                 zes_wrap.ZES_MEM_TYPE_DDR : "DDR",
                 zes_wrap.ZES_MEM_TYPE_DDR3 : "DDR3",
                 zes_wrap.ZES_MEM_TYPE_DDR4 : "DDR4",
                 zes_wrap.ZES_MEM_TYPE_DDR5 : "DDR5",
                 zes_wrap.ZES_MEM_TYPE_LPDDR : "LPDDR",
                 zes_wrap.ZES_MEM_TYPE_LPDDR3 : "LPDDR3",
                 zes_wrap.ZES_MEM_TYPE_LPDDR4 : "LPDDR4",
                 zes_wrap.ZES_MEM_TYPE_LPDDR5 : "LPDDR5",
                 zes_wrap.ZES_MEM_TYPE_SRAM : "SRAM",
                 zes_wrap.ZES_MEM_TYPE_L1 : "L1",
                 zes_wrap.ZES_MEM_TYPE_L3 : "L3",
                 zes_wrap.ZES_MEM_TYPE_GRF : "GRF",
                 zes_wrap.ZES_MEM_TYPE_SLM : "SLM" }
    return memTypes.get(memType, "Unknown")

def memLocString(memLoc):
    memLocs = { zes_wrap.ZES_MEM_LOC_SYSTEM : "System",
                zes_wrap.ZES_MEM_LOC_DEVICE : "Device" }
    return memLocs.get(memLoc, "Unknown")

def barTypeString(barType):
    barTypes = { zes_wrap.ZES_PCI_BAR_TYPE_MMIO : "MMIO",
                 zes_wrap.ZES_PCI_BAR_TYPE_ROM : "ROM",
                 zes_wrap.ZES_PCI_BAR_TYPE_MEM : "MEM" }
    return barTypes.get(barType, "Unknown")

def memHealthString(memHealth):
    memHealths = { zes_wrap.ZES_MEM_HEALTH_OK : "Good",
                   zes_wrap.ZES_MEM_HEALTH_DEGRADED : "Degraded",
                   zes_wrap.ZES_MEM_HEALTH_CRITICAL : "Critical",
                   zes_wrap.ZES_MEM_HEALTH_REPLACE : "Replace" }
    return memHealths.get(memHealth, "Unknown")

def throttleReasonsString(reasons):
    if not reasons:
        return "Not throttled"

    throttleReasons = [(zes_wrap.ZES_FREQ_THROTTLE_REASON_FLAG_AVE_PWR_CAP, "Exceeded sustained power limit"),
                       (zes_wrap.ZES_FREQ_THROTTLE_REASON_FLAG_BURST_PWR_CAP, "Exceeded burst power limit"),
                       (zes_wrap.ZES_FREQ_THROTTLE_REASON_FLAG_CURRENT_LIMIT, "Exceeded electrical limits"),
                       (zes_wrap.ZES_FREQ_THROTTLE_REASON_FLAG_THERMAL_LIMIT, "Exceeded temperature limit"),
                       (zes_wrap.ZES_FREQ_THROTTLE_REASON_FLAG_PSU_ALERT, "Exceeded power supply limits"),
                       (zes_wrap.ZES_FREQ_THROTTLE_REASON_FLAG_SW_RANGE, "Request outside software limits"),
                       (zes_wrap.ZES_FREQ_THROTTLE_REASON_FLAG_HW_RANGE, "Request outside hardware limits")]
    reasonStrings = []
    for r, s in throttleReasons:
        if reasons & r:
            reasons ^= r
            reasonStrings.append(s)
    if reasons != 0:
        reasonStrings.append("Unknown throttle reason")

    return ", ".join(reasonStrings)

def pciLinkStatusString(pciLinkStatus):
    pciLinkStatuses = { zes_wrap.ZES_PCI_LINK_STATUS_GOOD : "Good",
                        zes_wrap.ZES_PCI_LINK_STATUS_UNKNOWN : "Unknown",
                        zes_wrap.ZES_PCI_LINK_STATUS_QUALITY_ISSUES : "Degraded",
                        zes_wrap.ZES_PCI_LINK_STATUS_STABILITY_ISSUES : "Unstable" }
    return pciLinkStatuses.get(pciLinkStatus, "Unknown")

def pciQualityIssuesString(reasons):
    if not reasons:
        return "None"

    qualityIssues = [(zes_wrap.ZES_PCI_LINK_QUAL_ISSUE_FLAG_REPLAYS, "Excessive packet replays"),
                     (zes_wrap.ZES_PCI_LINK_QUAL_ISSUE_FLAG_SPEED, "Reduced bitrate")]
    reasonStrings = []
    for r, s in qualityIssues:
        if reasons & r:
            reasons ^= r
            reasonStrings.append(s)
    if reasons != 0:
        reasonStrings.append("Unknown degradation reason")

    return ", ".join(reasonStrings)

def pciStabilityIssuesString(reasons):
    if not reasons:
        return "None"

    stabilityIssues = [(zes_wrap.ZES_PCI_LINK_STAB_ISSUE_FLAG_RETRAINING, "Link retraining occurred")]
    reasonStrings = []
    for r, s in stabilityIssues:
        if reasons & r:
            reasons ^= r
            reasonStrings.append(s)
    if reasons != 0:
        reasonStrings.append("Unknown stability issue")

    return ", ".join(reasonStrings)

def portStatusString(portStatus):
    portStatuses = { zes_wrap.ZES_FABRIC_PORT_STATUS_UNKNOWN : "Unknown",
                     zes_wrap.ZES_FABRIC_PORT_STATUS_HEALTHY : "Good",
                     zes_wrap.ZES_FABRIC_PORT_STATUS_DEGRADED : "Degraded",
                     zes_wrap.ZES_FABRIC_PORT_STATUS_FAILED : "Failed",
                     zes_wrap.ZES_FABRIC_PORT_STATUS_DISABLED : "Off" }
    return portStatuses.get(portStatus, "Unknown")

def portQualityIssuesString(reasons):
    if not reasons:
        return "None"

    qualityIssues = [(zes_wrap.ZES_FABRIC_PORT_QUAL_ISSUE_FLAG_LINK_ERRORS, "Excessive link errors"),
                     (zes_wrap.ZES_FABRIC_PORT_QUAL_ISSUE_FLAG_SPEED, "Reduced bitrate/width")]
    reasonStrings = []
    for r, s in qualityIssues:
        if reasons & r:
            reasons ^= r
            reasonStrings.append(s)
    if reasons != 0:
        reasonStrings.append("Unknown degradation reason")

    return ", ".join(reasonStrings)

def portFailureReasonsString(reasons):
    if not reasons:
        return "None"

    failureReasons = [(zes_wrap.ZES_FABRIC_PORT_FAILURE_FLAG_FAILED, "Port failure"),
                      (zes_wrap.ZES_FABRIC_PORT_FAILURE_FLAG_TRAINING_TIMEOUT, "Training timeout"),
                      (zes_wrap.ZES_FABRIC_PORT_FAILURE_FLAG_FLAPPING, "Flapping")]
    reasonStrings = []
    for r, s in failureReasons:
        if reasons & r:
            reasons ^= r
            reasonStrings.append(s)
    if reasons != 0:
        reasonStrings.append("Unknown failure")

    return ", ".join(reasonStrings)

def rasTypeString(rasType):
    rasTypes = { zes_wrap.ZES_RAS_ERROR_TYPE_CORRECTABLE : "Correctable",
                 zes_wrap.ZES_RAS_ERROR_TYPE_UNCORRECTABLE : "Uncorrectable" }
    return rasTypes.get(rasType, "Unknown")

def stbyTypeString(stbyType):
    stbyTypes = { zes_wrap.ZES_STANDBY_TYPE_GLOBAL : "Global" }
    return stbyTypes.get(stbyType, "Unknown")

def stbyPromoModeString(stbyPromoMode):
    stbyPromoModes = { zes_wrap.ZES_STANDBY_PROMO_MODE_DEFAULT : "Enabled",
                       zes_wrap.ZES_STANDBY_PROMO_MODE_NEVER : "Disabled" }
    return stbyPromoModes.get(stbyPromoMode, "Unknown")

def enginesUsedString(engines):
    if not engines:
        return "None"

    engineTypes = [(1 << zes_wrap.ZES_ENGINE_TYPE_FLAG_OTHER, "Other"),
                   (1 << zes_wrap.ZES_ENGINE_TYPE_FLAG_COMPUTE, "Compute"),
                   (1 << zes_wrap.ZES_ENGINE_TYPE_FLAG_3D, "3D"),
                   (1 << zes_wrap.ZES_ENGINE_TYPE_FLAG_MEDIA, "Media"),
                   (1 << zes_wrap.ZES_ENGINE_TYPE_FLAG_DMA, "DMA")]

    engineStrings = []
    for t, s in engineTypes:
        if engines & t:
            engines ^= t
            engineStrings.append(s)

    if engines != 0:
        engineStrings.append("Unknown engine")

    return ", ".join(engineStrings)

def schedModeString(schedMode):
    schedModes = { zes_wrap.ZES_SCHED_MODE_TIMEOUT : "Timeout",
                   zes_wrap.ZES_SCHED_MODE_TIMESLICE : "Timeslice",
                   zes_wrap.ZES_SCHED_MODE_EXCLUSIVE : "Exclusive",
                   zes_wrap.ZES_SCHED_MODE_COMPUTE_UNIT_DEBUG : "Debug" }
    return schedModes.get(schedMode, "Unknown")

def schedSupportedModesString(schedMode):
    schedModes = [zes_wrap.ZES_SCHED_MODE_TIMEOUT, zes_wrap.ZES_SCHED_MODE_TIMESLICE,
                  zes_wrap.ZES_SCHED_MODE_EXCLUSIVE, zes_wrap.ZES_SCHED_MODE_COMPUTE_UNIT_DEBUG]

    modesStrings = []
    for m in schedModes:
        if schedMode & (1 << m):
            schedMode ^= (1 << m)
            modesStrings.append(schedModeString(m))

    if schedMode != 0:
        modesStrings.append("Unknown mode")

    return ", ".join(modesStrings)

def diagResultString(diagResult):
    diagResults = { zes_wrap.ZES_DIAG_RESULT_NO_ERRORS : "Pass",
                    zes_wrap.ZES_DIAG_RESULT_ABORT : "Diagnostic aborted",
                    zes_wrap.ZES_DIAG_RESULT_FAIL_CANT_REPAIR : "Fail, irreparable",
                    zes_wrap.ZES_DIAG_RESULT_REBOOT_FOR_REPAIR : "Fail, reboot required" }
    return diagResults.get(diagResult, "Unknown")

def eventsString(events):
    if not events:
        return "None"

    eventCodes = [ (zes_wrap.ZES_EVENT_TYPE_FLAG_DEVICE_DETACH, "Detach"),
                   (zes_wrap.ZES_EVENT_TYPE_FLAG_DEVICE_ATTACH, "Reattach"),
                   (zes_wrap.ZES_EVENT_TYPE_FLAG_DEVICE_SLEEP_STATE_ENTER, "Sleeping"),
                   (zes_wrap.ZES_EVENT_TYPE_FLAG_DEVICE_SLEEP_STATE_EXIT, "Waking"),
                   (zes_wrap.ZES_EVENT_TYPE_FLAG_FREQ_THROTTLED, "Frequency throttled"),
                   (zes_wrap.ZES_EVENT_TYPE_FLAG_ENERGY_THRESHOLD_CROSSED, "Energy threshold"),
                   (zes_wrap.ZES_EVENT_TYPE_FLAG_TEMP_CRITICAL, "Temperature critical"),
                   (zes_wrap.ZES_EVENT_TYPE_FLAG_TEMP_THRESHOLD1, "Temperature threshold 1"),
                   (zes_wrap.ZES_EVENT_TYPE_FLAG_TEMP_THRESHOLD2, "Temperature threshold 2"),
                   (zes_wrap.ZES_EVENT_TYPE_FLAG_MEM_HEALTH, "Memory health change"),
                   (zes_wrap.ZES_EVENT_TYPE_FLAG_FABRIC_PORT_HEALTH, "Port health change"),
                   (zes_wrap.ZES_EVENT_TYPE_FLAG_PCI_LINK_HEALTH, "PCI health change"),
                   (zes_wrap.ZES_EVENT_TYPE_FLAG_RAS_CORRECTABLE_ERRORS, "Correctable error threshold"),
                   (zes_wrap.ZES_EVENT_TYPE_FLAG_RAS_UNCORRECTABLE_ERRORS, "Uncorrectable error threshold"),
                   (zes_wrap.ZES_EVENT_TYPE_FLAG_DEVICE_RESET_REQUIRED, "Reset required") ]

    eventStrings = []
    for t, s in eventCodes:
        if events & t:
            events ^= t
            eventStrings.append(s)

    if events != 0:
        eventStrings.append("Unknown event")

    return ", ".join(eventStrings)

def fanModeString(mode):
    fanModes = { zes_wrap.ZES_FAN_SPEED_MODE_DEFAULT:  "Default",
                 zes_wrap.ZES_FAN_SPEED_MODE_FIXED : "Fixed",
                 zes_wrap.ZES_FAN_SPEED_MODE_TABLE : "Table" }
    return fanModes.get(mode, "Unknown")

def supportedFanModesString(modes):
    if not modes:
        return "None"

    allModes = [ zes_wrap.ZES_FAN_SPEED_MODE_DEFAULT, zes_wrap.ZES_FAN_SPEED_MODE_FIXED, zes_wrap.ZES_FAN_SPEED_MODE_TABLE ]

    modeStrings = []
    for m in allModes:
        if modes & (1 << m):
            modes ^= (1 << m)
            modeStrings.append(fanModeString(m))

    if modes != 0:
        modeStrings.append("Unknown")

    return ", ".join(modeStrings)

def fanUnitString(unit):
    fanUnits = { zes_wrap.ZES_FAN_SPEED_UNITS_RPM : "RPM",
                 zes_wrap.ZES_FAN_SPEED_UNITS_PERCENT : "%" }
    return fanUnits.get(unit, "Unknown")

def supportedFanUnitsString(units):
    if not units:
        return "None"

    allUnits = [ zes_wrap.ZES_FAN_SPEED_UNITS_RPM, zes_wrap.ZES_FAN_SPEED_UNITS_PERCENT ]

    unitStrings = []
    for u in allUnits:
        if units & (1 << u):
            units ^= (1 << u)
            unitStrings.append(fanUnitString(u))

    if units != 0:
        unitStrings.append("Unknown")

    return ", ".join(unitStrings)

def psuVoltageStatusString(status):
    psuVoltageStatuses = { zes_wrap.ZES_PSU_VOLTAGE_STATUS_NORMAL : "Normal",
                           zes_wrap.ZES_PSU_VOLTAGE_STATUS_OVER : "Overvoltage",
                           zes_wrap.ZES_PSU_VOLTAGE_STATUS_UNDER : "Undervoltage" }
    return psuVoltageStatuses.get(status, "Unknown")

def c6(fraction):
    colorTable = (0, 11, 16, 21, 26, 31)
    refVal = fraction * 32
    return min(range(6), key=lambda x:abs(colorTable[x] - refVal))

def colorCodeAnsi256(red, green, blue):
    return 16 + 36 * c6(red) + 6 * c6(green) + c6(blue)

def colorBlockAnsi256(color):
    return "\x1b[0m[\x1b[48;5;%dm \x1b[0m]" % color

def onOffString(on):
    if on:
        return "On"
    else:
        return "Off"

def colorString(colors):
    s = "%.2f / %.2f / %.2f" % (colors.red, colors.green, colors.blue)
    if state.addAnsi256ColorBlock:
        s += " " + colorBlockAnsi256(colorCodeAnsi256(colors.red, colors.green, colors.blue))
    return s

def fullUUID(uuid):
    return uuid

def tinyUUID(uuid):
    return uuid[:2] + ".." + uuid[-2:]

deviceUUID = fullUUID
