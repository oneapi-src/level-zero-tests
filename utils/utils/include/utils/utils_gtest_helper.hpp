/*
 *
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <gtest/gtest.h>
#include <level_zero/ze_api.h>

class LztGtestSkipExectuionException : public std::runtime_error {
public:
  explicit LztGtestSkipExectuionException(const std::string &message)
      : std::runtime_error(message) {}
};
namespace {
template <size_t N> constexpr size_t string_length(const char (&)[N]) {
  return N - 1;
}
} // namespace
#define ASSERT_ZE_RESULT_SUCCESS(val)                                          \
  {                                                                            \
    const auto lzt_assert_ze_result_success_macro = val;                       \
    if (lzt_assert_ze_result_success_macro ==                                  \
        ZE_RESULT_ERROR_UNSUPPORTED_VERSION) {                                 \
      throw LztGtestSkipExectuionException("Unsupported API version");         \
    }                                                                          \
    ASSERT_EQ(ZE_RESULT_SUCCESS, lzt_assert_ze_result_success_macro);          \
  }

#define EXPECT_ZE_RESULT_SUCCESS(val)                                          \
  {                                                                            \
    const auto lzt_expect_ze_result_success_macro = val;                       \
    if (lzt_expect_ze_result_success_macro ==                                  \
        ZE_RESULT_ERROR_UNSUPPORTED_VERSION) {                                 \
      throw LztGtestSkipExectuionException("Unsupported API version");         \
    }                                                                          \
    EXPECT_EQ(ZE_RESULT_SUCCESS, lzt_expect_ze_result_success_macro);          \
  }

#define LZT_NAME_STATIC_VALIDATION(test_suite_name, test_name)                 \
  static_assert(sizeof(GTEST_STRINGIFY_(test_suite_name)) > 1,                 \
                "test_suite_name must not be empty");                          \
  static_assert(sizeof(GTEST_STRINGIFY_(test_name)) > 1,                       \
                "test_name must not be empty");                                \
  static_assert(string_length(GTEST_STRINGIFY_(                                \
                    L0_CTS_##test_suite_name##_##test_name)) < 250,            \
                "test suite with test name exceeds the maximum length of 250 " \
                "characters.");

#define TEST(test_suite_name, test_name)                                       \
  static_assert(false, "Use LZT_TEST instead of TEST");                        \
  void LZT_BODY_##test_suite_name##_##test_name()
#define TEST_F(test_suite_name, test_name)                                     \
  static_assert(false, "Use LZT_TEST_F instead of TEST_F");                    \
  void LZT_BODY_##test_suite_name##_##test_name()
#ifdef TEST_P
#undef TEST_P
#endif
#define TEST_P(test_suite_name, test_name)                                     \
  static_assert(false, "Use LZT_TEST_P instead of TEST_P");                    \
  void LZT_BODY_##test_suite_name##_##test_name()

#define LZT_TEST(test_suite_name, test_name)                                   \
  LZT_NAME_STATIC_VALIDATION(test_suite_name, test_name)                       \
  void LZT_BODY_##test_suite_name##_##test_name();                             \
  GTEST_TEST(test_suite_name, test_name) {                                     \
    try {                                                                      \
      LZT_BODY_##test_suite_name##_##test_name();                              \
    } catch (const LztGtestSkipExectuionException &e) {                        \
      GTEST_SKIP() << e.what();                                                \
    }                                                                          \
  }                                                                            \
  void LZT_BODY_##test_suite_name##_##test_name()

#define LZT_TEST_P(test_suite_name, test_name)                                 \
  LZT_NAME_STATIC_VALIDATION(test_suite_name, test_name)                       \
  class GTEST_TEST_CLASS_NAME_(test_suite_name, test_name)                     \
      : public test_suite_name {                                               \
  public:                                                                      \
    GTEST_TEST_CLASS_NAME_(test_suite_name, test_name)() {}                    \
    void TestBody() override;                                                  \
    void LztTestBodyHelper();                                                  \
                                                                               \
  private:                                                                     \
    static int AddToRegistry() {                                               \
      ::testing::UnitTest::GetInstance()                                       \
          ->parameterized_test_registry()                                      \
          .GetTestSuitePatternHolder<test_suite_name>(                         \
              GTEST_STRINGIFY_(test_suite_name),                               \
              ::testing::internal::CodeLocation(__FILE__, __LINE__))           \
          ->AddTestPattern(                                                    \
              GTEST_STRINGIFY_(test_suite_name), GTEST_STRINGIFY_(test_name),  \
              new ::testing::internal::TestMetaFactory<GTEST_TEST_CLASS_NAME_( \
                  test_suite_name, test_name)>(),                              \
              ::testing::internal::CodeLocation(__FILE__, __LINE__));          \
      return 0;                                                                \
    }                                                                          \
    [[maybe_unused]] static int gtest_registering_dummy_;                      \
  };                                                                           \
  int GTEST_TEST_CLASS_NAME_(test_suite_name,                                  \
                             test_name)::gtest_registering_dummy_ =            \
      GTEST_TEST_CLASS_NAME_(test_suite_name, test_name)::AddToRegistry();     \
  void GTEST_TEST_CLASS_NAME_(test_suite_name, test_name)::TestBody() {        \
    try {                                                                      \
      LztTestBodyHelper();                                                     \
    } catch (const LztGtestSkipExectuionException &e) {                        \
      GTEST_SKIP() << e.what();                                                \
    }                                                                          \
  }                                                                            \
  void GTEST_TEST_CLASS_NAME_(test_suite_name, test_name)::LztTestBodyHelper()

#define LZT_TEST_F_(test_suite_name, test_name)                                \
  LZT_NAME_STATIC_VALIDATION(test_suite_name, test_name)                       \
  class GTEST_TEST_CLASS_NAME_(test_suite_name, test_name)                     \
      : public test_suite_name {                                               \
  public:                                                                      \
    GTEST_TEST_CLASS_NAME_(test_suite_name, test_name)() = default;            \
    ~GTEST_TEST_CLASS_NAME_(test_suite_name, test_name)() override = default;  \
    GTEST_TEST_CLASS_NAME_(test_suite_name, test_name)                         \
    (const GTEST_TEST_CLASS_NAME_(test_suite_name, test_name) &) = delete;     \
    GTEST_TEST_CLASS_NAME_(test_suite_name, test_name) &                       \
    operator=(const GTEST_TEST_CLASS_NAME_(test_suite_name, test_name) &) =    \
        delete; /* NOLINT */                                                   \
    GTEST_TEST_CLASS_NAME_(test_suite_name, test_name)                         \
    (GTEST_TEST_CLASS_NAME_(test_suite_name, test_name) &&) noexcept = delete; \
    GTEST_TEST_CLASS_NAME_(test_suite_name, test_name) &operator=(             \
        GTEST_TEST_CLASS_NAME_(test_suite_name,                                \
                               test_name) &&) noexcept = delete; /* NOLINT */  \
                                                                               \
  private:                                                                     \
    void TestBody() override;                                                  \
    void LztTestBodyHelper();                                                  \
    [[maybe_unused]] static ::testing::TestInfo *const test_info_;             \
  };                                                                           \
                                                                               \
  ::testing::TestInfo *const GTEST_TEST_CLASS_NAME_(test_suite_name,           \
                                                    test_name)::test_info_ =   \
      ::testing::internal::MakeAndRegisterTestInfo(                            \
          #test_suite_name, #test_name, nullptr, nullptr,                      \
          ::testing::internal::CodeLocation(__FILE__, __LINE__),               \
          (::testing::internal::GetTypeId<test_suite_name>()),                 \
          ::testing::internal::SuiteApiResolver<                               \
              test_suite_name>::GetSetUpCaseOrSuite(__FILE__, __LINE__),       \
          ::testing::internal::SuiteApiResolver<                               \
              test_suite_name>::GetTearDownCaseOrSuite(__FILE__, __LINE__),    \
          new ::testing::internal::TestFactoryImpl<GTEST_TEST_CLASS_NAME_(     \
              test_suite_name, test_name)>);                                   \
  void GTEST_TEST_CLASS_NAME_(test_suite_name, test_name)::TestBody() {        \
    try {                                                                      \
      LztTestBodyHelper();                                                     \
    } catch (const LztGtestSkipExectuionException &e) {                        \
      GTEST_SKIP() << e.what();                                                \
    }                                                                          \
  }                                                                            \
  void GTEST_TEST_CLASS_NAME_(test_suite_name, test_name)::LztTestBodyHelper()

#define LZT_TEST_F(test_suite_name, test_name)                                 \
  LZT_TEST_F_(test_suite_name, test_name)
