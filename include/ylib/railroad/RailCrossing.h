#pragma once
//
// Created by Yaison on 27/11/25.
//


#include <Arduino.h>


class RailCrossing {
  const String _label;
  const int _pin;
  const int _aliveDuration; // in seconds
  unsigned long _previousMillis;

  public:
    RailCrossing(const String label,
                 const int pin,
                 const unsigned int
                 aliveDuration): _label(label),
                                 _pin(pin), _aliveDuration(aliveDuration) {
    }

    void setup() {
      pinMode(_pin, OUTPUT);
      _previousMillis = millis();
    }

    void loop() const {
      unsigned long elapsedMillis = millis() - _previousMillis;
      if (elapsedMillis > _aliveDuration * 1000) {
        digitalWrite(_pin, LOW);
      }
    }

    void activate() {
      digitalWrite(_pin, HIGH);
      _previousMillis = millis();
    }
};
