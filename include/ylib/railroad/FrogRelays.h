//
// Created by Yaison on 26/11/25.
//

#pragma once
#include <Arduino.h>

namespace ylib::railroad {
enum FrogRelayState {
  POS,
  NEG,
  NEUTRAl
};

/**
 * Control the two relays that set railroad frog polarity.
 *
 * operate() requests positive or negative polarity. loop() performs the
 * change using a break-before-make sequence: both relays are turned off for
 * one loop pass before the requested relay is turned on. This helps prevent a
 * short circuit while polarity changes.
 */
class FrogRelays {
  const String _label;
  const int _pinPos;
  const int _pinNeg;

  FrogRelayState _state = NEUTRAl;
  FrogRelayState _futureState = NEUTRAl;

  public:
    FrogRelays(const String label, const int pinPos, const int pinNeg): _label(label),
      _pinPos(pinPos), _pinNeg(pinNeg) {
    }

    void setup() const {
      // Set the output values first so neither relay is briefly energized.
      digitalWrite(_pinNeg, LOW);
      digitalWrite(_pinPos, LOW);
      pinMode(_pinNeg, OUTPUT);
      pinMode(_pinPos, OUTPUT);
    }

    void loop() {
      // A polarity change takes two passes: neutral first, target second.
      if (_state != _futureState) {
        if (_state == NEUTRAl) {
          if (_futureState == POS) {
            digitalWrite(_pinPos, HIGH);
          } else if (_futureState == NEG) {
            digitalWrite(_pinNeg, HIGH);
          }
          _state = _futureState;
        } else {
          _state = NEUTRAl;
          digitalWrite(_pinPos, LOW);
          digitalWrite(_pinNeg, LOW);
        }
      }
    }

    // Request positive polarity for a non-zero value, or negative for zero.
    void operate(const int val) {
      if (val) {
        _futureState = POS;
      }else {
        _futureState = NEG;
      }
    }
};
} // namespace ylib::railroad
