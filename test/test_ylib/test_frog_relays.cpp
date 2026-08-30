#include <Arduino.h>
#include <unity.h>

namespace {
enum class PinCallType : uint8_t {
  MODE,
  WRITE
};

struct PinCall {
  PinCallType type;
  uint8_t pin;
  uint8_t value;
};

PinCall pinCalls[8] = {};
uint8_t pinCallCount = 0;

void recordFrogPinMode(const uint8_t pin, const uint8_t mode) {
  pinCalls[pinCallCount++] = {PinCallType::MODE, pin, mode};
}

void recordFrogDigitalWrite(const uint8_t pin, const uint8_t value) {
  pinCalls[pinCallCount++] = {PinCallType::WRITE, pin, value};
}

void resetFrogPinCalls() {
  pinCallCount = 0;
}
} // namespace

// Redirect this header's pin operations so no physical relay is energized.
#define pinMode recordFrogPinMode
#define digitalWrite recordFrogDigitalWrite
#include <ylib/railroad/FrogRelays.h>
#undef digitalWrite
#undef pinMode

using ylib::railroad::FrogRelays;

void test_frog_relays_setup_forces_both_relays_off() {
  constexpr uint8_t positivePin = 2;
  constexpr uint8_t negativePin = 3;
  resetFrogPinCalls();
  FrogRelays relays("frog", positivePin, negativePin);

  relays.setup();

  TEST_ASSERT_EQUAL_UINT8(4, pinCallCount);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(PinCallType::WRITE),
                        static_cast<int>(pinCalls[0].type));
  TEST_ASSERT_EQUAL_INT(negativePin, pinCalls[0].pin);
  TEST_ASSERT_EQUAL_INT(LOW, pinCalls[0].value);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(PinCallType::WRITE),
                        static_cast<int>(pinCalls[1].type));
  TEST_ASSERT_EQUAL_INT(positivePin, pinCalls[1].pin);
  TEST_ASSERT_EQUAL_INT(LOW, pinCalls[1].value);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(PinCallType::MODE),
                        static_cast<int>(pinCalls[2].type));
  TEST_ASSERT_EQUAL_INT(negativePin, pinCalls[2].pin);
  TEST_ASSERT_EQUAL_INT(OUTPUT, pinCalls[2].value);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(PinCallType::MODE),
                        static_cast<int>(pinCalls[3].type));
  TEST_ASSERT_EQUAL_INT(positivePin, pinCalls[3].pin);
  TEST_ASSERT_EQUAL_INT(OUTPUT, pinCalls[3].value);
}

void test_frog_relays_use_break_before_make() {
  constexpr uint8_t positivePin = 2;
  constexpr uint8_t negativePin = 3;
  FrogRelays relays("frog", positivePin, negativePin);

  resetFrogPinCalls();
  relays.operate(1);
  relays.loop();
  TEST_ASSERT_EQUAL_UINT8(1, pinCallCount);
  TEST_ASSERT_EQUAL_INT(positivePin, pinCalls[0].pin);
  TEST_ASSERT_EQUAL_INT(HIGH, pinCalls[0].value);

  resetFrogPinCalls();
  relays.operate(0);
  relays.loop();
  TEST_ASSERT_EQUAL_UINT8(2, pinCallCount);
  TEST_ASSERT_EQUAL_INT(positivePin, pinCalls[0].pin);
  TEST_ASSERT_EQUAL_INT(LOW, pinCalls[0].value);
  TEST_ASSERT_EQUAL_INT(negativePin, pinCalls[1].pin);
  TEST_ASSERT_EQUAL_INT(LOW, pinCalls[1].value);

  resetFrogPinCalls();
  relays.loop();
  TEST_ASSERT_EQUAL_UINT8(1, pinCallCount);
  TEST_ASSERT_EQUAL_INT(negativePin, pinCalls[0].pin);
  TEST_ASSERT_EQUAL_INT(HIGH, pinCalls[0].value);
}
