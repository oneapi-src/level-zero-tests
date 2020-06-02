/*
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 */
%module zesysman

%{
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <level_zero/zet_api.h>

#define _THROWS_ZET_ERROR __attribute__((noreturn))
#define _CAN_THROW
void _zet_exception(const char *format, ...) _THROWS_ZET_ERROR;
void _initialize_zet() _CAN_THROW;
uint32_t driver_count() _CAN_THROW;

void _zet_exception(const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);

    /* A bit draconian, perhaps replace with some type of exception throwing */
    fprintf(stderr, "\nKILLING PROCESS!\n");
    exit(1);
}

/* TODO: Should there be a way to disable this (call from Python instead)? */

void _initialize_zet()
{
    if (zeInit(ZE_INIT_FLAG_NONE) != ZE_RESULT_SUCCESS ||
        zetInit(ZE_INIT_FLAG_NONE) != ZE_RESULT_SUCCESS)
    {
        _zet_exception("Error initializing L0 libraries");
    }
}

static char hexdigit(int i)
{
    return (i > 9) ? 'a' - 10 + i : '0' + i;
}

static void generic_uuid_to_string(const uint8_t *id, int bytes, char *s)
{
    int i;

    for (i = bytes - 1; i >= 0; --i)
    {
        *s++ = hexdigit(id[i] / 0x10);
        *s++ = hexdigit(id[i] % 0x10);
        if (i >= 6 && i <= 12 && (i & 1) == 0)
        {
            *s++ = '-';
        }
    }
    *s = '\0';
}
%}

%init %{
    _initialize_zet();
%}

%include <typemaps.i>

//
// Declarations from ze_api.h
//

%include <level_zero/ze_common.h>

%apply int * OUTPUT { ze_api_version_t* version };
%include <level_zero/ze_driver.h>
%clear ze_api_version_t* version;

%include <level_zero/ze_device.h>

// %include "ze_cmdqueue.h"
// %include "ze_cmdlist.h"
// %include "ze_image.h"
// %include "ze_barrier.h"

// Suppress "const char *" field warnings from ze_module.h:
%warnfilter(451) pBuildFlags;
%warnfilter(451) pKernelName;

%include <level_zero/ze_module.h>

%warnfilter(+451) pBuildFlags;
%warnfilter(+451) pKernelName;

// %include "ze_residency.h"
// %include "ze_cl_interop.h"
// %include "ze_event.h"
// %include "ze_sampler.h"
// %include "ze_memory.h"
// %include "ze_fence.h"
// %include "ze_copy.h"

//
// Declarations from zet_api.h
//

%include <level_zero/zet_common.h>
%include <level_zero/zet_driver.h>
%include <level_zero/zet_device.h>

// %include "zet_cmdlist.h"

#ifndef ZET_STRING_PROPERTY_SIZE
#define ZET_STRING_PROPERTY_SIZE  64
#endif

#ifndef ZET_MAX_FABRIC_PORT_MODEL_SIZE
#define ZET_MAX_FABRIC_PORT_MODEL_SIZE  256
#endif

#ifndef ZET_MAX_FABRIC_LINK_TYPE_SIZE
#define ZET_MAX_FABRIC_LINK_TYPE_SIZE  256
#endif

typedef struct _zet_sysman_properties_t
{
    ze_device_properties_t core;
    uint32_t numSubdevices;
    char serialNumber[ZET_STRING_PROPERTY_SIZE];
    char boardNumber[ZET_STRING_PROPERTY_SIZE];
    char brandName[ZET_STRING_PROPERTY_SIZE];
    char modelName[ZET_STRING_PROPERTY_SIZE];
    char vendorName[ZET_STRING_PROPERTY_SIZE];
    char driverVersion[ZET_STRING_PROPERTY_SIZE];
} zet_sysman_properties_t;

typedef struct _zet_firmware_properties_t
{
    ze_bool_t onSubdevice;
    uint32_t subdeviceId;
    ze_bool_t canControl;
    char name[ZET_STRING_PROPERTY_SIZE];
    char version[ZET_STRING_PROPERTY_SIZE];
} zet_firmware_properties_t;

typedef struct _zet_fabric_port_properties_t
{
    char model[ZET_MAX_FABRIC_PORT_MODEL_SIZE];
    ze_bool_t onSubdevice;
    uint32_t subdeviceId;
    zet_fabric_port_uuid_t portUuid;
    zet_fabric_port_speed_t maxRxSpeed;
    zet_fabric_port_speed_t maxTxSpeed;
} zet_fabric_port_properties_t;

typedef struct _zet_fabric_link_type_t
{
    int8_t desc[ZET_MAX_FABRIC_LINK_TYPE_SIZE];
} zet_fabric_link_type_t;

%warnfilter(302) _zet_sysman_properties_t;
%warnfilter(302) _zet_firmware_properties_t;
%warnfilter(302) _zet_fabric_port_properties_t;
%warnfilter(302) _zet_fabric_link_type_t;

%apply double * OUTPUT { double* pTemperature };
%apply int * OUTPUT { zet_standby_promo_mode_t* pMode };
%apply int * OUTPUT { zet_repair_status_t* pRepairStatus };
%apply int * OUTPUT { zet_sched_mode_t* pMode };
%apply int * OUTPUT { ze_bool_t* pNeedReboot };

%include <level_zero/zet_sysman.h>

%clear ze_bool_t* pNeedReboot;
%clear zet_sched_mode_t* pMode;
%clear zet_repair_status_t* pRepairStatus;
%clear zet_standby_promo_mode_t* pMode;
%clear double* pTemperature;

%warnfilter(+302) _zet_sysman_properties_t;
%warnfilter(+302) _zet_firmware_properties_t;
%warnfilter(+302) _zet_fabric_port_properties_t;
%warnfilter(+302) _zet_fabric_link_type_t;

// %include "zet_module.h"
// %include "zet_pin.h"
// %include "zet_metric.h"
// %include "zet_debug.h"
// %include "zet_tracing.h"

//
// Standard C constructs
//

%include "carrays.i"
// %include "cdata.i"
// %include "cmalloc.i"
%include "cpointer.i"
// %include "cstring.i"

%include "stdint.i"

//
// Helpers
//

%inline %{
#define MAX_UUID_STRING_SIZE    49

typedef struct _uuid_string_t
{
    char id[MAX_UUID_STRING_SIZE];
} uuid_string_t;

void driver_uuid_to_string(ze_driver_uuid_t *uuid_struct, uuid_string_t *uuid_string)
{
    generic_uuid_to_string(uuid_struct->id, ZE_MAX_DRIVER_UUID_SIZE, uuid_string->id);
}

void device_uuid_to_string(ze_device_uuid_t *uuid_struct, uuid_string_t *uuid_string)
{
    generic_uuid_to_string(uuid_struct->id, ZE_MAX_DEVICE_UUID_SIZE, uuid_string->id);
}
%}

%pointer_class(uint32_t, uint32_ptr)
%pointer_class(uint64_t, uint64_ptr)
%pointer_functions(uint32_t, uint32)
%pointer_functions(uint64_t, uint64)
%pointer_class(zet_sysman_handle_t, zet_sysman_handle_ptr);

%array_class(ze_driver_handle_t, ze_driver_handle_array);
%array_class(ze_device_handle_t, ze_device_handle_array);
%array_class(zet_sysman_freq_handle_t, zet_sysman_freq_handle_array);
%array_class(zet_sysman_temp_handle_t, zet_sysman_temp_handle_array);
%array_class(zet_sysman_pwr_handle_t, zet_sysman_pwr_handle_array);
%array_class(zet_sysman_engine_handle_t, zet_sysman_engine_handle_array);
%array_class(zet_sysman_mem_handle_t, zet_sysman_mem_handle_array);
%array_class(zet_sysman_fabric_port_handle_t, zet_sysman_fabric_port_handle_array);
%array_class(zet_sysman_ras_handle_t, zet_sysman_ras_handle_array);
%array_class(zet_sysman_standby_handle_t, zet_sysman_standby_handle_array);
%array_class(double, double_array);
%array_class(zet_pci_bar_properties_t, zet_pci_bar_properties_array);
%array_class(zet_process_state_t, zet_process_state_array);
%array_class(zet_sched_mode_t, zet_sched_mode_array);

%pointer_cast(unsigned long, zet_sysman_handle_t, ulong_to_sysman_handle);
%pointer_cast(unsigned long, ze_driver_handle_t, ulong_to_driver_handle);
%pointer_cast(unsigned long, ze_device_handle_t, ulong_to_device_handle);
%pointer_cast(unsigned long, zet_sysman_freq_handle_t, ulong_to_freq_handle);
%pointer_cast(unsigned long, zet_sysman_temp_handle_t, ulong_to_temp_handle);
%pointer_cast(unsigned long, zet_sysman_pwr_handle_t, ulong_to_pwr_handle);
%pointer_cast(unsigned long, zet_sysman_engine_handle_t, ulong_to_engine_handle);
%pointer_cast(unsigned long, zet_sysman_mem_handle_t, ulong_to_mem_handle);
%pointer_cast(unsigned long, zet_sysman_fabric_port_handle_t, ulong_to_fabric_port_handle);
%pointer_cast(unsigned long, zet_sysman_ras_handle_t, ulong_to_ras_handle);
%pointer_cast(unsigned long, zet_sysman_standby_handle_t, ulong_to_standby_handle);

%pointer_cast(zet_sysman_handle_t, unsigned long, sysman_handle_to_ulong);
%pointer_cast(ze_driver_handle_t, unsigned long, driver_handle_to_ulong);
%pointer_cast(ze_device_handle_t, unsigned long, device_handle_to_ulong);
%pointer_cast(zet_sysman_freq_handle_t, unsigned long, freq_handle_to_ulong);
%pointer_cast(zet_sysman_temp_handle_t, unsigned long, temp_handle_to_ulong);
%pointer_cast(zet_sysman_pwr_handle_t, unsigned long, pwr_handle_to_ulong);
%pointer_cast(zet_sysman_engine_handle_t, unsigned long, engine_handle_to_ulong);
%pointer_cast(zet_sysman_mem_handle_t, unsigned long, mem_handle_to_ulong);
%pointer_cast(zet_sysman_fabric_port_handle_t, unsigned long, fabric_port_handle_to_ulong);
%pointer_cast(zet_sysman_ras_handle_t, unsigned long, ras_handle_to_ulong);
%pointer_cast(zet_sysman_standby_handle_t, unsigned long, standby_handle_to_ulong);

// TODO: consider writing typemaps for ze_bool_t
