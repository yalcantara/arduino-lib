#include <Arduino.h>
#include <unity.h>

namespace {
unsigned long fakeMillisValue = 0;
int configuredPin = -1;
int configuredMode = -1;
int writtenPin = -1;
int writtenValue = -1;
uint8_t writeCount = 0;

unsigned long readFakeMillis() {
  return fakeMillisValue;
}

void recordPinMode(const uint8_t pin, const uint8_t mode) {
  configuredPin = pin;
  configuredMode = mode;
}

void recordDigitalWrite(const uint8_t pin, const uint8_t value) {
  writtenPin = pin;
  writtenValue = value;
  ++writeCount;
}

void resetRailCrossingFakes() {
  fakeMillisValue = 0;
  configuredPin = -1;
  configuredMode = -1;
  writtenPin = -1;
  writtenValue = -1;
  writeCount = 0;
}
} // namespace

// Redirect this header's hardware calls to deterministic test functions.
#define millis readFakeMillis
#define pinMode recordPinMode
#define digitalWrite recordDigitalWrite
#include <ylib/railroad/RailCrossing.h>
#undef digitalWrite
#undef pinMode
#undef millis

using ylib::railroad::RailCrossing;

void test_rail_crossing_turns_off_at_the_configured_time() {
  constexpr uint8_t pin = 7;
  resetRailCrossingFakes();
  fakeMillisValue = 100;
  RailCrossing crossing("crossing", pin, 2);

  crossing.setup();
  TEST_ASSERT_EQUAL_INT(pin, configuredPin);
  TEST_ASSERT_EQUAL_INT(OUTPUT, configuredMode);

  crossing.activate();
  TEST_ASSERT_EQUAL_INT(pin, writtenPin);
  TEST_ASSERT_EQUAL_INT(HIGH, writtenValue);
  TEST_ASSERT_EQUAL_UINT8(1, writeCount);

  fakeMillisValue = 2099;
  crossing.loop();
  TEST_ASSERT_EQUAL_UINT8(1, writeCount);

  fakeMillisValue = 2100;
  crossing.loop();
  TEST_ASSERT_EQUAL_INT(LOW, writtenValue);
  TEST_ASSERT_EQUAL_UINT8(2, writeCount);
}

void test_rail_crossing_timer_survives_millis_rollover() {
  resetRailCrossingFakes();
  fakeMillisValue = ~0UL - 499UL;
  RailCrossing crossing("crossing", 7, 1);
  crossing.setup();
  crossing.activate();

  fakeMillisValue = 500;
  crossing.loop();

  TEST_ASSERT_EQUAL_INT(LOW, writtenValue);
  TEST_ASSERT_EQUAL_UINT8(2, writeCount);
}
