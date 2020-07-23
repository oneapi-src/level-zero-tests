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
#include <level_zero/zes_api.h>

#define _THROWS_ZET_ERROR __attribute__((noreturn))
#define _CAN_THROW
void _zes_exception(const char *format, ...) _THROWS_ZET_ERROR;
void _initialize_zes() _CAN_THROW;
uint32_t driver_count() _CAN_THROW;

void _zes_exception(const char *format, ...)
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

void _initialize_zesysman()
{
    /*
     * Belts and suspenders: this should be set in the Python wrapper first:
     */
    if (setenv("ZES_ENABLE_SYSMAN", "1", 0) != 0)
    {
        _zes_exception("Error setting ZES_ENABLE_SYSMAN environment variable");
    }

    if (zeInit(0) != ZE_RESULT_SUCCESS)
    {
        _zes_exception("Error initializing L0 libraries");
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

/*
 * By setting ZES_ENABLE_SYSMAN here, it's in the Python environment also:
 */
%pythonbegin %{
import os
os.environ.setdefault('ZES_ENABLE_SYSMAN',"1")
%}

%init %{
    _initialize_zesysman();
%}

%include <typemaps.i>

// Suppress "const char *" field warnings:
%warnfilter(314) global;
%warnfilter(451) pBuildFlags;
%warnfilter(451) pKernelName;

// output parameters to convert to function outputs
%apply int * OUTPUT { ze_api_version_t* version };

%include <level_zero/ze_api.h>

%clear ze_api_version_t* version;

%warnfilter(+314) global;
%warnfilter(+451) pBuildFlags;
%warnfilter(+451) pKernelName;

// output parameters to convert to function outputs
%apply double * OUTPUT { double* pTemperature };
%apply int * OUTPUT { zes_standby_promo_mode_t* pMode };
%apply int * OUTPUT { zes_sched_mode_t* pMode };

%include <level_zero/zes_api.h>

%clear zes_sched_mode_t* pMode;
%clear zes_standby_promo_mode_t* pMode;
%clear double* pTemperature;


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

%array_class(ze_driver_handle_t, ze_driver_handle_array);
%array_class(ze_device_handle_t, ze_device_handle_array);
%array_class(zes_freq_handle_t, zes_freq_handle_array);
%array_class(zes_temp_handle_t, zes_temp_handle_array);
%array_class(zes_pwr_handle_t, zes_pwr_handle_array);
%array_class(zes_engine_handle_t, zes_engine_handle_array);
%array_class(zes_mem_handle_t, zes_mem_handle_array);
%array_class(zes_fabric_port_handle_t, zes_fabric_port_handle_array);
%array_class(zes_ras_handle_t, zes_ras_handle_array);
%array_class(zes_sched_handle_t, zes_sched_handle_array);
%array_class(zes_standby_handle_t, zes_standby_handle_array);
%array_class(double, double_array);
%array_class(zes_pci_bar_properties_t, zes_pci_bar_properties_array);
%array_class(zes_process_state_t, zes_process_state_array);

%pointer_cast(unsigned long, ze_driver_handle_t, ulong_to_driver_handle);
%pointer_cast(unsigned long, ze_device_handle_t, ulong_to_device_handle);
%pointer_cast(unsigned long, zes_freq_handle_t, ulong_to_freq_handle);
%pointer_cast(unsigned long, zes_temp_handle_t, ulong_to_temp_handle);
%pointer_cast(unsigned long, zes_pwr_handle_t, ulong_to_pwr_handle);
%pointer_cast(unsigned long, zes_engine_handle_t, ulong_to_engine_handle);
%pointer_cast(unsigned long, zes_mem_handle_t, ulong_to_mem_handle);
%pointer_cast(unsigned long, zes_fabric_port_handle_t, ulong_to_fabric_port_handle);
%pointer_cast(unsigned long, zes_ras_handle_t, ulong_to_ras_handle);
%pointer_cast(unsigned long, zes_standby_handle_t, ulong_to_standby_handle);

%pointer_cast(ze_driver_handle_t, unsigned long, driver_handle_to_ulong);
%pointer_cast(ze_device_handle_t, unsigned long, device_handle_to_ulong);
%pointer_cast(zes_freq_handle_t, unsigned long, freq_handle_to_ulong);
%pointer_cast(zes_temp_handle_t, unsigned long, temp_handle_to_ulong);
%pointer_cast(zes_pwr_handle_t, unsigned long, pwr_handle_to_ulong);
%pointer_cast(zes_engine_handle_t, unsigned long, engine_handle_to_ulong);
%pointer_cast(zes_mem_handle_t, unsigned long, mem_handle_to_ulong);
%pointer_cast(zes_fabric_port_handle_t, unsigned long, fabric_port_handle_to_ulong);
%pointer_cast(zes_ras_handle_t, unsigned long, ras_handle_to_ulong);
%pointer_cast(zes_standby_handle_t, unsigned long, standby_handle_to_ulong);
