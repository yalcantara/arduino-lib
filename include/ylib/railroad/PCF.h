//
// Created by Yaison on 25/10/25.
//

#pragma once

#include <Arduino.h>
#include <Wire.h>

namespace ylib::railroad {
/**
 * Read and write one 8-bit PCF-style I2C input/output expander.
 *
 * The application must start Wire before calling setup(). setup() writes 0xFF
 * so all 8 pins are high and can act as inputs on a PCF8574-style device.
 */
class PCF {
  const int _addr;

  public:
    explicit PCF(const int addr): _addr(addr) {
    }

    // Copying is intentionally disabled for this hardware wrapper.
    // Declaring these copy operations also prevents automatic move operations.
    //=========================================================================
    // Copy constructor
    PCF(const PCF &other) = delete;

    // Copy assignment
    PCF &operator=(const PCF &other) = delete;
    //=========================================================================

    void setup() const {
      Wire.beginTransmission(_addr);
      Wire.write(0xFF); // Set all 8 pins high so they can act as inputs.
      Wire.endTransmission();
    }

    // Read all 8 pins. Return 0xFF when no byte is available.
    int read() const {
      Wire.requestFrom(_addr, 1);
      if (Wire.available()) {
        return Wire.read();
      }
      return 0xFF; // Treat a failed read as all pins high.
    }

    void write(const uint8_t b) const {
      Wire.beginTransmission(_addr);
      Wire.write(b);
      Wire.endTransmission();
    }

    // Change one bit in a caller-provided 8-bit state and write the new state.
    void write(const int state, const int pin, const int val) const {
      int modifiedPcf = state;
      bitWrite(modifiedPcf, pin, val);
      write(modifiedPcf);
    }
};
} // namespace ylib::railroad
