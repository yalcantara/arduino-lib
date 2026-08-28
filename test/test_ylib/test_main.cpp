#include <Arduino.h>
#include <unity.h>

void test_dcsensor_uses_overridden_reading();
void test_dcsensor_calibrates_300ma_offset_to_zero();

void setUp() {}

void tearDown() {}

void setup()
{
  delay(2000);

  UNITY_BEGIN();
  RUN_TEST(test_dcsensor_uses_overridden_reading);
  RUN_TEST(test_dcsensor_calibrates_300ma_offset_to_zero);
  UNITY_END();
}

void loop() {}
