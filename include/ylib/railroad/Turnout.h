//
// Created by Yaison on 4/5/25.
//
#pragma once


#include <Arduino.h>
#include <Servo.h>

#include <ylib/core/core.h>

using ylib::core::checkArgument;

namespace ylib::railroad {
enum TurnoutType {
  LEFT,
  RIGHT,
  WYE
};

enum TurnoutState {
  STRAIGHT,
  IN_PROGRESS,
  TURNING
};

/**
 * Control one model-railroad turnout with a servo.
 *
 * The object stores the servo angles for the left and right routes. The
 * turnout type decides which angle means straight. A WYE currently follows
 * the same mapping as a right-hand turnout.
 */
class Turnout {
  static constexpr int STEP_SIZE = 1;

  Servo _servo;

  const String _label;
  const int _pin;
  const TurnoutType _type;
  const int _leftDegree;
  const int _rightDegree;

  unsigned long _lastMoveTime;
  int _crtPos;
  int _desiredPos;

  // Send one target angle directly to the attached servo.
  void move(const int to) {
    // _rightDegree < _leftDegree
    const int target = constrain(to, _rightDegree, _leftDegree);
    _servo.write(target);
    _crtPos = target;
    _lastMoveTime = millis();
  }

  bool step() {
    const int target = _desiredPos;

    const int crtPost = pos();
    if (crtPost == _desiredPos) {
      return true;
    }

    const int direction = (target - crtPost > 0) ? 1 : -1;
    const int newPos = crtPost + STEP_SIZE * direction;

    move(crtPost + STEP_SIZE * direction);
    return newPos == target;
  }

  void operateMove(const int pos, bool slow) {
    if (slow) {
      _desiredPos = pos;
      // Just set desired position. Let the loop function do it's work
    } else {
      _desiredPos = pos;
      move(pos);
    }
  }

  bool _straight(const bool checkInProgress, const bool slow) {
    if (checkInProgress && isInProgress()) {
      return false;
    }

    switch (_type) {
      case LEFT:
        operateMove(_leftDegree, slow);
        break;
      case RIGHT:
      case WYE: // WYE is treated as RIGHT
        operateMove(_rightDegree, slow);
        break;
    }

    return true;
  }

  bool _turn(const bool checkInProgress, const bool slow) {
    if (checkInProgress && isInProgress()) {
      return false;
    }

    if (_type == LEFT) {
      operateMove(_rightDegree, slow);
    } else {
      operateMove(_leftDegree, slow);
    }

    return true;
  }

  public:
    // Program-wide option that applications can set for turnout movement.
    // The current move() method still writes the target angle immediately.
    inline static bool SLOW_MOVE = true;

    static constexpr int MINIMUM_IN_PROGRESS_TIME = 500;

    Turnout(const String &label,
            const uint8_t pin,
            const TurnoutType type,
            const int left,
            const int right): _label(label),
                              _pin(pin),
                              _type(type),
                              _leftDegree(left),
                              _rightDegree(right),
                              _lastMoveTime(0),
                              _crtPos(_rightDegree),
                              _desiredPos(_rightDegree) {
    }

    // Attach the servo and move it to its initial route.
    [[nodiscard]] bool setup(const bool straight = true) {
      // min of 10 degree between ends
      const bool valid =
          checkArgument(10, 180, _leftDegree) &&
          checkArgument(0, 170, _rightDegree) &&
          checkArgument(0, _leftDegree - 10, _rightDegree,
            "The `left` degree should be at least 10 more than `right`.");

      if (!valid) {
        return false;
      }

      if (!_servo.attach(_pin)) {
        return false;
      }

      // During setup() we don't do slow move
      if (straight) {
        return this->_straight(false, false);
      }

      return this->_turn(false, false);
    }

    void loop() {
      // Loops is only for slow move
      if (SLOW_MOVE) {
        step();
      }
    }

    [[nodiscard]] bool isInProgress() const {
      if (millis() - _lastMoveTime < MINIMUM_IN_PROGRESS_TIME) {
        return true;
      }
      return pos() != _desiredPos;
    }

    bool invert() {
      switch (state()) {
        case STRAIGHT:
          return turn();
        case TURNING:
          return straight();
        default:
          return false;
      }
    }

    // Move to the angle that represents the straight route for this type.
    bool straight() {
      return _straight(true, SLOW_MOVE);
    }

    // Move to the diverging route.
    bool turn() {
      return _turn(true, SLOW_MOVE);
    }

    // Select the diverging route for non-zero, or straight for zero.
    // Convenient method to work with PCF [0, 1] values.
    bool operate(const int value) {
      if (value) {
        return turn();
      }

      return straight();
    }

    // Apply the inverse mapping of operate().
    bool operateInv(const int value) {
      if (value) {
        return straight();
      }
      return turn();
    }

    [[nodiscard]] String label() const {
      return _label;
    }

    // Return the last angle given to the Servo library.
    [[nodiscard]] int pos() const {
      return _crtPos;
    }

    [[nodiscard]] TurnoutState state() const {
      if (isInProgress()) {
        return IN_PROGRESS;
      }

      switch (_type) {
        case RIGHT:
        case WYE:
          if (pos() == _rightDegree) {
            return STRAIGHT;
          }
          return TURNING;
        case LEFT:
          if (pos() == _leftDegree) {
            return STRAIGHT;
          }
          return TURNING;
      }
      return STRAIGHT;
    }

    void debug() const {
      Serial.print("Slow move loop: ");
      Serial.println(SLOW_MOVE);
      Serial.print("last move time: ");
      Serial.println(_lastMoveTime);
      Serial.print("Millis: ");
      Serial.println(millis());
      Serial.print("Is in progress: ");
      Serial.println(isInProgress());
      Serial.println("Desired pos: ");
      Serial.println(_desiredPos);
      Serial.println("Current pos: ");
      Serial.println(pos());
    }
};
} // namespace ylib::railroad
