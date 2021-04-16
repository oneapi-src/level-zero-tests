#!/usr/bin/env python3
# Copyright (C) 2020 Intel Corporation
# SPDX-License-Identifier: MIT

from . import output
from zesys.zes_wrap import *

def default_structure_init(stype, obj):
    obj.stype = stype
    return obj

def ze_typed_structure(stype):
    ctor_map = { ZE_STRUCTURE_TYPE_DRIVER_PROPERTIES : ze_driver_properties_t,
                 ZE_STRUCTURE_TYPE_DRIVER_IPC_PROPERTIES : ze_driver_ipc_properties_t,
                 ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES : ze_device_properties_t,
                 ZE_STRUCTURE_TYPE_DEVICE_COMPUTE_PROPERTIES : ze_device_compute_properties_t,
                 ZE_STRUCTURE_TYPE_DEVICE_MODULE_PROPERTIES : ze_device_module_properties_t,
                 ZE_STRUCTURE_TYPE_COMMAND_QUEUE_GROUP_PROPERTIES : ze_command_queue_group_properties_t,
                 ZE_STRUCTURE_TYPE_DEVICE_MEMORY_PROPERTIES : ze_device_memory_properties_t,
                 ZE_STRUCTURE_TYPE_DEVICE_MEMORY_ACCESS_PROPERTIES : ze_device_memory_access_properties_t,
                 ZE_STRUCTURE_TYPE_DEVICE_CACHE_PROPERTIES : ze_device_cache_properties_t,
                 ZE_STRUCTURE_TYPE_DEVICE_IMAGE_PROPERTIES : ze_device_image_properties_t,
                 ZE_STRUCTURE_TYPE_DEVICE_P2P_PROPERTIES : ze_device_p2p_properties_t,
                 ZE_STRUCTURE_TYPE_CONTEXT_DESC : ze_context_desc_t,
                 ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC : ze_command_queue_desc_t,
                 ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC : ze_command_list_desc_t,
                 ZE_STRUCTURE_TYPE_EVENT_POOL_DESC : ze_event_pool_desc_t,
                 ZE_STRUCTURE_TYPE_EVENT_DESC : ze_event_desc_t,
                 ZE_STRUCTURE_TYPE_FENCE_DESC : ze_fence_desc_t,
                 ZE_STRUCTURE_TYPE_IMAGE_DESC : ze_image_desc_t,
                 ZE_STRUCTURE_TYPE_IMAGE_PROPERTIES : ze_image_properties_t,
                 ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC : ze_device_mem_alloc_desc_t,
                 ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC : ze_host_mem_alloc_desc_t,
                 ZE_STRUCTURE_TYPE_MEMORY_ALLOCATION_PROPERTIES : ze_memory_allocation_properties_t,
                 ZE_STRUCTURE_TYPE_MODULE_DESC : ze_module_desc_t,
                 ZE_STRUCTURE_TYPE_MODULE_PROPERTIES : ze_module_properties_t,
                 ZE_STRUCTURE_TYPE_KERNEL_DESC : ze_kernel_desc_t,
                 ZE_STRUCTURE_TYPE_KERNEL_PROPERTIES : ze_kernel_properties_t,
                 ZE_STRUCTURE_TYPE_SAMPLER_DESC : ze_sampler_desc_t,
                 ZE_STRUCTURE_TYPE_PHYSICAL_MEM_DESC : ze_physical_mem_desc_t }
    init_map = { }

    ctor = ctor_map.get(stype, ze_base_properties_t)
    init = init_map.get(stype, default_structure_init)
    return init(stype, ctor())

def zes_device_properties_init(stype, obj):
    obj = default_structure_init(stype, obj)
    obj.core.stype = ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES
    return obj

def zes_typed_structure(stype):
    ctor_map = { ZES_STRUCTURE_TYPE_DEVICE_PROPERTIES : zes_device_properties_t,
                 ZES_STRUCTURE_TYPE_PCI_PROPERTIES : zes_pci_properties_t,
                 ZES_STRUCTURE_TYPE_PCI_BAR_PROPERTIES : zes_pci_bar_properties_t,
                 ZES_STRUCTURE_TYPE_DIAG_PROPERTIES : zes_diag_properties_t,
                 ZES_STRUCTURE_TYPE_ENGINE_PROPERTIES : zes_engine_properties_t,
                 ZES_STRUCTURE_TYPE_FABRIC_PORT_PROPERTIES : zes_fabric_port_properties_t,
                 ZES_STRUCTURE_TYPE_FAN_PROPERTIES : zes_fan_properties_t,
                 ZES_STRUCTURE_TYPE_FIRMWARE_PROPERTIES : zes_firmware_properties_t,
                 ZES_STRUCTURE_TYPE_FREQ_PROPERTIES : zes_freq_properties_t,
                 ZES_STRUCTURE_TYPE_LED_PROPERTIES : zes_led_properties_t,
                 ZES_STRUCTURE_TYPE_MEM_PROPERTIES : zes_mem_properties_t,
                 ZES_STRUCTURE_TYPE_PERF_PROPERTIES : zes_perf_properties_t,
                 ZES_STRUCTURE_TYPE_POWER_PROPERTIES : zes_power_properties_t,
                 ZES_STRUCTURE_TYPE_PSU_PROPERTIES : zes_psu_properties_t,
                 ZES_STRUCTURE_TYPE_RAS_PROPERTIES : zes_ras_properties_t,
                 ZES_STRUCTURE_TYPE_SCHED_PROPERTIES : zes_sched_properties_t,
                 ZES_STRUCTURE_TYPE_SCHED_TIMEOUT_PROPERTIES : zes_sched_timeout_properties_t,
                 ZES_STRUCTURE_TYPE_SCHED_TIMESLICE_PROPERTIES : zes_sched_timeslice_properties_t,
                 ZES_STRUCTURE_TYPE_STANDBY_PROPERTIES : zes_standby_properties_t,
                 ZES_STRUCTURE_TYPE_TEMP_PROPERTIES : zes_temp_properties_t,
                 ZES_STRUCTURE_TYPE_DEVICE_STATE : zes_device_state_t,
                 ZES_STRUCTURE_TYPE_PROCESS_STATE : zes_process_state_t,
                 ZES_STRUCTURE_TYPE_PCI_STATE : zes_pci_state_t,
                 ZES_STRUCTURE_TYPE_FABRIC_PORT_CONFIG : zes_fabric_port_config_t,
                 ZES_STRUCTURE_TYPE_FABRIC_PORT_STATE : zes_fabric_port_state_t,
                 ZES_STRUCTURE_TYPE_FAN_CONFIG : zes_fan_config_t,
                 ZES_STRUCTURE_TYPE_FREQ_STATE : zes_freq_state_t,
                 ZES_STRUCTURE_TYPE_OC_CAPABILITIES : zes_oc_capabilities_t,
                 ZES_STRUCTURE_TYPE_LED_STATE : zes_led_state_t,
                 ZES_STRUCTURE_TYPE_MEM_STATE : zes_mem_state_t,
                 ZES_STRUCTURE_TYPE_PSU_STATE : zes_psu_state_t,
                 ZES_STRUCTURE_TYPE_BASE_STATE : zes_base_state_t,
                 ZES_STRUCTURE_TYPE_RAS_CONFIG : zes_ras_config_t,
                 ZES_STRUCTURE_TYPE_RAS_STATE : zes_ras_state_t,
                 ZES_STRUCTURE_TYPE_TEMP_CONFIG : zes_temp_config_t }
    init_map = { ZES_STRUCTURE_TYPE_DEVICE_PROPERTIES : zes_device_properties_init }

    ctor = ctor_map.get(stype, zes_base_properties_t)
    init = init_map.get(stype, default_structure_init)
    return init(stype, ctor())

def zes_typed_array(stype, count):
    ctor_map = { ZES_STRUCTURE_TYPE_PCI_BAR_PROPERTIES : zes_pci_bar_properties_array,
                 ZES_STRUCTURE_TYPE_PROCESS_STATE : zes_process_state_array }
    init_map = { }

    # no default array constructor, size must be known:
    array_ctor = ctor_map[stype]
    init = init_map.get(stype, default_structure_init)

    array = array_ctor(count)
    for i in range(count):
        init(stype, array[i])

    return array

#
# Parse ZE function result, stripping (first) return code and converting non-success to exception,
# forwarding to stub handler if function has been replaced
#
def zeCall(rc):
    if type(rc) == type({}):
        return rc['fn'](*rc['args'], **rc['kwargs'])
    elif type(rc) in (type(()),type([])):
        val = rc[1:]
        rc = rc[0]
        if len(val) == 1:
            val = val[0]
    else:
        val = None
    if (rc == ZE_RESULT_ERROR_NOT_AVAILABLE or
        rc == ZE_RESULT_ERROR_DEPENDENCY_UNAVAILABLE or
        rc == ZE_RESULT_ERROR_UNSUPPORTED_VERSION or
        rc == ZE_RESULT_ERROR_UNSUPPORTED_FEATURE):
        raise NotImplementedError
    elif rc != ZE_RESULT_SUCCESS:
        raise ValueError(output.resultString(rc))
    return val
