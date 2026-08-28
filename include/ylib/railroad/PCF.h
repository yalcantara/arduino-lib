#pragma once

//
// Created by Yaison on 25/10/25.
//


class PCF {
  const int _addr;

  public:
    explicit PCF(const int addr): _addr(addr) {
    }

    //Rule of five
    //=========================================================================
    //1. Copy Constructor
    //We don't allow to be copied.
    PCF(const PCF &other) = delete;

    //2. Copy Assignment
    //We don't allow to be copied.
    PCF &operator=(const PCF &other) = delete;

    //3. Move Constructor (Allowed)
    //4. Move Assignment (Allowed)
    //5. Delete (no need to be implemented)
    //=========================================================================

    void setup() const {
      Wire.beginTransmission(_addr);
      Wire.write(0xFF); // Todos los pines en HIGH para que actúen como entradas
      Wire.endTransmission();
    }

    int read() const {
      Wire.requestFrom(_addr, 1);
      if (Wire.available()) {
        return Wire.read();
      }
      return 0xFF; // Retorna todos los pin HIGH si falla
    }

    void write(const uint8_t b) const {
      Wire.beginTransmission(_addr);
      Wire.write(b);
      Wire.endTransmission();
    }

    void write(const int state, const int pin, const int val) const {
      int modifiedPcf = state;
      bitWrite(modifiedPcf, pin, val);
      write(modifiedPcf);
    }
};
