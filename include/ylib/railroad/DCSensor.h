//
// Created by Yaison on 18/6/25.
//

#ifndef DCSENSOR_H
#define DCSENSOR_H

#include <ylib/ylib.h>

constexpr float ACS712_SENSITIVITY = 0.185; // Sensitivity of the ACS712 sensor in V/A (185 mV/A)
constexpr float ACS712_MAX_AMPS = 5.0; // Maximum current in Amps

class DCSensor {
  const String _label;
  const int _pin;
  float _calibration; // Calibration value for the sensor
  SMA<int> _buffer;
  bool _calibrated = false;

  [[nodiscard]] float computeMilliAmps(const float rawADC) const {
    // Convert ADC reading to voltage
    const float voltage = rawADC * (5.0f / 1023.0);

    // Calculate current in Amps
    const float calibratedVoltage = voltage - _calibration;
    const float currentAmps = calibratedVoltage / ACS712_SENSITIVITY;

    // Convert to milliamps
    return currentAmps * 1000.0f;
  }

  public:
    DCSensor(
      const String label,
      const int pin,
      const float calibration,
      const int buffSize = 8): _label(label),
                               _pin(pin),
                               _calibration(calibration),
                               _buffer(buffSize) {
    }

    void setup() const {
      checkArgument(PIN_A1, PIN_A5, _pin);
      checkArgFloat(1.0, 5.0, _calibration);
    }

    void loop() {
      const int r = avgAnalogRead(_pin, 16);
      if (r < 0 || r > 1023) {
        Serial.println("Analog read out of range (0-1023)");
        return;
      }

      _buffer.next(r);
    }

    [[nodiscard]] bool isCalibrated() const {
      return _calibrated;
    }


    [[nodiscard]] String label() const {
      return _label;
    }

    [[nodiscard]] float getLastMilliAmps(const size_t n) const {
      if (_buffer.isFilled() == false) {
        return 0.0;
      }

      return computeMilliAmps(_buffer.getAvgOfLast(n));
    }

    [[nodiscard]] float getMilliAmps() const {
      if (_buffer.isFilled() == false) {
        return 0.0;
      }

      return computeMilliAmps(_buffer.getAvg());
    }


    static const uint8_t CALIB_SAMPLES = 64;

    void calibrate(float buff[]) {
      // Persistent state
      static uint8_t idx = 0;
      static bool full = false;
      static float lastAvg = 0.0f;
      static uint8_t withinBandCnt = 0; // consecutive safe readings

      // ---- 1) Take new reading ----
      float mA = getMilliAmps();
      if (mA == 0.0f) {
        // No valid reading yet
        return;
      }

      // ---- 2) Store in circular buffer ----
      buff[idx] = mA;
      idx++;
      if (idx >= CALIB_SAMPLES) {
        idx = 0;
        full = true;
      }

      // ---- 3) Compute rolling average ----
      uint8_t count = full ? CALIB_SAMPLES : idx;
      if (count == 0) return;

      float sum = 0.0f;
      for (uint8_t i = 0; i < count; ++i) sum += buff[i];
      float avg = sum / (float) count;

      // ---- 4) Deadband logic ----
      const float deadband_mA = 2.0f; // acceptable noise range

      if (fabsf(avg) <= deadband_mA) {
        // Average inside acceptable band → increment safety counter
        if (withinBandCnt < 255) withinBandCnt++;

        // Need *7 consecutive safe averages* to stop adjusting
        if (withinBandCnt >= 7) {
          lastAvg = avg;
          _calibrated = true;
          Serial.print(_label);
          Serial.print(" -> ");
          Serial.print("Calibration complete. Final Avg mA: ");
          Serial.print(avg, 2);
          Serial.print("  Final Calib: ");
          Serial.print(_calibration, 5);
          Serial.println();
          return;
        }
        // Otherwise continue adjusting
      } else {
        // Outside acceptable noise band → reset counter
        withinBandCnt = 0;
      }

      // ---- 5) Fixed calibration step size ----
      float step = 0.0001f; // <-- FIXED STEP SIZE (ABSOLUTE)

      // Overshoot detection can be kept if desired,
      // but with fixed steps it's optional. We'll keep it relevant:
      bool overshoot =
          (avg > 0 && lastAvg < 0) ||
          (avg < 0 && lastAvg > 0);

      if (overshoot) {
        // With fixed step size, overshoot just means continue gently.
        // No need to change step, but we can log or react here if needed.
      }

      // ---- 6) Direction of calibration adjust ----
      // avg > 0  → reading too high → increase _calibration
      // avg < 0  → reading too low  → decrease _calibration
      if (avg > 0)
        _calibration += step;
      else
        _calibration -= step;


      Serial.print(_label);
      Serial.print(" -> ");
      Serial.print("Calibrating... Avg mA: ");
      Serial.print(avg, 2);
      Serial.print("  New Calib: ");
      Serial.print(_calibration, 5);
      Serial.print("  Step: ");
      Serial.print(step, 5);
      Serial.println();
      // ---- 7) Save average for next iteration ----
      lastAvg = avg;
    }
};

#endif //DCSENSOR_H
