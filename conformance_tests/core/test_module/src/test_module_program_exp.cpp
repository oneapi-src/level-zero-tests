/*
 *
 * Copyright (C) 2021-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "gtest/gtest.h"
#include "utils/utils.hpp"
#include "test_harness/test_harness.hpp"
#include "random/random.hpp"
#include <stdlib.h>
#include <level_zero/ze_api.h>

namespace lzt = level_zero_tests;

#ifdef ZE_MODULE_PROGRAM_EXP_NAME

namespace {

bool check_ext_version() {
  auto ext_props = lzt::get_extension_properties(lzt::get_default_driver());
  uint32_t ext_version = 0;
  for (auto prop : ext_props) {
    if (strncmp(prop.name, ZE_MODULE_PROGRAM_EXP_NAME, ZE_MAX_EXTENSION_NAME) ==
        0) {
      ext_version = prop.version;
      break;
    }
  }
  if (ext_version == 0) {
    printf("ZE_MODULE_PROGRAM_EXP EXT not found, not running test\n");
    return false;
  } else {
    printf("Extension version %d found\n", ext_version);
    return true;
  }
}

void RunGivenModulesWithLinkageDependenciesWhenCreatingTest(bool is_immediate) {
  auto driver = lzt::get_default_driver();
  auto context = lzt::create_context(driver);
  auto device = lzt::get_default_device(driver);
  auto cmd_bundle = lzt::create_command_bundle(
      context, device, 0, ZE_COMMAND_QUEUE_MODE_DEFAULT,
      ZE_COMMAND_QUEUE_PRIORITY_NORMAL, 0, 0, 0, is_immediate);
  void *result_buffer =
      lzt::allocate_shared_memory(sizeof(int), 1, 0, 0, device, context);

  std::vector<lzt::module_program_modules_t> in_modules;
  lzt::module_program_modules_t in_module;
  in_module.filename = "import_kernel.spv";
  in_module.pBuildFlags = nullptr;
  in_module.pConstants = nullptr;
  in_modules.push_back(in_module);
  in_module.filename = "export_kernel.spv";
  in_module.pBuildFlags = nullptr;
  in_module.pConstants = nullptr;
  in_modules.push_back(in_module);
  auto out_module = lzt::create_program_module(context, device, in_modules);
  auto kernel = lzt::create_function(out_module, "import_function");

  lzt::set_group_size(kernel, 1, 1, 1);

  int x = lzt::generate_value<int>();
  int y = lzt::generate_value<int>();
  lzt::set_argument_value(kernel, 0, sizeof(int), &x);
  lzt::set_argument_value(kernel, 1, sizeof(int), &y);
  lzt::set_argument_value(kernel, 2, sizeof(result_buffer), &result_buffer);

  ze_group_count_t group_count = {1, 1, 1};

  lzt::append_launch_function(cmd_bundle.list, kernel, &group_count, nullptr, 0,
                              nullptr);
  lzt::close_command_list(cmd_bundle.list);
  lzt::execute_and_sync_command_bundle(cmd_bundle, UINT64_MAX);

  int expectedResult = (x + y);

  ASSERT_EQ(expectedResult, *(int *)result_buffer);

  // Cleanup
  lzt::free_memory(context, result_buffer);
  lzt::destroy_command_bundle(cmd_bundle);
  lzt::destroy_function(kernel);
  lzt::destroy_module(out_module);
  lzt::destroy_context(context);
}

LZT_TEST(zeModuleProgramTests,
         GivenModulesWithLinkageDependenciesWhenCreatingThenSuccessIsReturned) {
  if (!check_ext_version())
    GTEST_SKIP();
  RunGivenModulesWithLinkageDependenciesWhenCreatingTest(false);
}

LZT_TEST(
    zeModuleProgramTests,
    GivenModulesWithLinkageDependenciesWhenCreatingOnImmediateCmdListThenSuccessIsReturned) {
  if (!check_ext_version())
    GTEST_SKIP();
  RunGivenModulesWithLinkageDependenciesWhenCreatingTest(true);
}

} // namespace

#else
#warning "ZE_MODULE_PROGRAM_EXP support not found, not building tests for it"
#endif // #ifdef ZE_MODULE_PROGRAM_EXP_VERSION_CURRENT
