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
    "tools/src/test_harness_api_ltracing.cpp"
    "tools/src/test_harness_debug.cpp"
    "tools/src/test_harness_metric.cpp"
    "sysman/src/test_harness_sysman_frequency.cpp"
    "sysman/src/test_harness_sysman_standby.cpp"
    "sysman/src/test_harness_sysman_pci.cpp"
    "sysman/src/test_harness_sysman_power.cpp"
    "sysman/src/test_harness_sysman_diagnostics.cpp"
    "sysman/src/test_harness_sysman_event.cpp"
    "sysman/src/test_harness_sysman_memory.cpp"
    "sysman/src/test_harness_sysman_fabric.cpp"
    "sysman/src/test_harness_sysman_device.cpp"
    "sysman/src/test_harness_sysman_psu.cpp"
    "sysman/src/test_harness_sysman_fan.cpp"
    "sysman/src/test_harness_sysman_led.cpp"
    "sysman/src/test_harness_sysman_ras.cpp"
    "sysman/src/test_harness_sysman_engine.cpp"
    "sysman/src/test_harness_sysman_firmware.cpp"
    "sysman/src/test_harness_sysman_temp.cpp"
    "sysman/src/test_harness_sysman_overclocking.cpp"
    "sysman/src/test_harness_sysman_scheduler.cpp"
    "sysman/src/test_harness_sysman_performance.cpp"

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

