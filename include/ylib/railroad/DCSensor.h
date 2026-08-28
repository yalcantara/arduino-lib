#pragma once

//
// Created by Yaison on 18/6/25.
//

#include <Arduino.h>
#include <ylib/core/core.h>

namespace arduino {
class String;
}

using namespace ylib::core;

constexpr float ACS712_SENSITIVITY = 0.185; // Sensitivity of the ACS712 sensor in V/A (185 mV/A)
constexpr float ACS712_MAX_AMPS = 5.0; // Maximum current in Amps

class DCSensor {
  const String _label;
  const int _pin;
  const float _sensorSensitivity;

  float _calibration; // Calibration value for the sensor
  SMA<int> _buffer;
  bool _calibrated = false;
  uint8_t _calibrationIdx = 0;
  bool _calibrationBufferFull = false;
  float _lastCalibrationAvg = 0.0f;
  uint8_t _withinCalibrationBandCount = 0;

  [[nodiscard]] float computeMilliAmps(const float rawADC) const {
    // Convert ADC reading to voltage
    const float voltage = rawADC * (5.0f / 1023.0);

    // Calculate current in Amps
    const float calibratedVoltage = voltage - _calibration;
    const float currentAmps = calibratedVoltage / _sensorSensitivity;

    // Convert to milliamps
    return currentAmps * 1000.0f;
  }

  public:
    DCSensor(
      const String &label,
      const int pin,
      const float sensorSensitivity = ACS712_SENSITIVITY,
      // Default assumes ACS712
      const float calibration = 1.0,
      const int buffSize = 8): _label(label),
                               _pin(pin),
                               _sensorSensitivity(sensorSensitivity),
                               _calibration(calibration),
                               _buffer(buffSize) {
    }

    virtual ~DCSensor() = default;

    void setup() const {
      checkArgument(PIN_A1, PIN_A5, _pin);
      checkArgFloat(1.0, 5.0, _calibration);
    }

    virtual int doRead() const {
      return avgAnalogRead(_pin, 16);
    }

    void loop() {
      const int r = doRead();
      if (r < 0 || r > 1023) {
        Serial.println("Analog read out of range (0-1023)");
        return;
      }

      _buffer.next(r);
    }

    [[nodiscard]] bool isCalibrated() const {
      return _calibrated;
    }


    [[nodiscard]] const String &label() const {
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


    static constexpr uint8_t CALIB_SAMPLES = 64;

    bool calibrate(float buff[]) {
      if (_calibrated) {
        return true;
      }

      // ---- 1) Take new reading ----
      if (_buffer.isFilled() == false) {
        return false;
      }

      const float mA = getMilliAmps();

      // ---- 2) Store in circular buffer ----
      buff[_calibrationIdx] = mA;
      _calibrationIdx++;
      if (_calibrationIdx >= CALIB_SAMPLES) {
        _calibrationIdx = 0;
        _calibrationBufferFull = true;
      }

      // ---- 3) Compute rolling average ----
      const uint8_t count = _calibrationBufferFull ? CALIB_SAMPLES : _calibrationIdx;
      if (count == 0) return false;

      float sum = 0.0f;
      for (uint8_t i = 0; i < count; ++i) sum += buff[i];
      const float avg = sum / static_cast<float>(count);

      // ---- 4) Deadband logic ----
      constexpr float deadband_mA = 2.0f; // acceptable noise range

      if (fabsf(avg) <= deadband_mA) {
        // Average inside acceptable band → increment safety counter
        if (_withinCalibrationBandCount < 255) _withinCalibrationBandCount++;

        // Need *7 consecutive safe averages* to stop adjusting
        if (_withinCalibrationBandCount >= 7) {
          _lastCalibrationAvg = avg;
          _calibrated = true;
          Serial.print(_label);
          Serial.print(" -> ");
          Serial.print("Calibration complete.\nFinal Avg mA: ");
          Serial.print(avg, 2);
          Serial.print(", Final Calibration value: ");
          Serial.print(_calibration, 5);
          Serial.println();
          return true;
        }

        // Keep the current calibration while confirming that readings remain
        // inside the deadband.
        _lastCalibrationAvg = avg;
        return false;
      }
      // Outside acceptable noise band → reset counter
      _withinCalibrationBandCount = 0;


      // ---- 5) Move the measured current toward zero ----
      // Limit each correction so a large offset converges predictably instead
      // of causing one abrupt calibration jump.
      constexpr float maxCorrectionMilliAmps = 100.0f;
      const float correctionMilliAmps =
          avg > maxCorrectionMilliAmps
            ? maxCorrectionMilliAmps
            : (avg < -maxCorrectionMilliAmps ? -maxCorrectionMilliAmps : avg);

      _calibration +=
          (correctionMilliAmps / 1000.0f) * _sensorSensitivity;

      // Samples measured with the previous calibration must not influence the
      // next correction.
      _calibrationIdx = 0;
      _calibrationBufferFull = false;
      _lastCalibrationAvg = avg;

      return false;
    }
};
