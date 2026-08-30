//
// Created by Yaison on 4/5/25.
//
#pragma once



#include <Arduino.h>

#include <Servo.h>

namespace ylib::railroad {
// Program-wide option that applications can set for turnout movement.
// The current move() method still writes the target angle immediately.
bool __YLIB_TURNOUT_SLOW_MOVE_ENABLED = true;

enum TurnoutType {
  LEFT,
  RIGHT,
  WYE
};

/**
 * Control one model-railroad turnout with a servo.
 *
 * The object stores the servo angles for the left and right routes. The
 * turnout type decides which angle means straight. A WYE currently follows
 * the same mapping as a right-hand turnout.
 */
class Turnout {
  Servo _servo;
  const String _label;
  const int _pin;
  const TurnoutType _type;
  const int _leftDegree;
  const int _rightDegree;
  /*
   * Move the servo slowly from one position to another.
   * This is useful for simulating a slow turnout movement.
   * The delay can be adjusted to make it slower or faster.
   */
  /* // Not used in this version, as delay is not recommended in the main loop.
  void slowMove(const int from, const int to) {
    if (__YLIB_TURNOUT_SLOW_MOVE_ENABLED) {
      const int delta = abs(to - from);
      const int step = (to - from) / delta;
      int x = from;
      for (int i = 1; i <= delta; i++) {
        x += step;
        _servo.write(x);
        delay(5);
      }
    } else {
      _servo.write(to);
    }
  }*/

  // Send one target angle directly to the attached servo.
  void move(const int to) {
    _servo.write(to);
  }

  public:
    Turnout(const String label,
            const uint8_t pin,
            const TurnoutType type,
            const int left,
            const int right): _label(label),
                              _pin(pin),
                              _type(type), _leftDegree(left), _rightDegree(right) {
    }

    // Attach the servo and move it to its initial route.
    void setup(bool straight = true) {
      _servo.attach(_pin);
      if (straight) {
        this->straight();
      } else {
        this->turn();
      }
    }

    // Move to the angle that represents the straight route for this type.
    void straight() {
      switch (_type) {
        case LEFT:
          move(_rightDegree);
          break;
        case RIGHT:
        case WYE: // WYE is treated as RIGHT
          move(_leftDegree);
          break;
      }
    }

    // Move to the diverging route.
    void turn() {
      if (_type == LEFT) {
        move(_leftDegree);
      } else {
        move(_rightDegree);
      }
    }

    // Select the diverging route for non-zero, or straight for zero.
    void operate(const int value) {
      if (value) {
        turn();
      } else {
        straight();
      }
    }

    // Apply the inverse mapping of operate().
    void operateInv(const int value) {
      if (value) {
        straight();
      } else {
        turn();
      }
    }

    String label() const {
      return _label;
    }

    // Return the last angle given to the Servo library.
    int pos() {
      return _servo.read();
    }
};
} // namespace ylib::railroad
