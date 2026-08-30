#include <Arduino.h>
#include <unity.h>

#include <ylib/core/core.h>

using ylib::core::Timer;

void test_timer_fires_only_after_its_timeout() {
  Timer timer(10UL);

  TEST_ASSERT_FALSE(timer.next());
  delay(15);
  TEST_ASSERT_TRUE(timer.next());
  TEST_ASSERT_FALSE(timer.next());
}
