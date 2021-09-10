/*
 *
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"
#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include <stdlib.h>
#include <level_zero/ze_api.h>

namespace lzt = level_zero_tests;

#ifdef ZE_KERNEL_SCHEDULING_HINTS_EXP_NAME

namespace {

bool check_ext_version() {
  auto ext_props = lzt::get_extension_properties(lzt::get_default_driver());
  uint32_t ext_version = 0;
  for (auto prop : ext_props) {
    if (strncmp(prop.name, ZE_KERNEL_SCHEDULING_HINTS_EXP_NAME,
                ZE_MAX_EXTENSION_NAME) == 0) {
      ext_version = prop.version;
      break;
    }
  }
  if (ext_version == 0) {
    printf("ZE_KERNEL_SCHEDULING_HINTS_EXP_NAME EXT not found, not running "
           "test\n");
    return false;
  } else {
    printf("Extention version %d found\n", ext_version);
    return true;
  }
}

TEST(zeKernelScheduleHintsTests,
     GivenKernelScheduleHintWhenRunningKernelThenResultIsCorrect) {

  if (!check_ext_version())
    return;
  auto driver = lzt::get_default_driver();
  auto context = lzt::create_context(driver);
  auto device = lzt::get_default_device(driver);

  auto module = lzt::create_module(context, device, "module_add.spv",
                                   ZE_MODULE_FORMAT_IL_SPIRV, "", nullptr);
  auto kernel = lzt::create_function(module, "module_add_constant_2");

  uint32_t hints[] = {ZE_SCHEDULING_HINT_EXP_FLAG_OLDEST_FIRST,
                      ZE_SCHEDULING_HINT_EXP_FLAG_ROUND_ROBIN,
                      ZE_SCHEDULING_HINT_EXP_FLAG_STALL_BASED_ROUND_ROBIN};

  for (auto hint : hints) {
    lzt::set_kernel_scheduling_hint(kernel, hint);
    const int addval = 3;
    const size_t size = 256;
    const size_t alignment = 1;
    auto buffer =
        lzt::allocate_shared_memory(size, alignment, 0, 0, device, context);
    memset(buffer, 0, size);
    lzt::set_argument_value(kernel, 0, sizeof(buffer), &buffer);
    lzt::set_argument_value(kernel, 1, sizeof(addval), &addval);
    auto cmdlist = lzt::create_command_list(context, device, 0);
    auto cmdqueue = lzt::create_command_queue(
        context, device, 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
        ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0, 0);

    lzt::set_group_size(kernel, 256, 1, 1);
    ze_group_count_t group_count = {};
    group_count.groupCountX = 1;
    group_count.groupCountY = 1;
    group_count.groupCountZ = 1;
    lzt::append_launch_function(cmdlist, kernel, &group_count, nullptr, 0,
                                nullptr);
    lzt::close_command_list(cmdlist);
    lzt::execute_command_lists(cmdqueue, 1, &cmdlist, nullptr);
    lzt::synchronize(cmdqueue, UINT64_MAX);

    lzt::destroy_command_list(cmdlist);
    lzt::destroy_command_queue(cmdqueue);

    for (size_t i = 0; i < size; i++) {
      ASSERT_EQ(((uint8_t *)buffer)[i], addval);
    }
  }

  lzt::destroy_function(kernel);
  lzt::destroy_module(module);
  lzt::destroy_context(context);
}
} // namespace

#else
#warning                                                                       \
    "ZE_KERNEL_SCHEDULING_HINTS_EXP support not found, not building tests for it"
#endif //#ifdef ZE_KERNEL_SCHEDULING_HINTS_EXP_NAME
