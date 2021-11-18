/*
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 */
%module zes_wrap

%{
#include <sys/stat.h>
#include <stdarg.h>
#include <stdbool.h>
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

void _initialize_zes_wrap()
{
    unsigned int result;

    /*
     * Belts and suspenders: this should have been set by the Python wrapper
     */
    if (setenv("ZES_ENABLE_SYSMAN", "1", 0) != 0)
    {
        _zes_exception("Error setting ZES_ENABLE_SYSMAN environment variable");
    }

    result = zeInit(0);

    if (result != ZE_RESULT_SUCCESS)
    {
        _zes_exception("Error 0x%x while initializing L0 libraries", result);
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
    _initialize_zes_wrap();
%}

%include <typemaps.i>
%include <pybuffer.i>

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
%apply int * OUTPUT { zes_diag_result_t* pResult };
%apply int * OUTPUT { uint32_t* pNumDeviceEvents };
%apply int * OUTPUT { int32_t* pSpeed };
%apply double * OUTPUT { double* pCurrentOcFrequency };
%apply double * OUTPUT { double* pCurrentVoltageTarget };
%apply double * OUTPUT { double* pCurrentVoltageOffset };
%apply int * OUTPUT { zes_oc_mode_t* pCurrentOcMode };
%apply double * OUTPUT { double* pOcIccMax };
%apply double * OUTPUT { double* pOcTjMax };
%apply double * OUTPUT { double* pFactor };

%include <level_zero/zes_api.h>

%clear double* pFactor;
%clear double* pOcTjMax;
%clear double* pOcIccMax;
%clear zes_oc_mode_t* pCurrentOcMode;
%clear double* pCurrentVoltageOffset;
%clear double* pCurrentVoltageTarget;
%clear double* pCurrentOcFrequency;
%clear int32_t* pSpeed;
%clear uint32_t* pNumDeviceEvents;
%clear zes_diag_result_t* pResult;
%clear zes_sched_mode_t* pMode;
%clear zes_standby_promo_mode_t* pMode;
%clear double* pTemperature;

// This would be better but fails in Ubuntu 19.10 (and presumably SWIG 3):
// %pybuffer_binary(void* pImage, uint32_t size);

// So use a char-based wrapper instead:
%pybuffer_binary(char* pImage, uint32_t nBytes);
%inline %{
ze_result_t
zesFirmwareFlashData(zes_firmware_handle_t hFirmware, char* pImage, uint32_t nBytes)
{
    return zesFirmwareFlash(hFirmware, pImage, nBytes);
}
%}
%clear (char* pImage, uint32_t nBytes);

// Can use the same mechanism to access UUIDs in binary:
%pybuffer_binary(char* pUUID, uint32_t nBytes);
%inline %{
// Perhaps these should throw exceptions if nBytes is wrong
uint32_t write_driver_uuid(ze_driver_uuid_t *uuid_struct, char* pUUID, uint32_t nBytes)
{
    if (nBytes > ZE_MAX_DRIVER_UUID_SIZE)
    {
        nBytes = ZE_MAX_DRIVER_UUID_SIZE;
    }
    memcpy(uuid_struct->id, pUUID, nBytes);
}
uint32_t read_driver_uuid(char* pUUID, uint32_t nBytes, ze_driver_uuid_t *uuid_struct)
{
    if (nBytes > ZE_MAX_DRIVER_UUID_SIZE)
    {
        nBytes = ZE_MAX_DRIVER_UUID_SIZE;
    }
    memcpy(pUUID, uuid_struct->id, nBytes);
}
uint32_t write_device_uuid(ze_device_uuid_t *uuid_struct, char* pUUID, uint32_t nBytes)
{
    if (nBytes > ZE_MAX_DEVICE_UUID_SIZE)
    {
        nBytes = ZE_MAX_DEVICE_UUID_SIZE;
    }
    memcpy(uuid_struct->id, pUUID, nBytes);
}
uint32_t read_device_uuid(char* pUUID, uint32_t nBytes, ze_device_uuid_t *uuid_struct)
{
    if (nBytes > ZE_MAX_DEVICE_UUID_SIZE)
    {
        nBytes = ZE_MAX_DEVICE_UUID_SIZE;
    }
    memcpy(pUUID, uuid_struct->id, nBytes);
}
%}
%clear (char* pUUID, uint32_t nBytes);

//
// Standard C constructs
//

%include <carrays.i>
%include <cdata.i>
// %include <cmalloc.i>
%include <cpointer.i>
// %include <cstring.i>

%include <stdint.i>

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

void fan_speed_table_set(zes_fan_speed_table_t *t, int32_t numPoints, zes_fan_temp_speed_t *temp_speeds)
{
    int i;
    t->numPoints = numPoints;

    if (numPoints > 0 && numPoints <= ZES_FAN_TEMP_SPEED_PAIR_COUNT)
        for (i=0; i < numPoints; ++i)
            t->table[i] = temp_speeds[i];
}

void fan_speed_table_get(zes_fan_speed_table_t *t, int16_t limit, zes_fan_temp_speed_t *temp_speeds)
{
    int i;

    if (limit > t->numPoints)
        limit = t->numPoints;

    for (i=0; i < limit; ++i)
        temp_speeds[i] = t->table[i];
}

void ras_category_set(zes_ras_state_t *state, unsigned int idx, uint64_t val)
{
    if (idx < ZES_MAX_RAS_ERROR_CATEGORY_COUNT)
        state->category[idx] = val;
}

uint64_t ras_category(zes_ras_state_t *state, unsigned int idx)
{
    if (idx < ZES_MAX_RAS_ERROR_CATEGORY_COUNT)
        return state->category[idx];
    else
        return ~0ULL;
}
%}

%pointer_class(int32_t, int32_ptr)
%pointer_class(uint32_t, uint32_ptr)
%pointer_class(uint64_t, uint64_ptr)
%pointer_functions(int32_t, int32)
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
%array_class(zes_diag_handle_t, zes_diag_handle_array);
%array_class(double, double_array);
%array_class(zes_pci_bar_properties_t, zes_pci_bar_properties_array);
%array_class(zes_process_state_t, zes_process_state_array);
%array_class(zes_diag_test_t, zes_diag_test_array);
%array_class(zes_event_type_flags_t, zes_event_type_flags_array);
%array_class(zes_fan_handle_t, zes_fan_handle_array);
%array_class(zes_fan_temp_speed_t, zes_fan_temp_speed_array);
%array_class(zes_firmware_handle_t, zes_firmware_handle_array);
%array_class(zes_led_handle_t, zes_led_handle_array);
%array_class(zes_perf_handle_t, zes_perf_handle_array);
%array_class(zes_psu_handle_t, zes_psu_handle_array);

%pointer_cast(unsigned long, ze_driver_handle_t, ulong_to_driver_handle);
%pointer_cast(unsigned long, ze_device_handle_t, ulong_to_device_handle);
%pointer_cast(unsigned long, zes_freq_handle_t, ulong_to_freq_handle);
%pointer_cast(unsigned long, zes_temp_handle_t, ulong_to_temp_handle);
%pointer_cast(unsigned long, zes_pwr_handle_t, ulong_to_pwr_handle);
%pointer_cast(unsigned long, zes_engine_handle_t, ulong_to_engine_handle);
%pointer_cast(unsigned long, zes_mem_handle_t, ulong_to_mem_handle);
%pointer_cast(unsigned long, zes_fabric_port_handle_t, ulong_to_fabric_port_handle);
%pointer_cast(unsigned long, zes_ras_handle_t, ulong_to_ras_handle);
%pointer_cast(unsigned long, zes_sched_handle_t, ulong_to_sched_handle);
%pointer_cast(unsigned long, zes_standby_handle_t, ulong_to_standby_handle);
%pointer_cast(unsigned long, zes_diag_handle_t, ulong_to_diag_handle);
%pointer_cast(unsigned long, zes_fan_handle_t, ulong_to_fan_handle);
%pointer_cast(unsigned long, zes_firmware_handle_t, ulong_to_firmware_handle);
%pointer_cast(unsigned long, zes_led_handle_t, ulong_to_led_handle);
%pointer_cast(unsigned long, zes_perf_handle_t, ulong_to_perf_handle);
%pointer_cast(unsigned long, zes_psu_handle_t, ulong_to_psu_handle);

%pointer_cast(ze_driver_handle_t, unsigned long, driver_handle_to_ulong);
%pointer_cast(ze_device_handle_t, unsigned long, device_handle_to_ulong);
%pointer_cast(zes_freq_handle_t, unsigned long, freq_handle_to_ulong);
%pointer_cast(zes_temp_handle_t, unsigned long, temp_handle_to_ulong);
%pointer_cast(zes_pwr_handle_t, unsigned long, pwr_handle_to_ulong);
%pointer_cast(zes_engine_handle_t, unsigned long, engine_handle_to_ulong);
%pointer_cast(zes_mem_handle_t, unsigned long, mem_handle_to_ulong);
%pointer_cast(zes_fabric_port_handle_t, unsigned long, fabric_port_handle_to_ulong);
%pointer_cast(zes_ras_handle_t, unsigned long, ras_handle_to_ulong);
%pointer_cast(zes_sched_handle_t, unsigned long, sched_handle_to_ulong);
%pointer_cast(zes_standby_handle_t, unsigned long, standby_handle_to_ulong);
%pointer_cast(zes_diag_handle_t, unsigned long, diag_handle_to_ulong);
%pointer_cast(zes_fan_handle_t, unsigned long, fan_handle_to_ulong);
%pointer_cast(zes_firmware_handle_t, unsigned long, firmware_handle_to_ulong);
%pointer_cast(zes_led_handle_t, unsigned long, led_handle_to_ulong);
%pointer_cast(zes_perf_handle_t, unsigned long, perf_handle_to_ulong);
%pointer_cast(zes_psu_handle_t, unsigned long, psu_handle_to_ulong);
