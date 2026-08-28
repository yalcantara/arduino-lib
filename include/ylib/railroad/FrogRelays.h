#pragma once

//
// Created by Yaison on 26/11/25.
//



#include <Arduino.h>

enum FrogRelayState {
  POS,
  NEG,
  NEUTRAl
};

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
      // The OUTPUT state defaults to LOW
      pinMode(_pinNeg, OUTPUT);
      pinMode(_pinPos, OUTPUT);
    }

    void loop() {

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

    void operate(const int val) {
      if (val) {
        _futureState = POS;
      }else {
        _futureState = NEG;
      }
    }
};
