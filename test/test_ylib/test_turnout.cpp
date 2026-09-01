#include <Arduino.h>
#include <unity.h>

namespace turnout_test {
unsigned long fakeMillisValue = 0;
uint8_t attachResult = 1;
int attachedPin = -1;
int lastWrittenAngle = -1;
uint16_t writeCount = 0;

unsigned long readFakeMillis() {
  return fakeMillisValue;
}

void resetFakes() {
  fakeMillisValue = 1000;
  attachResult = 1;
  attachedPin = -1;
  lastWrittenAngle = -1;
  writeCount = 0;
}
} // namespace turnout_test

class TurnoutTestServo {
  public:
    uint8_t attach(const int pin) {
      turnout_test::attachedPin = pin;
      return turnout_test::attachResult;
    }

    void write(const int angle) {
      turnout_test::lastWrittenAngle = angle;
      ++turnout_test::writeCount;
    }
};

// Replace the Servo class and clock only while compiling Turnout for this test.
#define Servo_h
#define Servo TurnoutTestServo
#define millis turnout_test::readFakeMillis
#include <ylib/railroad/Turnout.h>
#undef millis
#undef Servo
#undef Servo_h

using ylib::railroad::IN_PROGRESS;
using ylib::railroad::LEFT;
using ylib::railroad::RIGHT;
using ylib::railroad::STRAIGHT;
using ylib::railroad::TURNING;
using ylib::railroad::Turnout;

// Check that setup moves each turnout type to its straight route.
// The turnout stays busy for a short time, then reports that it is straight.
void test_turnout_setup_establishes_state_for_each_type() {
  turnout_test::resetFakes();
  Turnout::SLOW_MOVE = true;

  Turnout rightTurnout("right", 4, RIGHT, 94, 73);
  TEST_ASSERT_TRUE(rightTurnout.setup());
  TEST_ASSERT_EQUAL_INT(4, turnout_test::attachedPin);
  TEST_ASSERT_EQUAL_INT(73, turnout_test::lastWrittenAngle);
  TEST_ASSERT_EQUAL_INT(73, rightTurnout.pos());
  TEST_ASSERT_EQUAL_INT(IN_PROGRESS, rightTurnout.state());

  Turnout leftTurnout("left", 5, LEFT, 94, 73);
  TEST_ASSERT_TRUE(leftTurnout.setup());
  TEST_ASSERT_EQUAL_INT(94, leftTurnout.pos());
  TEST_ASSERT_EQUAL_INT(IN_PROGRESS, leftTurnout.state());

  turnout_test::fakeMillisValue += Turnout::MINIMUM_IN_PROGRESS_TIME;
  TEST_ASSERT_EQUAL_INT(STRAIGHT, rightTurnout.state());
  TEST_ASSERT_EQUAL_INT(STRAIGHT, leftTurnout.state());
}

// Check that slow movement changes the servo angle one degree at a time.
// After reaching the target, the turnout waits for the servo to settle.
void test_turnout_moves_slowly_and_waits_for_servo_to_settle() {
  turnout_test::resetFakes();
  Turnout::SLOW_MOVE = true;

  Turnout turnout("slow", 4, RIGHT, 94, 73);
  TEST_ASSERT_TRUE(turnout.setup());
  turnout_test::fakeMillisValue += Turnout::MINIMUM_IN_PROGRESS_TIME;
  TEST_ASSERT_EQUAL_INT(STRAIGHT, turnout.state());

  TEST_ASSERT_TRUE(turnout.turn());
  TEST_ASSERT_EQUAL_INT(73, turnout.pos());
  TEST_ASSERT_EQUAL_INT(IN_PROGRESS, turnout.state());

  for (int expected = 74; expected <= 94; ++expected) {
    turnout.loop();
    TEST_ASSERT_EQUAL_INT(expected, turnout.pos());
    TEST_ASSERT_EQUAL_INT(expected, turnout_test::lastWrittenAngle);
  }

  TEST_ASSERT_EQUAL_INT(IN_PROGRESS, turnout.state());
  turnout_test::fakeMillisValue += Turnout::MINIMUM_IN_PROGRESS_TIME - 1;
  TEST_ASSERT_EQUAL_INT(IN_PROGRESS, turnout.state());
  ++turnout_test::fakeMillisValue;
  TEST_ASSERT_EQUAL_INT(TURNING, turnout.state());
}

// Check that normal, inverse, and invert commands choose the correct route.
// A new command is rejected while the previous movement is still settling.
void test_turnout_switching_operations_select_expected_routes() {
  turnout_test::resetFakes();
  Turnout::SLOW_MOVE = false;

  Turnout turnout("switching", 4, RIGHT, 94, 73);
  TEST_ASSERT_TRUE(turnout.setup());
  turnout_test::fakeMillisValue += Turnout::MINIMUM_IN_PROGRESS_TIME;

  TEST_ASSERT_TRUE(turnout.operate(1));
  TEST_ASSERT_EQUAL_INT(94, turnout.pos());
  TEST_ASSERT_FALSE(turnout.operate(0));
  turnout_test::fakeMillisValue += Turnout::MINIMUM_IN_PROGRESS_TIME;
  TEST_ASSERT_EQUAL_INT(TURNING, turnout.state());

  TEST_ASSERT_TRUE(turnout.operate(0));
  TEST_ASSERT_EQUAL_INT(73, turnout.pos());
  turnout_test::fakeMillisValue += Turnout::MINIMUM_IN_PROGRESS_TIME;
  TEST_ASSERT_EQUAL_INT(STRAIGHT, turnout.state());

  TEST_ASSERT_TRUE(turnout.operateInv(0));
  TEST_ASSERT_EQUAL_INT(94, turnout.pos());
  turnout_test::fakeMillisValue += Turnout::MINIMUM_IN_PROGRESS_TIME;
  TEST_ASSERT_EQUAL_INT(TURNING, turnout.state());

  TEST_ASSERT_TRUE(turnout.operateInv(1));
  TEST_ASSERT_EQUAL_INT(73, turnout.pos());
  turnout_test::fakeMillisValue += Turnout::MINIMUM_IN_PROGRESS_TIME;
  TEST_ASSERT_TRUE(turnout.invert());
  TEST_ASSERT_EQUAL_INT(94, turnout.pos());
}
