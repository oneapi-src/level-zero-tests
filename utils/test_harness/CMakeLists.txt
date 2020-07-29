#
# Copyright (C) 2020 Intel Corporation
#
# SPDX-License-Identifier: MIT
#


add_core_library(test_harness
    SOURCE
    "src/test_harness_device.cpp"
    "src/test_harness_driver.cpp"
    "src/test_harness_cmdlist.cpp"
    "src/test_harness_cmdqueue.cpp"
    "src/test_harness_event.cpp"
    "src/test_harness_memory.cpp"
    "src/test_harness_image.cpp"
    "src/test_harness_fence.cpp"
    "src/test_harness_module.cpp"
    "src/test_harness_sampler.cpp"
    #"src/test_harness_ocl_interop.cpp"
    "src/test_harness_driver_info.cpp"
    "tools/src/test_harness_api_tracing.cpp"
    "tools/src/test_harness_metric.cpp"
    #"tools/sysman/src/test_harness_sysman_frequency.cpp"
    "tools/sysman/src/test_harness_sysman_standby.cpp"
    #"tools/sysman/src/test_harness_sysman_init.cpp"
    "tools/sysman/src/test_harness_sysman_pci.cpp"
    "tools/sysman/src/test_harness_sysman_power.cpp"
    "tools/sysman/src/test_harness_sysman_diagnostics.cpp"
    #"tools/sysman/src/test_harness_sysman_event.cpp"
    #"tools/sysman/src/test_harness_sysman_memory.cpp"
    #"tools/sysman/src/test_harness_sysman_fabric.cpp"
    "tools/sysman/src/test_harness_sysman_device.cpp"
    "tools/sysman/src/test_harness_sysman_psu.cpp"
    #"tools/sysman/src/test_harness_sysman_fan.cpp"
    "tools/sysman/src/test_harness_sysman_led.cpp"
    "tools/sysman/src/test_harness_sysman_ras.cpp"
    #"tools/sysman/src/test_harness_sysman_engine.cpp"
    "tools/sysman/src/test_harness_sysman_firmware.cpp"
    #"tools/sysman/src/test_harness_sysman_temp.cpp"
    #"tools/sysman/src/test_harness_sysman_overclocking.cpp"
    "tools/sysman/src/test_harness_sysman_scheduler.cpp"

)
target_link_libraries(test_harness
    PUBLIC
    GTest::GTest
    level_zero_tests::image
    level_zero_tests::logging
    level_zero_tests::utils
    LevelZero::LevelZero
)
target_include_directories(test_harness
    PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

