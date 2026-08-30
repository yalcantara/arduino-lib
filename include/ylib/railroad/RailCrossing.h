#pragma once
//
// Created by Yaison on 27/11/25.
//

#include <Arduino.h>

namespace ylib::railroad {
/**
 * Keep a railroad-crossing output active for a fixed number of seconds.
 *
 * activate() drives the pin high and restarts the time window. Call loop()
 * often; it drives the pin low after the configured duration without using a
 * blocking delay.
 */
class RailCrossing {
  const String _label;
  const int _pin;
  const unsigned long _aliveDurationMillis;
  unsigned long _previousMillis;

  public:
    RailCrossing(const String &label,
                 const int pin,
                 const unsigned int
                 aliveDuration): _label(label),
                                 _pin(pin),
                                 _aliveDurationMillis(
                                   static_cast<unsigned long>(aliveDuration) * 1000UL),
                                 _previousMillis(0) {
    }

    // Prepare the output pin and start the first time window.
    void setup() {
      pinMode(_pin, OUTPUT);
      _previousMillis = millis();
    }

    // Turn the output off after the active time has passed.
    void loop() const {
      unsigned long elapsedMillis = millis() - _previousMillis;
      if (elapsedMillis >= _aliveDurationMillis) {
        digitalWrite(_pin, LOW);
      }
    }

    // Turn the output on and restart its active time window.
    void activate() {
      digitalWrite(_pin, HIGH);
      _previousMillis = millis();
    }
};
} // namespace ylib::railroad
