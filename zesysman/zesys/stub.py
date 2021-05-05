#!/usr/bin/env python3
# Copyright (C) 2020 Intel Corporation
# SPDX-License-Identifier: MIT

from . import logger
from . import state
from . import util
from zesys.zes_wrap import *
from zesys.types import *

def api():
    #
    # Default API stubs
    #

    # For stubs to retain state
    state.stub = {}

    # Stubs to replace API functions
    def zesDeviceEnumTemperatureSensors(dev, count_ptr, handle_array):
        if handle_array is None:
            uint32_assign(count_ptr, 6)
        elif uint32_value(count_ptr) == 6:
            items = zes_temp_handle_array.frompointer(handle_array)
            items[0] = ulong_to_temp_handle(ZES_TEMP_SENSORS_GLOBAL)
            items[1] = ulong_to_temp_handle(ZES_TEMP_SENSORS_GPU)
            items[2] = ulong_to_temp_handle(ZES_TEMP_SENSORS_MEMORY)
            items[3] = ulong_to_temp_handle(ZES_TEMP_SENSORS_GLOBAL_MIN)
            items[4] = ulong_to_temp_handle(ZES_TEMP_SENSORS_GPU_MIN)
            items[5] = ulong_to_temp_handle(ZES_TEMP_SENSORS_MEMORY_MIN)
        else:
            raise ValueError(uint32_value(count_ptr))
    def zesTemperatureGetProperties(temp, tempProps):
        t = temp_handle_to_ulong(temp)
        tempProps.type = t
        tempProps.onSubdevice = False
        tempProps.subdeviceId = 0
        tempProps.maxTemperature = 90.0 - 20.0 * t
        tempProps.isCriticalTempSupported = t < ZES_TEMP_SENSORS_GLOBAL_MIN
        tempProps.isThreshold1Supported = t != ZES_TEMP_SENSORS_MEMORY
        tempProps.isThreshold2Supported = t not in [ZES_TEMP_SENSORS_MEMORY, ZES_TEMP_SENSORS_MEMORY_MIN]
    def zesTemperatureGetConfig(temp, tempCfg):
        t = temp_handle_to_ulong(temp)
        temps = state.stub.setdefault("TempConfig", {})
        cfg = temps.get(t, ( False, ( False, False, 50.0 ), ( False, False, 50.0 ) ))
        tempCfg.enableCritical, t1cfg, t2cfg = cfg
        t1 = tempCfg.threshold1
        t1.enableLowToHigh, t1.enableHighToLow, t1.threshold = t1cfg
        tempCfg.threshold1 = t1
        t2 = tempCfg.threshold2
        t2.enableLowToHigh, t2.enableHighToLow, t2.threshold = t2cfg
        tempCfg.threshold2 = t2
    def zesTemperatureSetConfig(temp, tempCfg):
        t = temp_handle_to_ulong(temp)
        temps = state.stub.setdefault("TempConfig", {})
        temps[t] = ( tempCfg.enableCritical,
                     ( tempCfg.threshold1.enableLowToHigh, tempCfg.threshold1.enableHighToLow,
                       tempCfg.threshold1.threshold),
                     ( tempCfg.threshold2.enableLowToHigh, tempCfg.threshold2.enableHighToLow,
                       tempCfg.threshold2.threshold) )
    def zesTemperatureGetState(temp):
        t = state.stub.get("Temp", 50.0)
        state.stub["Temp"] = t + 0.1
        return t
    def zesDeviceEnumPowerDomains(dev, count_ptr, handle_array):
        if handle_array is None:
            uint32_assign(count_ptr, 1)
        elif uint32_value(count_ptr) == 1:
            p = state.stub.setdefault("Pwr", {})
            p["sustained"] = { "enabled" : True, "power" : 100000, "interval" : 1000 }
            p["burst"] = { "enabled" : True, "power" : 150000 }
            p["peak"] = { "powerAC" : 120000, "powerDC" : 90000 }
            p["counter"] = { "energy" : 0, "timestamp" : 0 }
            p["threshold"] = { "threshold" : 0.0, "process" : 0xffffffff }
            items = zes_pwr_handle_array.frompointer(handle_array)
            items[0] = ulong_to_pwr_handle(0)
        else:
            raise ValueError(uint32_value(count_ptr))
    def zesPowerGetProperties(pwr, pwrProps):
        pwrProps.onSubdevice = False
        pwrProps.canControl = True
        pwrProps.isEnergyThresholdSupported = True
        pwrProps.defaultLimit = -1
        pwrProps.minLimit = 20
        pwrProps.maxLimit = 200000
    def zesPowerSetLimits(pwr, sustainedLimit, burstLimit, peakLimit):
        p = state.stub["Pwr"]
        if sustainedLimit is not None:
            p["sustained"]["enabled"] = sustainedLimit.enabled
            p["sustained"]["power"] = sustainedLimit.power
            p["sustained"]["interval"] = sustainedLimit.interval
        if burstLimit is not None:
            p["burst"]["enabled"] = burstLimit.enabled
            p["burst"]["power"] = burstLimit.power
        if peakLimit is not None:
            p["peak"]["powerAC"] = peakLimit.powerAC
            p["peak"]["powerDC"] = peakLimit.powerDC
    def zesPowerGetLimits(pwr, sustainedLimit, burstLimit, peakLimit):
        p = state.stub["Pwr"]
        if sustainedLimit is not None:
            sustainedLimit.enabled = p["sustained"]["enabled"]
            sustainedLimit.power = p["sustained"]["power"]
            sustainedLimit.interval = p["sustained"]["interval"]
        if burstLimit is not None:
            burstLimit.enabled = p["burst"]["enabled"]
            burstLimit.power = p["burst"]["power"]
        if peakLimit is not None:
            peakLimit.powerAC = p["peak"]["powerAC"]
            peakLimit.powerDC = p["peak"]["powerDC"]
    def zesPowerGetEnergyCounter(pwr, pwrCounter):
        p = state.stub["Pwr"]
        p["counter"]["energy"] += 100000000
        p["counter"]["timestamp"] += 1000000
        pwrCounter.energy = p["counter"]["energy"]
        pwrCounter.timestamp = p["counter"]["timestamp"]
    def zesPowerGetEnergyThreshold(pwr, pwrThreshold):
        p = state.stub["Pwr"]
        pwrThreshold.enable = p["threshold"]["threshold"] != 0.0
        pwrThreshold.threshold = p["threshold"]["threshold"]
        pwrThreshold.processId = p["threshold"]["process"]
    def zesPowerSetEnergyThreshold(pwr, threshold):
        p = state.stub["Pwr"]
        p["threshold"]["threshold"] = threshold
        p["threshold"]["process"] = os.getpid()
    def zesDeviceEnumFrequencyDomains(dev, count_ptr, handle_array):
        if handle_array is None:
            uint32_assign(count_ptr, 2)
        elif uint32_value(count_ptr) == 2:
            p = state.stub.setdefault("Freq", {})
            p[ZES_FREQ_DOMAIN_GPU] = { "Range" : (200.0, 1200.0),
                                       "State" : 300.0,
                                       "Throttled" : [0, 0],
                                       "ocFreq" : 1200.0,
                                       "ocVolt" : (3.0, 0.0),
                                       "ocMode" : ZES_OC_MODE_OFF,
                                       "iccMax" : 15.0,
                                       "tjMax" : 150.0 }
            p[ZES_FREQ_DOMAIN_MEMORY] = { "Range" : (200.0, 1200.0),
                                          "State" : 300.0,
                                          "Throttled" : [0, 0] }
            items = zes_freq_handle_array.frompointer(handle_array)
            items[0] = ulong_to_freq_handle(ZES_FREQ_DOMAIN_GPU)
            items[1] = ulong_to_freq_handle(ZES_FREQ_DOMAIN_MEMORY)
        else:
            raise ValueError(uint32_value(count_ptr))
    def zesFrequencyGetProperties(freq, freqProp):
        freqProp.type = freq_handle_to_ulong(freq)
        freqProp.onSubdevice = False
        freqProp.subdeviceId = 0
        freqProp.canControl = True
        freqProp.isThrottleEventSupported = False
        freqProp.min = 200.0
        freqProp.max = 1200.0
    def zesFrequencyGetAvailableClocks(freq, count_ptr, freq_array):
        freqProps = zes_typed_structure(ZES_STRUCTURE_TYPE_FREQ_PROPERTIES)
        # zeCall(zesFrequencyGetProperties(freq, freqProps))
        zesFrequencyGetProperties(freq, freqProps)
        freqs = []
        f = freqProps.min
        while f <= freqProps.max:
            freqs.append(f)
            f += 50.0
        if freq_array is None:
            uint32_assign(count_ptr, len(freqs))
        elif uint32_value(count_ptr) == len(freqs):
            results = double_array.frompointer(freq_array)
            for i in range(len(freqs)):
                results[i] = freqs[i]
        else:
            raise ValueError(uint32_value(count_ptr))
    def zesFrequencyGetRange(freq, freqLimits):
        f = freq_handle_to_ulong(freq)
        freqLimits.min, freqLimits.max = state.stub["Freq"][f]["Range"]
    def zesFrequencySetRange(freq, freqLimits):
        f = freq_handle_to_ulong(freq)
        minLimit = util.saturate(freqLimits.min, 200.0, 1200.0)
        maxLimit = util.saturate(freqLimits.max, 200.0, 1200.0)
        state.stub["Freq"][f]["Range"] = minLimit, maxLimit
    def zesFrequencyGetState(freq, freqState):
        f = freq_handle_to_ulong(freq)
        freqState.currentVoltage = 3.0
        freqState.request = state.stub["Freq"][f]["State"]
        freqState.tdp = 1200.0
        freqState.efficient = 1000.0
        freqState.actual = state.stub["Freq"][f]["State"]
        freqState.throttleReasons = 0
    def zesFrequencyGetThrottleTime(freq, freqThrottle):
        f = freq_handle_to_ulong(freq)
        freqThrottle.throttleTime, freqThrottle.timestamp = state.stub["Freq"][f]["Throttled"]
        state.stub["Freq"][f]["Throttled"][0] += 100000
        state.stub["Freq"][f]["Throttled"][1] += 1000000
    def zesFrequencyOcGetCapabilities(freq, ocCapabilities):
        f = freq_handle_to_ulong(freq)
        if f == 0:
            ocCapabilities.isOcSupported = True
            ocCapabilities.maxFactoryDefaultFrequency = 300.0
            ocCapabilities.maxFactoryDefaultVoltage = 3.0
            ocCapabilities.maxOcFrequency = 2000
            ocCapabilities.minOcVoltageOffset = 0.5
            ocCapabilities.maxOcVoltageOffset = 1.5
            ocCapabilities.maxOcVoltage = 5.0
            ocCapabilities.isTjMaxSupported = True
            ocCapabilities.isIccMaxSupported = True
            ocCapabilities.isHighVoltModeCapable = True
            ocCapabilities.isHighVoltModeEnabled = False
            ocCapabilities.isExtendedModeSupported = True
            ocCapabilities.isFixedModeSupported = True
        elif f == 1:
            ocCapabilities.isOcSupported = False
            ocCapabilities.maxFactoryDefaultFrequency = 300.0
            ocCapabilities.maxFactoryDefaultVoltage = 3.0
            ocCapabilities.maxOcFrequency = 1200
            ocCapabilities.minOcVoltageOffset = 0.0
            ocCapabilities.maxOcVoltageOffset = 0.0
            ocCapabilities.maxOcVoltage = 3.0
            ocCapabilities.isTjMaxSupported = False
            ocCapabilities.isIccMaxSupported = False
            ocCapabilities.isHighVoltModeCapable = False
            ocCapabilities.isHighVoltModeEnabled = False
            ocCapabilities.isExtendedModeSupported = False
            ocCapabilities.isFixedModeSupported = False
        else:
            raise ValueError(f)
    def zesFrequencyOcGetFrequencyTarget(freq):
        f = freq_handle_to_ulong(freq)
        if f == 0:
            return state.stub["Freq"][f]["ocFreq"]
        else:
            raise ValueError(f)
    def zesFrequencyOcSetFrequencyTarget(freq, ocFTarget):
        f = freq_handle_to_ulong(freq)
        if f == 0:
            state.stub["Freq"][f]["ocFreq"] = ocFTarget
        else:
            raise ValueError(f)
    def zesFrequencyOcGetVoltageTarget(freq):
        f = freq_handle_to_ulong(freq)
        if f == 0:
            return state.stub["Freq"][f]["ocVolt"]
        else:
            raise ValueError(f)
    def zesFrequencyOcSetVoltageTarget(freq, ocVTarget, ocVOffset):
        f = freq_handle_to_ulong(freq)
        if f == 0:
            state.stub["Freq"][f]["ocVolt"] = (ocVTarget, ocVOffset)
        else:
            raise ValueError(f)
    def zesFrequencyOcSetMode(freq, ocMode):
        f = freq_handle_to_ulong(freq)
        if f == 0:
            state.stub["Freq"][f]["ocMode"] = ocMode
        else:
            raise ValueError(f)
    def zesFrequencyOcGetMode(freq):
        f = freq_handle_to_ulong(freq)
        if f == 0:
            return state.stub["Freq"][f]["ocMode"]
        else:
            return ZES_OC_MODE_OFF
    def zesFrequencyOcGetIccMax(freq):
        f = freq_handle_to_ulong(freq)
        if f == 0:
            return state.stub["Freq"][f]["iccMax"]
        else:
            raise ValueError(f)
    def zesFrequencyOcSetIccMax(freq, ocIccMax):
        f = freq_handle_to_ulong(freq)
        if f == 0:
            state.stub["Freq"][f]["iccMax"] = ocIccMax
        else:
            raise ValueError(f)
    def zesFrequencyOcGetTjMax(freq):
        f = freq_handle_to_ulong(freq)
        if f == 0:
            return state.stub["Freq"][f]["tjMax"]
        else:
            raise ValueError(f)
    def zesFrequencyOcSetTjMax(freq, ocTjMax):
        f = freq_handle_to_ulong(freq)
        if f == 0:
            state.stub["Freq"][f]["tjMax"] = ocTjMax
        else:
            raise ValueError(f)
    def zesDeviceEnumEngineGroups(dev, count_ptr, handle_array):
        if handle_array is None:
            uint32_assign(count_ptr, 9)
        elif uint32_value(count_ptr) == 9:
            e = state.stub.setdefault("Engine", {})
            e[ZES_ENGINE_GROUP_ALL] = { "activeTime" : 0, "timestamp" : 0 }
            e[ZES_ENGINE_GROUP_COMPUTE_ALL] = { "activeTime" : 0, "timestamp" : 0 }
            e[ZES_ENGINE_GROUP_MEDIA_ALL] = { "activeTime" : 0, "timestamp" : 0 }
            e[ZES_ENGINE_GROUP_COPY_ALL] = { "activeTime" : 0, "timestamp" : 0 }
            e[ZES_ENGINE_GROUP_COMPUTE_SINGLE] = { "activeTime" : 0, "timestamp" : 0 }
            e[ZES_ENGINE_GROUP_RENDER_SINGLE] = { "activeTime" : 0, "timestamp" : 0 }
            e[ZES_ENGINE_GROUP_MEDIA_DECODE_SINGLE] = { "activeTime" : 0, "timestamp" : 0 }
            e[ZES_ENGINE_GROUP_MEDIA_ENCODE_SINGLE] = { "activeTime" : 0, "timestamp" : 0 }
            e[ZES_ENGINE_GROUP_COPY_SINGLE] = { "activeTime" : 0, "timestamp" : 0 }
            items = zes_engine_handle_array.frompointer(handle_array)
            items[0] = ulong_to_engine_handle(ZES_ENGINE_GROUP_ALL)
            items[1] = ulong_to_engine_handle(ZES_ENGINE_GROUP_COMPUTE_ALL)
            items[2] = ulong_to_engine_handle(ZES_ENGINE_GROUP_MEDIA_ALL)
            items[3] = ulong_to_engine_handle(ZES_ENGINE_GROUP_COPY_ALL)
            items[4] = ulong_to_engine_handle(ZES_ENGINE_GROUP_COMPUTE_SINGLE)
            items[5] = ulong_to_engine_handle(ZES_ENGINE_GROUP_RENDER_SINGLE)
            items[6] = ulong_to_engine_handle(ZES_ENGINE_GROUP_MEDIA_DECODE_SINGLE)
            items[7] = ulong_to_engine_handle(ZES_ENGINE_GROUP_MEDIA_ENCODE_SINGLE)
            items[8] = ulong_to_engine_handle(ZES_ENGINE_GROUP_COPY_SINGLE)
        else:
            raise ValueError(uint32_value(count_ptr))
    def zesEngineGetProperties(eng, engProps):
        engProps.type = engine_handle_to_ulong(eng)
    def zesEngineGetActivity(eng, utilStats):
        e = state.stub["Engine"]
        etype = engine_handle_to_ulong(eng)
        stats = e[etype]
        stats["timestamp"] += 1000000
        if etype == ZES_ENGINE_GROUP_ALL:
            stats["activeTime"] += 700000
        if etype == ZES_ENGINE_GROUP_COMPUTE_ALL:
            stats["activeTime"] += 500000
        if etype == ZES_ENGINE_GROUP_MEDIA_ALL:
            stats["activeTime"] += 150000
        if etype == ZES_ENGINE_GROUP_COPY_ALL:
            stats["activeTime"] += 50000
        if etype == ZES_ENGINE_GROUP_COMPUTE_SINGLE:
            stats["activeTime"] += 300000
        if etype == ZES_ENGINE_GROUP_RENDER_SINGLE:
            stats["activeTime"] += 200000
        if etype == ZES_ENGINE_GROUP_MEDIA_DECODE_SINGLE:
            stats["activeTime"] += 30000
        if etype == ZES_ENGINE_GROUP_MEDIA_ENCODE_SINGLE:
            stats["activeTime"] += 120000
        if etype == ZES_ENGINE_GROUP_COPY_SINGLE:
            stats["activeTime"] += 50000
        utilStats.timestamp = stats["timestamp"]
        utilStats.activeTime = stats["activeTime"]
    def zesDevicePciGetProperties(dev, pciProps):
        pciProps.address.domain = 1
        pciProps.address.bus = 2
        pciProps.address.device = 3
        pciProps.address.function = 4
        pciProps.maxSpeed.gen = 4
        pciProps.maxSpeed.width = 16
        pciProps.maxSpeed.maxBandwidth = -1
        pciProps.haveBandwidthCounters = True
        pciProps.havePacketCounters = True
        pciProps.haveReplayCounters = True
    def zesDevicePciGetState(dev, pciState):
        pciState.status = ZES_PCI_LINK_STATUS_GOOD
        pciState.qualityIssues = 0
        pciState.stabilityIssues = 0
        pciState.speed.gen = 4
        pciState.speed.width = 4
        pciState.speed.maxBandwidth = 7880000000
    def zesDevicePciGetStats(dev, pciCounter):
        devs = state.stub.setdefault("Devices", {})
        d = devs.setdefault(device_handle_to_ulong(dev), {})
        r = d.setdefault("PCIstats", [0,0,0,0,0])
        pciCounter.timestamp = r[0]
        pciCounter.replayCounter = r[1]
        pciCounter.packetCounter = r[2]
        pciCounter.rxCounter = r[3]
        pciCounter.txCounter = r[4]
        r[0] += 1000000
        r[1] += 10
        r[2] += 1000
        r[3] += 788000000
        r[4] += 78800000
        pciCounter.speed.gen = 4
        pciCounter.speed.width = 4
        pciCounter.speed.maxBandwidth = 7880000000
    def zesDevicePciGetBars(dev, count_ptr, handle_array):
        if handle_array is None:
            uint32_assign(count_ptr, 3)
        elif uint32_value(count_ptr) == 3:
            items = zes_pci_bar_properties_array.frompointer(handle_array)
            item = items[0]
            item.type = ZES_PCI_BAR_TYPE_MMIO
            item.index = 0
            item.base = 0x0000400000000000
            item.size = 0x0000000000010000
            items[0] = item
            item = items[1]
            item.type = ZES_PCI_BAR_TYPE_ROM
            item.index = 1
            item.base = 0x0000400000080000
            item.size = 0x0000000000080000
            items[1] = item
            item = items[2]
            item.type = ZES_PCI_BAR_TYPE_MEM
            item.index = 2
            item.base = 0x0000400000100000
            item.size = 0x0000000000300000
            items[2] = item
        else:
            raise ValueError(uint32_value(count_ptr))
    def zesDeviceEnumDiagnosticTestSuites(dev, count_ptr, handle_array):
        if handle_array is None:
            uint32_assign(count_ptr, 2)
        elif uint32_value(count_ptr) == 2:
            items = zes_diag_handle_array.frompointer(handle_array)
            items[0] = ulong_to_diag_handle(0)
            items[1] = ulong_to_diag_handle(1)
        else:
            raise ValueError(uint32_value(count_ptr))
    def zesDiagnosticsGetProperties(diag, diagProps):
        diagProps.onSubdevice = False
        suite = diag_handle_to_ulong(diag)
        if suite == 0:
            diagProps.name = "ARRAY"
            diagProps.haveTests = True
        elif suite == 1:
            diagProps.name = "SCAN"
            diagProps.haveTests = False
        else:
            raise ValueError(suite)
    def zesDiagnosticsGetTests(diag, count_ptr, tests_array):
        suite = diag_handle_to_ulong(diag)
        if suite == 0:
            if tests_array is None:
                uint32_assign(count_ptr, 3)
            elif uint32_value(count_ptr) == 3:
                tests = zes_diag_test_array.frompointer(tests_array)
                test = tests[0]
                test.index = 65
                test.name = "A"
                tests[0] = test
                test = tests[1]
                test.index = 77
                test.name = "M"
                tests[1] = test
                test = tests[2]
                test.index = 80
                test.name = "P"
                tests[2] = test
            else:
                raise ValueError(uint32_value(count_ptr))
        elif suite == 1:
            if tests_array is None:
                uint32_assign(count_ptr, 0)
            elif uint32_value(count_ptr) != 0:
                raise ValueError(uint32_value(count_ptr))
        else:
            raise ValueError(suite)
    def zesDiagnosticsRunTests(diag, first, last):
        suite = diag_handle_to_ulong(diag)
        if suite == 0:
            result = ZES_DIAG_RESULT_ABORT
            if first <= 65 and last >= 65:
                result = ZES_DIAG_RESULT_NO_ERRORS
            if first <= 77 and last >= 77:
                result = ZES_DIAG_RESULT_REBOOT_FOR_REPAIR
            if first <= 80 and last >= 80:
                result = ZES_DIAG_RESULT_FAIL_CANT_REPAIR
            return result
        elif suite == 1:
            return ZES_DIAG_RESULT_NO_ERRORS
        else:
            raise ValueError(suite)
    def zesDeviceEventRegister(dev, events):
        devs = state.stub.setdefault("Devices", {})
        d = devs.setdefault(device_handle_to_ulong(dev), {})
        d["EventEnables"] = events
    def zesDriverEventListen(drv, timeout, count, device_array, event_array):
        devs = state.stub.setdefault("Devices", {})
        devices = ze_device_handle_array.frompointer(device_array)
        events = zes_event_type_flags_array.frompointer(event_array)
        for i in range(count):
            dev = devices[i]
            d = devs.setdefault(device_handle_to_ulong(dev), {})
            enables = d.setdefault("EventEnables", 0)
            events[i] = enables & (ZES_EVENT_TYPE_FLAG_FABRIC_PORT_HEALTH |
                                   ZES_EVENT_TYPE_FLAG_RAS_UNCORRECTABLE_ERRORS |
                                   ZES_EVENT_TYPE_FLAG_DEVICE_RESET_REQUIRED)
        return count
    def zesDeviceEnumFabricPorts(dev, count_ptr, handle_array):
        if handle_array is None:
            uint32_assign(count_ptr, 10)
        elif uint32_value(count_ptr) == 10:
            nextFabricId = state.stub.setdefault("NextFabricId", 0)
            devs = state.stub.setdefault("Devices", {})
            d = devs.setdefault(device_handle_to_ulong(dev), {})
            fabricId = d.setdefault("FabricId", nextFabricId)
            if fabricId == nextFabricId:
                state.stub["NextFabricId"] += 1
            items = zes_fabric_port_handle_array.frompointer(handle_array)
            for i in range(10):
                items[i] = ulong_to_fabric_port_handle(fabricId * 10 + i)
        else:
            raise ValueError(uint32_value(count_ptr))
    def zesFabricPortGetProperties(port, portProps):
        p = fabric_port_handle_to_ulong(port)
        fabricId = p // 10
        localIdx = p % 10
        portProps.model = "ANR"
        portProps.onSubdevice = True
        portProps.subdeviceId = localIdx // 5
        portProps.portId.fabricId = 0x10000 | fabricId
        portProps.portId.attachId = localIdx // 5
        portProps.portId.portNumber = localIdx % 5
        portProps.maxRxSpeed.bitRate = 1000000000
        portProps.maxRxSpeed.width = 8
        portProps.maxTxSpeed.bitRate = 1000000000
        portProps.maxTxSpeed.width = 8
    def zesFabricPortGetLinkType(port, linkType):
        linkType.desc = "ANR"
    def zesFabricPortGetConfig(port, portConfig):
        p = fabric_port_handle_to_ulong(port)
        ports = state.stub.setdefault("FabricPortConfig", {})
        portConfig.enabled, portConfig.beaconing = ports.setdefault(p, (True, False))
    def zesFabricPortSetConfig(port, portConfig):
        p = fabric_port_handle_to_ulong(port)
        ports = state.stub.setdefault("FabricPortConfig", {})
        ports[p] = (portConfig.enabled, portConfig.beaconing)
    def zesFabricPortGetState(port, portState):
        p = fabric_port_handle_to_ulong(port)
        ports = state.stub.setdefault("FabricPortConfig", {})
        enabled, _ = ports.setdefault(p, (True, False))
        fabricId = p // 10
        localIdx = p % 10
        portState.qualityIssues = 0
        portState.failureReasons = 0
        if enabled:
            if localIdx == 0:
                portState.status = ZES_FABRIC_PORT_STATUS_UNKNOWN
            elif localIdx == 1:
                portState.status = ZES_FABRIC_PORT_STATUS_HEALTHY
            elif localIdx == 2:
                portState.status = ZES_FABRIC_PORT_STATUS_DEGRADED
                portState.qualityIssues = ZES_FABRIC_PORT_QUAL_ISSUE_FLAG_LINK_ERRORS
            elif localIdx == 3:
                portState.status = ZES_FABRIC_PORT_STATUS_DEGRADED
                portState.qualityIssues = ZES_FABRIC_PORT_QUAL_ISSUE_FLAG_SPEED
            elif localIdx == 4:
                portState.status = ZES_FABRIC_PORT_STATUS_DEGRADED
                portState.qualityIssues = (ZES_FABRIC_PORT_QUAL_ISSUE_FLAG_LINK_ERRORS |
                                           ZES_FABRIC_PORT_QUAL_ISSUE_FLAG_SPEED)
            elif localIdx == 5:
                portState.status = ZES_FABRIC_PORT_STATUS_FAILED
                portState.failureReasons = (ZES_FABRIC_PORT_FAILURE_FLAG_TRAINING_TIMEOUT |
                                            ZES_FABRIC_PORT_FAILURE_FLAG_FAILED)
            elif localIdx == 6:
                portState.status = ZES_FABRIC_PORT_STATUS_FAILED
                portState.failureReasons = (ZES_FABRIC_PORT_FAILURE_FLAG_FLAPPING |
                                            ZES_FABRIC_PORT_FAILURE_FLAG_FAILED)
            elif localIdx == 7:
                portState.status = ZES_FABRIC_PORT_STATUS_FAILED
                portState.failureReasons = ZES_FABRIC_PORT_FAILURE_FLAG_TRAINING_TIMEOUT
            elif localIdx == 8:
                portState.status = ZES_FABRIC_PORT_STATUS_FAILED
                portState.failureReasons = ZES_FABRIC_PORT_FAILURE_FLAG_FLAPPING
            else:
                portState.status = ZES_FABRIC_PORT_STATUS_FAILED
                portState.failureReasons = ZES_FABRIC_PORT_FAILURE_FLAG_FAILED
        else:
            portState.status = ZES_FABRIC_PORT_STATUS_DISABLED
        portState.remotePortId.fabricId = 0x10000 | (fabricId ^ 1)
        portState.remotePortId.attachId = localIdx // 5
        portState.remotePortId.portNumber = localIdx % 5
        portState.rxSpeed.bitRate = 1000000000
        portState.rxSpeed.width = 8
        portState.txSpeed.bitRate = 1000000000
        portState.txSpeed.width = 8
    def zesFabricPortGetThroughput(port, portThroughput):
        p = fabric_port_handle_to_ulong(port)
        ports = state.stub.setdefault("FabricPortThroughput", {})
        r = ports.setdefault(p, [0,0,0])
        portThroughput.timestamp = r[0]
        portThroughput.rxCounter = r[1]
        portThroughput.txCounter = r[2]
        r[0] += 1000000
        r[1] += 1000000000
        r[2] += 900000000
    def zesDeviceEnumRasErrorSets(dev, count_ptr, handle_array):
        state.stub.setdefault("RasErrors", {ZES_RAS_ERROR_TYPE_CORRECTABLE : [2, 0, 0, 0, 0, 0, 3],
                                           ZES_RAS_ERROR_TYPE_UNCORRECTABLE : [1, 0, 0, 0, 0, 0, 1]})
        state.stub.setdefault("RasThresh", {ZES_RAS_ERROR_TYPE_CORRECTABLE : [2, 3, 1, 0, 0, 0, 0, 9],
                                           ZES_RAS_ERROR_TYPE_UNCORRECTABLE : [1, 0, 1, 2, 3, 1, 0, 5]})
        if handle_array is None:
            uint32_assign(count_ptr, 2)
        elif uint32_value(count_ptr) == 2:
            items = zes_ras_handle_array.frompointer(handle_array)
            items[0] = ulong_to_ras_handle(ZES_RAS_ERROR_TYPE_CORRECTABLE)
            items[1] = ulong_to_ras_handle(ZES_RAS_ERROR_TYPE_UNCORRECTABLE)
        else:
            raise ValueError(uint32_value(count_ptr))
    def zesRasGetProperties(ras, rasProps):
        rasProps.type = ras_handle_to_ulong(ras)
        rasProps.onSubdevice = False
        rasProps.subdeviceId = 0
    def zesRasGetConfig(ras, rasConfig):
        r = ras_handle_to_ulong(ras)
        thresholds = state.stub["RasThresh"][r]
        rasConfig.totalThreshold = thresholds[ZES_MAX_RAS_ERROR_CATEGORY_COUNT]
        for i in range(ZES_MAX_RAS_ERROR_CATEGORY_COUNT):
            ras_category_set(rasConfig.detailedThresholds, i, thresholds[i])
    def zesRasSetConfig(ras, rasConfig):
        r = ras_handle_to_ulong(ras)
        thresholds = state.stub["RasThresh"][r]
        thresholds[ZES_MAX_RAS_ERROR_CATEGORY_COUNT] = rasConfig.totalThreshold
        for i in range(ZES_MAX_RAS_ERROR_CATEGORY_COUNT):
            thresholds[i] = ras_category(rasConfig.detailedThresholds, i)
    def zesRasGetState(ras, clearErrors, rasState):
        r = ras_handle_to_ulong(ras)
        if clearErrors:
            state.stub["RasErrors"][r] = [0, 0, 0, 0, 0, 0, 0]
        if rasState is not None:
            errors = state.stub["RasErrors"][r]
            for i in range(ZES_MAX_RAS_ERROR_CATEGORY_COUNT):
                ras_category_set(rasState, i, errors[i])
                errors[i] += (i + 1)
    def zesDeviceProcessesGetState(dev, count_ptr, proc_array):
        if proc_array is None:
            uint32_assign(count_ptr, 4)
        elif uint32_value(count_ptr) == 4:
            procs = zes_process_state_array.frompointer(proc_array)
            proc = procs[0]
            proc.processId = 32765
            proc.memSize = 0x10000
            proc.sharedSize = 0x1000
            proc.engines = 1 << ZES_ENGINE_TYPE_FLAG_OTHER | 1 << ZES_ENGINE_TYPE_FLAG_COMPUTE
            procs[0] = proc
            proc = procs[1]
            proc.processId = 32766
            proc.memSize = 0x20000
            proc.sharedSize = 0x1000
            proc.engines = 1 << ZES_ENGINE_TYPE_FLAG_3D
            procs[1] = proc
            proc = procs[2]
            proc.processId = 32767
            proc.memSize = 0x30000
            proc.sharedSize = 0x1000
            proc.engines = 1 << ZES_ENGINE_TYPE_FLAG_MEDIA | 1 << ZES_ENGINE_TYPE_FLAG_DMA
            procs[2] = proc
            proc = procs[3]
            proc.processId = 32768
            proc.memSize = 0x40000
            proc.sharedSize = 0x1000
            proc.engines = (1 << ZES_ENGINE_TYPE_FLAG_OTHER | 1 << ZES_ENGINE_TYPE_FLAG_COMPUTE |
                            1 << ZES_ENGINE_TYPE_FLAG_3D | 1 << ZES_ENGINE_TYPE_FLAG_MEDIA |
                            1 << ZES_ENGINE_TYPE_FLAG_DMA)
            procs[3] = proc
        else:
            raise ValueError(uint32_value(count_ptr))
    def zesDeviceEnumFans(dev, count_ptr, handle_array):
        if handle_array is None:
            uint32_assign(count_ptr, 2)
        elif uint32_value(count_ptr) == 2:
            items = zes_fan_handle_array.frompointer(handle_array)
            items[0] = ulong_to_fan_handle(0)
            items[1] = ulong_to_fan_handle(1)
        else:
            raise ValueError(uint32_value(count_ptr))
    def zesFanSetDefaultMode(fan):
        f = fan_handle_to_ulong(fan)
        fanModes = state.stub.setdefault("FanMode", {})
        fanModes[f] = ZES_FAN_SPEED_MODE_DEFAULT
    def zesFanSetFixedSpeedMode(fan, speed):
        f = fan_handle_to_ulong(fan)
        fanModes = state.stub.setdefault("FanMode", {})
        fanModes[f] = ZES_FAN_SPEED_MODE_FIXED
        fanSpeeds = state.stub.setdefault("FanFixedSpeed", {})
        fanUnits = state.stub.setdefault("FanFixedSpeedUnits", {})
        fanSpeeds[f] = speed.speed
        fanUnits[f] = speed.units
    def zesFanSetSpeedTableMode(fan, speedTable):
        f = fan_handle_to_ulong(fan)
        fanModes = state.stub.setdefault("FanMode", {})
        fanModes[f] = ZES_FAN_SPEED_MODE_TABLE
        fanTables = state.stub.setdefault("FanTable", {})
        fanTablePoints = state.stub.setdefault("FanTablePoints", {})
        fanTablePoints[f] = speedTable.numPoints
        table = []
        tableSize = speedTable.numPoints
        tempSpeed = zes_fan_temp_speed_array(tableSize)
        fan_speed_table_get(speedTable, tableSize, tempSpeed.cast())
        for t in range(tableSize):
            item = tempSpeed[t]
            entry = (item.temperature, item.speed.speed, item.speed.units)
            table.append(entry)
        fanTables[f] = table
    def zesFanGetProperties(fan, fanProps):
        f = fan_handle_to_ulong(fan)
        fanProps.onSubdevice = f > 0
        fanProps.subdeviceId = f
        fanProps.canControl = True
        fanProps.supportedModes = 7
        fanProps.supportedUnits = 3
        fanProps.maxRPM = 10000
        fanProps.maxPoints = 10
    def zesFanGetConfig(fan, fanConfig):
        f = fan_handle_to_ulong(fan)
        fanModes = state.stub.setdefault("FanMode", {})
        fanConfig.mode = fanModes.setdefault(f, ZES_FAN_SPEED_MODE_DEFAULT)
        if fanConfig.mode == ZES_FAN_SPEED_MODE_FIXED:
            fanSpeeds = state.stub.setdefault("FanFixedSpeed", {})
            fanUnits = state.stub.setdefault("FanFixedSpeedUnits", {})
            fanConfig.speedFixed.speed = fanSpeeds.setdefault(f, 1000)
            fanConfig.speedFixed.units = fanUnits.setdefault(f, ZES_FAN_SPEED_UNITS_RPM)
        elif fanConfig.mode == ZES_FAN_SPEED_MODE_TABLE:
            fanTables = state.stub.setdefault("FanTable", {})
            fanTablePoints = state.stub.setdefault("FanTablePoints", {})
            tableSize = fanTablePoints.setdefault(f, 3)
            table = fanTables.setdefault(f, [(50, 1000, ZES_FAN_SPEED_UNITS_RPM),
                                             (100, 5000, ZES_FAN_SPEED_UNITS_RPM),
                                             (150, 10000, ZES_FAN_SPEED_UNITS_RPM)])
            tempSpeed = zes_fan_temp_speed_array(tableSize)
            for t, entry in indexed(table):
                item = tempSpeed[t]
                item.temperature = entry[0]
                item.speed.speed = entry[1]
                item.speed.units = entry[2]
                tempSpeed[t] = item
            # fan_config_speed_table_set(fanConfig, tableSize, tempSpeed)
    def zesFanGetState(fan, units):
        f = fan_handle_to_ulong(fan)
        fanModes = state.stub.setdefault("FanMode", {})
        mode = fanModes.setdefault(f, ZES_FAN_SPEED_MODE_DEFAULT)
        if mode == ZES_FAN_SPEED_MODE_FIXED:
            fanSpeeds = state.stub.setdefault("FanFixedSpeed", {})
            fanUnits = state.stub.setdefault("FanFixedSpeedUnits", {})
            current_speed = fanSpeeds.setdefault(f, 1000)
            current_units = fanUnits.setdefault(f, ZES_FAN_SPEED_UNITS_RPM)
        elif mode == ZES_FAN_SPEED_MODE_TABLE:
            fanTables = state.stub.setdefault("FanTable", {})
            fanTablePoints = state.stub.setdefault("FanTablePoints", {})
            fanTableIndices = state.stub.setdefault("FanTableIndices", {})
            numPoints = fanTablePoints.setdefault(f, 3)
            table = fanTables.setdefault(f, [(50, 1000, ZES_FAN_SPEED_UNITS_RPM),
                                             (100, 5000, ZES_FAN_SPEED_UNITS_RPM),
                                             (150, 10000, ZES_FAN_SPEED_UNITS_RPM)])
            idx = fanTableIndices.setdefault(f, 0)
            current_speed = table[idx][1]
            current_units = table[idx][2]
            if units == ZES_FAN_SPEED_UNITS_PERCENT:
                fanTableIndices[f] = (idx + 1) % len(table)
        else:
            current_speed = 2000
            current_units = ZES_FAN_SPEED_UNITS_RPM
        if current_units == units:
            return current_speed
        elif units == ZES_FAN_SPEED_UNITS_RPM:
            return current_speed * 100
        else:
            return current_speed // 100
    def zesDeviceEnumFirmwares(dev, count_ptr, handle_array):
        if handle_array is None:
            uint32_assign(count_ptr, 1)
        elif uint32_value(count_ptr) == 1:
            items = zes_firmware_handle_array.frompointer(handle_array)
            items[0] = ulong_to_firmware_handle(0)
        else:
            raise ValueError(uint32_value(count_ptr))
    def zesFirmwareGetProperties(fw, fwProps):
        fwProps.onSubdevice = False
        fwProps.subdeviceId = 0
        fwProps.canControl = True
        fwProps.name = "FIRMWARE"
        fwProps.version = "1.0.0"
    def zesFirmwareFlashData(handle, image):
        fw = firmware_handle_to_ulong(handle)
        logger.pr("Flashing firmware image", fw, "of size", len(image), ":")
        if image.isascii():
            lines = str(image, 'utf-8').splitlines()
            if len(lines) < 100:
                logger.pr(" " + "\n ".join(lines))
            else:
                logger.pr(" [ascii data, greater than 100 lines]")
        else:
            logger.pr(" [binary data]")
    def zesDeviceEnumLeds(dev, count_ptr, handle_array):
        if handle_array is None:
            uint32_assign(count_ptr, 2)
        elif uint32_value(count_ptr) == 2:
            l = state.stub.setdefault("Led", {})
            l[0] = { "on" : False, "color" : (1.0, 0.0, 0.0) }
            l[1] = { "on" : False, "color" : (1.0, 0.0, 0.0) }
            items = zes_led_handle_array.frompointer(handle_array)
            items[0] = ulong_to_led_handle(0)
            items[1] = ulong_to_led_handle(1)
        else:
            raise ValueError(uint32_value(count_ptr))
    def zesLedGetProperties(led, ledProps):
        l = led_handle_to_ulong(led)
        if l > 1:
            raise ValueError(l)
        ledProps.canControl = True
        ledProps.haveRGB = l==0
    def zesLedGetState(led, ledState):
        l = led_handle_to_ulong(led)
        if l > 1:
            raise ValueError(l)
        ledState.isOn = state.stub["Led"][l]["on"]
        color = zes_led_color_t()
        color.red,color.green,color.blue = state.stub["Led"][l]["color"]
        ledState.color = color
    def zesLedSetState(led, enable):
        l = led_handle_to_ulong(led)
        if l > 1:
            raise ValueError(l)
        state.stub["Led"][l]["on"] = enable
    def zesLedSetColor(led, color):
        l = led_handle_to_ulong(led)
        if l > 1:
            raise ValueError(l)
        state.stub["Led"][l]["color"] = color.red,color.green,color.blue
    def zesDeviceEnumMemoryModules(dev, count_ptr, handle_array):
        if handle_array is None:
            uint32_assign(count_ptr, 14)
        elif uint32_value(count_ptr) == 14:
            m = state.stub.setdefault("Mem", {})
            m[ZES_MEM_TYPE_HBM] = { "usage" : (1000000000, 1000000000), "bw" : (0, 0, 1000000000, 0) }
            m[ZES_MEM_TYPE_DDR] = { "usage" : (1000000000, 1000000000), "bw" : (0, 0, 1000000000, 0) }
            m[ZES_MEM_TYPE_DDR3] = { "usage" : (1000000000, 1000000000), "bw" : (0, 0, 1000000000, 0) }
            m[ZES_MEM_TYPE_DDR4] = { "usage" : (1000000000, 1000000000), "bw" : (0, 0, 1000000000, 0) }
            m[ZES_MEM_TYPE_DDR5] = { "usage" : (1000000000, 1000000000), "bw" : (0, 0, 1000000000, 0) }
            m[ZES_MEM_TYPE_LPDDR] = { "usage" : (1000000000, 1000000000), "bw" : (0, 0, 1000000000, 0) }
            m[ZES_MEM_TYPE_LPDDR3] = { "usage" : (1000000000, 1000000000), "bw" : (0, 0, 1000000000, 0) }
            m[ZES_MEM_TYPE_LPDDR4] = { "usage" : (1000000000, 1000000000), "bw" : (0, 0, 1000000000, 0) }
            m[ZES_MEM_TYPE_LPDDR5] = { "usage" : (1000000000, 1000000000), "bw" : (0, 0, 1000000000, 0) }
            m[ZES_MEM_TYPE_SRAM] = { "usage" : (1000000000, 1000000000), "bw" : (0, 0, 1000000000, 0) }
            m[ZES_MEM_TYPE_L1] = { "usage" : (1000000000, 1000000000), "bw" : (0, 0, 1000000000, 0) }
            m[ZES_MEM_TYPE_L3] = { "usage" : (1000000000, 1000000000), "bw" : (0, 0, 1000000000, 0) }
            m[ZES_MEM_TYPE_GRF] = { "usage" : (1000000000, 1000000000), "bw" : (0, 0, 1000000000, 0) }
            m[ZES_MEM_TYPE_SLM] = { "usage" : (1000000000, 1000000000), "bw" : (0, 0, 1000000000, 0) }
            items = zes_mem_handle_array.frompointer(handle_array)
            items[0] = ulong_to_mem_handle(ZES_MEM_TYPE_HBM)
            items[1] = ulong_to_mem_handle(ZES_MEM_TYPE_DDR)
            items[2] = ulong_to_mem_handle(ZES_MEM_TYPE_DDR3)
            items[3] = ulong_to_mem_handle(ZES_MEM_TYPE_DDR4)
            items[4] = ulong_to_mem_handle(ZES_MEM_TYPE_DDR5)
            items[5] = ulong_to_mem_handle(ZES_MEM_TYPE_LPDDR)
            items[6] = ulong_to_mem_handle(ZES_MEM_TYPE_LPDDR3)
            items[7] = ulong_to_mem_handle(ZES_MEM_TYPE_LPDDR4)
            items[8] = ulong_to_mem_handle(ZES_MEM_TYPE_LPDDR5)
            items[9] = ulong_to_mem_handle(ZES_MEM_TYPE_SRAM)
            items[10] = ulong_to_mem_handle(ZES_MEM_TYPE_L1)
            items[11] = ulong_to_mem_handle(ZES_MEM_TYPE_L3)
            items[12] = ulong_to_mem_handle(ZES_MEM_TYPE_GRF)
            items[13] = ulong_to_mem_handle(ZES_MEM_TYPE_SLM)
        else:
            raise ValueError(uint32_value(count_ptr))
    def zesMemoryGetProperties(mem, memProps):
        m = mem_handle_to_ulong(mem)
        if m > ZES_MEM_TYPE_SLM:
            raise ValueError(m)
        memProps.type = m
        if m < ZES_MEM_TYPE_LPDDR:
            memProps.onSubdevice = False
        else:
            memProps.onSubdevice = True
        memProps.subdeviceId = 0
        if m < ZES_MEM_TYPE_SRAM:
            memProps.location = ZES_MEM_LOC_SYSTEM
        else:
            memProps.location = ZES_MEM_LOC_DEVICE
        memProps.physicalSize = 2000000000
        memProps.busWidth = 64
        memProps.numChannels = 2
    def zesMemoryGetState(mem, memState):
        m = mem_handle_to_ulong(mem)
        memState.health = m % 5
        f, s = state.stub["Mem"][m]["usage"]
        memState.free, memState.size = f, s
        f -= 100000000 + m * 10000000
        while f < 0:
            f += s
        state.stub["Mem"][m]["usage"] = f, s
    def zesMemoryGetBandwidth(mem, memBw):
        m = mem_handle_to_ulong(mem)
        r, w, x, t = state.stub["Mem"][m]["bw"]
        memBw.readCounter, memBw.writeCounter, memBw.maxBandwidth, memBw.timestamp = r, w, x, t
        r += 200000000 + m * 10000000
        w += 100000000 + m * 20000000
        t += 1000000
        state.stub["Mem"][m]["bw"] = r, w, x, t
    def zesDeviceEnumPerformanceFactorDomains(dev, count_ptr, handle_array):
        if handle_array is None:
            uint32_assign(count_ptr, 5)
        elif uint32_value(count_ptr) == 5:
            items = zes_perf_handle_array.frompointer(handle_array)
            items[0] = ulong_to_perf_handle(ZES_ENGINE_TYPE_FLAG_COMPUTE)
            items[1] = ulong_to_perf_handle(ZES_ENGINE_TYPE_FLAG_COMPUTE | ZES_ENGINE_TYPE_FLAG_3D)
            items[2] = ulong_to_perf_handle(ZES_ENGINE_TYPE_FLAG_OTHER | ZES_ENGINE_TYPE_FLAG_MEDIA)
            items[3] = ulong_to_perf_handle(ZES_ENGINE_TYPE_FLAG_OTHER | ZES_ENGINE_TYPE_FLAG_DMA)
            items[4] = ulong_to_perf_handle(ZES_ENGINE_TYPE_FLAG_DMA)
        else:
            raise ValueError(uint32_value(count_ptr))
    def zesPerformanceFactorGetProperties(perf, perfProps):
        p = perf_handle_to_ulong(perf)
        perfProps.onSubdevice = False
        perfProps.subdeviceId = 0
        perfProps.engines = p
    def zesPerformanceFactorGetConfig(perf):
        p = perf_handle_to_ulong(perf)
        pds = state.stub.setdefault("Perf", {})
        return pds.setdefault(p, 50.0)
    def zesPerformanceFactorSetConfig(perf, val):
        p = perf_handle_to_ulong(perf)
        pds = state.stub.setdefault("Perf", {})
        pds[p] = val
    def zesDeviceEnumPsus(dev, count_ptr, handle_array):
        state.stub.setdefault("PsuState", ZES_PSU_VOLTAGE_STATUS_NORMAL)
        state.stub.setdefault("PsuTable", { ZES_PSU_VOLTAGE_STATUS_UNKNOWN : (True, 250, 100),
                                           ZES_PSU_VOLTAGE_STATUS_NORMAL : (False, 50, 200),
                                           ZES_PSU_VOLTAGE_STATUS_OVER : (True, 150, 750),
                                           ZES_PSU_VOLTAGE_STATUS_UNDER : (False, 75, 150) })
        if handle_array is None:
            uint32_assign(count_ptr, 1)
        elif uint32_value(count_ptr) == 1:
            items = zes_psu_handle_array.frompointer(handle_array)
            items[0] = ulong_to_psu_handle(0)
        else:
            raise ValueError(uint32_value(count_ptr))
    def zesPsuGetProperties(psu, psuProps):
        psuProps.onSubdevice = False
        psuProps.subdeviceId = 0
        psuProps.haveFan = True
        psuProps.ampLimit = 750
    def zesPsuGetState(psu, psuState):
        voltStatus = state.stub["PsuState"]
        psuState.voltStatus = voltStatus
        psuState.fanFailed, psuState.temperature, psuState.current = state.stub["PsuTable"][voltStatus]
        state.stub["PsuState"] = (voltStatus + 1) % 4
    def zesDeviceEnumSchedulers(dev, count_ptr, handle_array):
        DMA_ENGINE_TYPES = ZES_ENGINE_TYPE_FLAG_DMA | ZES_ENGINE_TYPE_FLAG_OTHER
        state.stub.setdefault("SchedMode", { ZES_ENGINE_TYPE_FLAG_COMPUTE : ZES_SCHED_MODE_TIMESLICE,
                                            ZES_ENGINE_TYPE_FLAG_3D : ZES_SCHED_MODE_TIMEOUT,
                                            ZES_ENGINE_TYPE_FLAG_MEDIA : ZES_SCHED_MODE_TIMEOUT,
                                            DMA_ENGINE_TYPES : ZES_SCHED_MODE_EXCLUSIVE })
        state.stub.setdefault("Timeout", { ZES_ENGINE_TYPE_FLAG_COMPUTE : 5000000,
                                          ZES_ENGINE_TYPE_FLAG_3D : 25000000,
                                          ZES_ENGINE_TYPE_FLAG_MEDIA : 15000000,
                                          DMA_ENGINE_TYPES : 1000000 })
        state.stub.setdefault("Timeslice", { ZES_ENGINE_TYPE_FLAG_COMPUTE : (500000, 5000000),
                                            ZES_ENGINE_TYPE_FLAG_3D : (1000000, 25000000),
                                            ZES_ENGINE_TYPE_FLAG_MEDIA : (750000, 15000000),
                                            DMA_ENGINE_TYPES : (500000, 1000000) })
        if handle_array is None:
            uint32_assign(count_ptr, 4)
        elif uint32_value(count_ptr) == 4:
            items = zes_sched_handle_array.frompointer(handle_array)
            items[0] = ulong_to_sched_handle(ZES_ENGINE_TYPE_FLAG_COMPUTE)
            items[1] = ulong_to_sched_handle(ZES_ENGINE_TYPE_FLAG_3D)
            items[2] = ulong_to_sched_handle(ZES_ENGINE_TYPE_FLAG_MEDIA)
            items[3] = ulong_to_sched_handle(DMA_ENGINE_TYPES)
        else:
            raise ValueError(uint32_value(count_ptr))
    def zesSchedulerGetProperties(sched, schedProps):
        s = sched_handle_to_ulong(sched)
        schedProps.onSubdevice = False
        schedProps.subdeviceId = 0
        schedProps.canControl = not (s & ZES_ENGINE_TYPE_FLAG_DMA)
        schedProps.engines = s
        supportedModes = 0
        if s & ZES_ENGINE_TYPE_FLAG_DMA:
            supportedModes = 1 << ZES_SCHED_MODE_EXCLUSIVE
        if s & ZES_ENGINE_TYPE_FLAG_COMPUTE:
            supportedModes |= 1 << ZES_SCHED_MODE_TIMEOUT
            supportedModes |= 1 << ZES_SCHED_MODE_TIMESLICE
            supportedModes |= 1 << ZES_SCHED_MODE_EXCLUSIVE
            supportedModes |= 1 << ZES_SCHED_MODE_COMPUTE_UNIT_DEBUG
        if s & ZES_ENGINE_TYPE_FLAG_3D:
            supportedModes |= 1 << ZES_SCHED_MODE_TIMESLICE
        if s & ZES_ENGINE_TYPE_FLAG_MEDIA:
            supportedModes |= 1 << ZES_SCHED_MODE_TIMEOUT
            supportedModes |= 1 << ZES_SCHED_MODE_EXCLUSIVE
        schedProps.supportedModes = supportedModes
    def zesSchedulerGetCurrentMode(sched):
        s = sched_handle_to_ulong(sched)
        return state.stub["SchedMode"][s]
    def zesSchedulerGetTimeoutModeProperties(sched, getDefaults, schedConfig):
        s = sched_handle_to_ulong(sched)
        DMA_ENGINE_TYPES = ZES_ENGINE_TYPE_FLAG_DMA | ZES_ENGINE_TYPE_FLAG_OTHER
        if getDefaults:
            timeoutProperties = { ZES_ENGINE_TYPE_FLAG_COMPUTE : 5000000,
                                  ZES_ENGINE_TYPE_FLAG_3D : 25000000,
                                  ZES_ENGINE_TYPE_FLAG_MEDIA : 15000000,
                                  DMA_ENGINE_TYPES : 1000000 }
        else:
            timeoutProperties = state.stub["Timeout"]
        schedConfig.watchdogTimeout = timeoutProperties[s]
    def zesSchedulerGetTimesliceModeProperties(sched, getDefaults, schedConfig):
        s = sched_handle_to_ulong(sched)
        DMA_ENGINE_TYPES = ZES_ENGINE_TYPE_FLAG_DMA | ZES_ENGINE_TYPE_FLAG_OTHER
        if getDefaults:
            timesliceProperties = { ZES_ENGINE_TYPE_FLAG_COMPUTE : (500000, 5000000),
                                    ZES_ENGINE_TYPE_FLAG_3D : (1000000, 25000000),
                                    ZES_ENGINE_TYPE_FLAG_MEDIA : (750000, 15000000),
                                    DMA_ENGINE_TYPES : (500000, 1000000) }
        else:
            timesliceProperties = state.stub["Timeslice"]
        schedConfig.interval, schedConfig.yieldTimeout = timesliceProperties[s]
    def zesSchedulerSetTimeoutMode(sched, schedConfig):
        s = sched_handle_to_ulong(sched)
        needReload = state.stub["SchedMode"][s] != ZES_SCHED_MODE_TIMEOUT
        state.stub["SchedMode"][s] = ZES_SCHED_MODE_TIMEOUT
        if schedConfig:
            state.stub["Timeout"][s] = schedConfig.watchdogTimeout
        return needReload
    def zesSchedulerSetTimesliceMode(sched, schedConfig):
        s = sched_handle_to_ulong(sched)
        needReload = state.stub["SchedMode"][s] != ZES_SCHED_MODE_TIMESLICE
        state.stub["SchedMode"][s] = ZES_SCHED_MODE_TIMESLICE
        if schedConfig:
            state.stub["Timeslice"][s] = schedConfig.interval, schedConfig.yieldTimeout
        return needReload
    def zesSchedulerSetExclusiveMode(sched):
        s = sched_handle_to_ulong(sched)
        needReload = state.stub["SchedMode"][s] != ZES_SCHED_MODE_EXCLUSIVE
        state.stub["SchedMode"][s] = ZES_SCHED_MODE_EXCLUSIVE
        return needReload
    def zesSchedulerSetComputeUnitDebugMode(sched):
        s = sched_handle_to_ulong(sched)
        needReload = state.stub["SchedMode"][s] != ZES_SCHED_MODE_COMPUTE_UNIT_DEBUG
        state.stub["SchedMode"][s] = ZES_SCHED_MODE_COMPUTE_UNIT_DEBUG
        return needReload
    def zesDeviceEnumStandbyDomains(dev, count_ptr, handle_array):
        if handle_array is None:
            uint32_assign(count_ptr, 1)
        elif uint32_value(count_ptr) == 1:
            items = zes_standby_handle_array.frompointer(handle_array)
            items[0] = ulong_to_standby_handle(0)
        else:
            raise ValueError(uint32_value(count_ptr))
    def zesStandbyGetProperties(stby, stbyProps):
        stbyProps.type = ZES_STANDBY_TYPE_GLOBAL
        stbyProps.onSubdevice = False
        stbyProps.subdeviceId = 0
    def zesStandbyGetMode(stby):
        return state.stub.setdefault("Standby", ZES_STANDBY_PROMO_MODE_DEFAULT)
    def zesStandbySetMode(stby, stbyMode):
        state.stub["Standby"] = stbyMode
    return locals().copy()

def topo():
    #
    # Stubs for show-topo testing
    #

    # All fabric connections:
    # (fabricId, attachId, portNum, remoteFabricId, remoteAttachId, remotePortNum)
    state.stubFabric = [(0,0,1,3,0,1), (0,0,5,1,1,7), (0,0,6,2,0,4), (0,0,7,5,0,1), (0,0,8,4,1,8),
                        (0,1,1,5,1,7), (0,1,4,3,1,4), (0,1,5,2,1,5), (0,1,7,4,0,8), (0,1,8,1,0,1),
                        (1,0,1,0,1,8), (1,0,4,2,1,7), (1,0,5,4,0,5), (1,0,7,5,1,5), (1,0,8,3,1,7),
                        (1,1,1,4,1,1), (1,1,4,5,0,8), (1,1,5,2,0,5), (1,1,7,0,0,5), (1,1,8,3,0,5),
                        (2,0,1,3,0,6), (2,0,4,0,0,6), (2,0,5,1,1,5), (2,0,7,5,0,7), (2,0,8,4,1,4),
                        (2,1,1,3,1,5), (2,1,4,4,0,4), (2,1,5,0,1,5), (2,1,7,1,0,4), (2,1,8,5,1,8),
                        (3,0,1,0,0,1), (3,0,5,1,1,8), (3,0,6,2,0,1), (3,0,7,5,0,4), (3,0,8,4,1,7),
                        (3,1,1,5,1,4), (3,1,4,0,1,4), (3,1,5,2,1,1), (3,1,7,1,0,8), (3,1,8,4,0,1),
                        (4,0,1,3,1,8), (4,0,4,2,1,4), (4,0,5,1,0,5), (4,0,7,5,1,1), (4,0,8,0,1,7),
                        (4,1,1,1,1,1), (4,1,4,2,0,8), (4,1,5,5,0,5), (4,1,7,3,0,8), (4,1,8,0,0,8),
                        (5,0,1,0,0,7), (5,0,4,3,0,7), (5,0,5,4,1,5), (5,0,7,2,0,7), (5,0,8,1,1,4),
                        (5,1,1,4,0,7), (5,1,4,3,1,1), (5,1,5,1,0,7), (5,1,7,0,1,1), (5,1,8,2,1,8)]

    def zeDriverGet(count_ptr, handle_array):
        if handle_array is None:
            uint32_assign(count_ptr, 1)
        elif uint32_value(count_ptr) == 1:
            items = ze_driver_handle_array.frompointer(handle_array)
            items[0] = ulong_to_driver_handle(0)
        else:
            raise ValueError(uint32_value(count_ptr))
    def zeDriverGetApiVersion(drv):
        return 0x00010000
    def zeDriverGetProperties(drv, drvProps):
        uuid = bytes(6) + b'\xcd\xab\xef\xbe\xad\xde' + bytes(4)
        write_driver_uuid(drvProps.uuid, uuid)
        drvProps.driverVersion = 1
    def zeDriverGetIpcProperties(drv, ipcProps):
        ipcProps.flags = ZE_IPC_PROPERTY_FLAG_MEMORY
    def zeDeviceGet(drv, count_ptr, handle_array):
        if handle_array is None:
            uint32_assign(count_ptr, 12)
        elif uint32_value(count_ptr) == 12:
            items = ze_device_handle_array.frompointer(handle_array)
            for d in range(12):
                items[d] = ulong_to_device_handle(d)
        else:
            raise ValueError(uint32_value(count_ptr))
    def zeDeviceGetProperties(dev, devProps):
        d = device_handle_to_ulong(dev)
        devProps.type = ZE_DEVICE_TYPE_GPU
        devProps.vendorId = 0x8086
        devProps.deviceId = 0xa0cd
        devProps.flags = 0
        devProps.subdeviceId = 0
        devProps.coreClockRate = 4000
        devProps.maxMemAllocSize = 1000000000
        devProps.maxHardwareContexts = 1
        devProps.maxCommandQueuePriority = 1
        devProps.numThreadsPerEU = 8
        devProps.physicalEUSimdWidth = 8
        devProps.numEUsPerSubslice = 8
        devProps.numSubslicesPerSlice = 1
        devProps.numSlices = 1
        devProps.timerResolution = 500
        devProps.timestampValidBits = 32
        devProps.kernelTimestampValidBits = 24
        uuid = b'\x86\x80\x00\x00\xcd\xa0' + bytes(6) + bytes([d,0,0,0])
        write_device_uuid(devProps.uuid, uuid)
        devProps.name = "stubDevice" + str(d)
    def zesDeviceEnumFabricPorts(dev, count_ptr, handle_array):
        d = device_handle_to_ulong(dev)
        if handle_array is None:
            uint32_assign(count_ptr, 10)
        elif uint32_value(count_ptr) == 10:
            items = zes_fabric_port_handle_array.frompointer(handle_array)
            for p in range(10):
                items[p] = ulong_to_fabric_port_handle(10 * d + p)
        else:
            raise ValueError(uint32_value(count_ptr))
    def zesFabricPortGetProperties(port, portProps):
        p = fabric_port_handle_to_ulong(port)
        fabricId, attachId, portNum, remoteFabricId, remoteAttachId, remotePortNum = state.stubFabric[p % 60]
        if p >= 60:
            fabricId += 6
            remoteFabricId += 6
        portProps.model = "STUBBY_PVC"
        portProps.onSubdevice = 0
        portProps.subdeviceId = attachId
        portProps.portId.fabricId = 0x10000 | fabricId
        portProps.portId.attachId = attachId
        portProps.portId.portNumber = portNum
        portProps.maxRxSpeed.bitRate = 53000000000
        portProps.maxRxSpeed.width = 4
        portProps.maxTxSpeed.bitRate = 53000000000
        portProps.maxTxSpeed.width = 4
    def zesFabricPortGetState(port, portState):
        p = fabric_port_handle_to_ulong(port)
        fabricId, attachId, portNum, remoteFabricId, remoteAttachId, remotePortNum = state.stubFabric[p % 60]
        if p >= 60:
            fabricId += 6
            remoteFabricId += 6
        portState.status = ZES_FABRIC_PORT_STATUS_HEALTHY
        portState.qualityIssues = 0
        portState.failureReasons = 0
        portState.remotePortId.fabricId = 0x10000 | remoteFabricId
        portState.remotePortId.attachId = remoteAttachId
        portState.remotePortId.portNumber = remotePortNum
        portState.rxSpeed.bitRate = 25000000000
        portState.rxSpeed.width = 4
        portState.txSpeed.bitRate = 25000000000
        portState.txSpeed.width = 4
    return locals().copy()

#
# Produce a function definition that maps to a dict of calling details
#
def replacement_map_to_call_dict(f,s):
    return "def %s(*args, **kwargs): return {'fn':%s['%s'], 'args':args, 'kwargs':kwargs}" % (f, s, f)
