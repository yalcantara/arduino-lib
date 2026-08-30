#include <Arduino.h>
#include <unity.h>

#include <ylib/railroad/DCSensor.h>

namespace {
constexpr float ADC_REFERENCE_VOLTS = 5.0f;
constexpr float ADC_MAX_READING = 1023.0f;
constexpr float INITIAL_OFFSET_MILLIAMPS = 700.0f;
constexpr float CALIBRATION_DEADBAND_MILLIAMPS =
    DCSensor::CALIBRATION_DEADBAND_MILLIAMPS;
constexpr float MAX_SENSOR_NOISE_MILLIAMPS = 1.5f;
constexpr float ADC_VOLTS_PER_COUNT = ADC_REFERENCE_VOLTS / ADC_MAX_READING;
constexpr float NOISY_FAKE_SENSOR_SENSITIVITY =
    ADC_VOLTS_PER_COUNT / (MAX_SENSOR_NOISE_MILLIAMPS / 1000.0f);
constexpr uint8_t MIN_CALIBRATION_ITERATIONS = 5;
constexpr uint8_t MAX_CALIBRATION_ITERATIONS = 50;
constexpr uint8_t REQUIRED_STABLE_READINGS =
    DCSensor::REQUIRED_STABLE_READINGS;
constexpr uint8_t CURRENT_COLUMN_WIDTH = 8;
constexpr uint8_t CALIBRATION_BUFF_SIZE = 64;

constexpr float adcVoltage(const int reading) {
  return static_cast<float>(reading) * (ADC_REFERENCE_VOLTS / ADC_MAX_READING);
}

constexpr float calibrationForCurrent(
  const int reading,
  const float milliAmps,
  const float sensorSensitivity = ACS712_SENSITIVITY) {
  return adcVoltage(reading) -
      (milliAmps / 1000.0f) * sensorSensitivity;
}

/**
 * A DCSensor that returns a controlled ADC value instead of reading hardware.
 * A fixed pseudo-random sequence can add one or more ADC counts of noise while
 * keeping the test repeatable across runs.
 */
class FakeDCSensor final: public DCSensor {
  const int _reading;
  const uint8_t _maxNoiseAdcCounts;
  mutable uint8_t _readCount = 0;
  mutable uint32_t _randomState = 0x00C0FFEEu;

  public:
    FakeDCSensor(
      const String &label,
      const int reading,
      const float calibration,
      const float sensorSensitivity = ACS712_SENSITIVITY,
      const uint8_t maxNoiseAdcCounts = 0)
      : DCSensor(label, A1, sensorSensitivity, calibration, 1),
        _reading(reading),
        _maxNoiseAdcCounts(maxNoiseAdcCounts) {
    }

    int doRead() const override {
      ++_readCount;

      if (_maxNoiseAdcCounts == 0) {
        return _reading;
      }

      // A fixed seed keeps the pseudo-random noise repeatable in unit tests.
      _randomState = _randomState * 1664525u + 1013904223u;
      const uint8_t possibleValues = _maxNoiseAdcCounts * 2 + 1;
      const int noiseAdcCounts =
          static_cast<int>(_randomState % possibleValues) - _maxNoiseAdcCounts;

      return _reading + noiseAdcCounts;
    }

    [[nodiscard]] uint8_t readCount() const {
      return _readCount;
    }
};

void reportCalibrationStep(
  const uint8_t iteration,
  const float currentMilliAmps,
  const uint8_t stableReadings) {
  const String currentText(currentMilliAmps, 2);

  Serial.print("Step ");
  if (iteration < 10) {
    Serial.print(' ');
  }
  Serial.print(iteration);
  Serial.print(" - current: ");
  for (uint8_t column = currentText.length();
       column < CURRENT_COLUMN_WIDTH;
       ++column) {
    Serial.print(' ');
  }
  Serial.print(currentText);
  Serial.print(" mA  stable: ");
  Serial.print(stableReadings);
  Serial.print('/');
  Serial.println(REQUIRED_STABLE_READINGS);
}
} // namespace

// Confirm that loop() uses the virtual read method and custom sensitivity.
void test_dcsensor_uses_overridden_reading() {
  constexpr int reading = 600;
  constexpr float calibration = 2.5f;
  constexpr float sensorSensitivity = 0.100f;
  constexpr float expectedMilliAmps =
      ((adcVoltage(reading) - calibration) / sensorSensitivity) * 1000.0f;

  FakeDCSensor sensor("fake", reading, calibration, sensorSensitivity);
  sensor.loop();

  TEST_ASSERT_EQUAL_UINT8(1, sensor.readCount());
  TEST_ASSERT_FLOAT_WITHIN(0.01f, expectedMilliAmps, sensor.getMilliAmps());
}

// Confirm that noisy zero-current readings must be stable 7 times in a row.
void test_dcsensor_calibration() {
  constexpr int reading = 512;
  FakeDCSensor sensor(
    "fake-sensor",
    reading,
    calibrationForCurrent(
      reading,
      INITIAL_OFFSET_MILLIAMPS,
      NOISY_FAKE_SENSOR_SENSITIVITY),
    NOISY_FAKE_SENSOR_SENSITIVITY,
    1);

  float calibrationSamples[CALIBRATION_BUFF_SIZE] = {};
  uint8_t iterations = 0;
  uint8_t stableReadings = 0;
  bool sawNegativeNoise = false;
  bool sawPositiveNoise = false;

  while (!sensor.isCalibrated() && iterations < MAX_CALIBRATION_ITERATIONS) {
    sensor.loop();
    ++iterations;

    const float currentMilliAmps = sensor.getMilliAmps();
    if (currentMilliAmps >= -CALIBRATION_DEADBAND_MILLIAMPS &&
      currentMilliAmps <= CALIBRATION_DEADBAND_MILLIAMPS) {
      ++stableReadings;
      sawNegativeNoise = sawNegativeNoise || currentMilliAmps < 0.0f;
      sawPositiveNoise = sawPositiveNoise || currentMilliAmps > 0.0f;
    } else {
      stableReadings = 0;
    }

    reportCalibrationStep(iterations, currentMilliAmps, stableReadings);

    const bool calibrationComplete =
        sensor.calibrate(calibrationSamples, CALIBRATION_BUFF_SIZE);
    if (stableReadings < REQUIRED_STABLE_READINGS) {
      TEST_ASSERT_FALSE(calibrationComplete);
    }
  }

  TEST_ASSERT_TRUE(sensor.isCalibrated());
  TEST_ASSERT_TRUE(iterations >= MIN_CALIBRATION_ITERATIONS);
  TEST_ASSERT_TRUE(iterations <= MAX_CALIBRATION_ITERATIONS);
  TEST_ASSERT_EQUAL_UINT8(iterations, sensor.readCount());
  TEST_ASSERT_EQUAL_UINT8(REQUIRED_STABLE_READINGS, stableReadings);
  TEST_ASSERT_TRUE(sawNegativeNoise);
  TEST_ASSERT_TRUE(sawPositiveNoise);
  TEST_ASSERT_FLOAT_WITHIN(
    CALIBRATION_DEADBAND_MILLIAMPS,
    0.0f,
    sensor.getMilliAmps());
}
