#include <ylib/core/core.h>

#include <unity.h>

using ylib::core::SMA;
using ylib::core::padWithSpaces;

void test_pad_with_spaces_handles_any_value_width() {
  TEST_ASSERT_EQUAL_STRING("   42", padWithSpaces(5, 42).c_str());
  TEST_ASSERT_EQUAL_STRING("-1234", padWithSpaces(5, -1234).c_str());
  TEST_ASSERT_EQUAL_STRING("12345", padWithSpaces(2, 12345).c_str());
}

void test_sma_averages_only_available_recent_values() {
  SMA<int> average(4);

  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, average.getAvgOfLast(4));

  average.next(10);
  average.next(20);

  TEST_ASSERT_FLOAT_WITHIN(0.001f, 15.0f, average.getAvgOfLast(4));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 20.0f, average.getAvgOfLast(1));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, average.getAvgOfLast(0));
  TEST_ASSERT_FALSE(average.isFilled());

  average.next(30);
  average.next(40);

  TEST_ASSERT_FLOAT_WITHIN(0.001f, 35.0f, average.getAvgOfLast(2));
  TEST_ASSERT_TRUE(average.isFilled());
}
