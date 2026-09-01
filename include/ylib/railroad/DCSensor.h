#pragma once

//
// Created by Yaison on 18/6/25.
//

#include <Arduino.h>
#include <ylib/core/core.h>

namespace ylib::core {
constexpr float ACS712_SENSITIVITY = 0.185; // Sensitivity of the ACS712 sensor in V/A (185 mV/A)
constexpr float ACS712_MAX_AMPS = 5.0; // Maximum current in Amps

/**
 * Read and calibrate a DC current sensor connected to an analog pin.
 *
 * The default sensitivity is for the 5 A ACS712 sensor: 0.185 volts per amp.
 * loop() fills a moving-average buffer, and getMilliAmps() converts that
 * average into milliamps. Calibration assumes that no real current is flowing
 * and moves the measured value toward 0 mA in steps of at most 100 mA.
 */
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
    // A reading from -2 mA through +2 mA is close enough to zero.
    static constexpr float CALIBRATION_DEADBAND_MILLIAMPS = 2.0f;
    // Calibration ends after this many safe readings in a row.
    static constexpr uint8_t REQUIRED_STABLE_READINGS = 7;

    /**
     * Create a sensor with its label, analog pin, sensitivity, zero point, and
     * moving-average size. A larger buffer gives smoother but slower readings.
     */
    DCSensor(
      const String &label,
      const int pin,
      // Default assumes ACS712
      const float sensorSensitivity = ACS712_SENSITIVITY,
      const float calibration = 1.0,
      const int buffSize = 8): _label(label),
                               _pin(pin),
                               _sensorSensitivity(sensorSensitivity),
                               _calibration(calibration),
                               _buffer(buffSize) {
    }

    virtual ~DCSensor() = default;

    // Report a pin or calibration value that is outside the expected range.
    [[nodiscard]] bool setup() const {
      return  checkArgument(PIN_A0, PIN_A5, _pin) &&
              checkArgFloat(1.0, 5.0, _calibration);
    }

    // Override this in tests or for hardware that needs a different read path.
    virtual int doRead() const {
      return avgAnalogRead(_pin, 16);
    }

    // Take one valid 10-bit ADC reading and add it to the moving average.
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

    // Return the current calculated from the most recent n buffered readings.
    [[nodiscard]] float getLastMilliAmps(const size_t n) const {
      if (_buffer.isFilled() == false) {
        return 0.0;
      }

      return computeMilliAmps(_buffer.getAvgOfLast(n));
    }

    // Return the current calculated from the full moving-average buffer.
    [[nodiscard]] float getMilliAmps() const {
      if (_buffer.isFilled() == false) {
        return 0.0;
      }

      return computeMilliAmps(_buffer.getAvg());
    }

    /**
     * Move the zero point toward 0 mA while no real current is flowing.
     *
     * Pass the same buffer and size on every call. A size of at least 64 is
     * recommended. The method returns true after 7 readings in a row stay
     * within the +/-2 mA deadband.
     */
    bool calibrate(float calibrationBuff[], const uint8_t calibrationBuffSize) {
      if (_calibrated) {
        return true;
      }

      // ---- 1) Take new reading ----
      if (_buffer.isFilled() == false) {
        return false;
      }

      const float mA = getMilliAmps();

      // ---- 2) Store in circular buffer ----
      calibrationBuff[_calibrationIdx] = mA;
      _calibrationIdx++;
      if (_calibrationIdx >= calibrationBuffSize) {
        _calibrationIdx = 0;
        _calibrationBufferFull = true;
      }

      // ---- 3) Compute rolling average ----
      const uint8_t count = _calibrationBufferFull ? calibrationBuffSize : _calibrationIdx;
      if (count == 0) return false;

      float sum = 0.0f;
      for (uint8_t i = 0; i < count; ++i) sum += calibrationBuff[i];
      const float avg = sum / static_cast<float>(count);

      // ---- 4) Deadband logic ----
      if (fabsf(mA) <= CALIBRATION_DEADBAND_MILLIAMPS) {
        // Current reading inside acceptable band → increment safety counter
        if (_withinCalibrationBandCount < 255) _withinCalibrationBandCount++;

        // Require consecutive safe readings, not merely a safe rolling average.
        if (_withinCalibrationBandCount >= REQUIRED_STABLE_READINGS) {
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
} // namespace ylib::core
