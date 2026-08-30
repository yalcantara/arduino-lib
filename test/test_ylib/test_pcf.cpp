#include <Arduino.h>
#include <Wire.h>
#include <unity.h>

namespace {
class FakeWire {
  public:
    int transmissionAddress = -1;
    int requestedAddress = -1;
    int requestedBytes = 0;
    int writeCount = 0;
    int endTransmissionCount = 0;
    uint8_t lastWrittenValue = 0;
    int availableBytes = 0;
    int nextReadValue = 0xFF;

    void reset() {
      transmissionAddress = -1;
      requestedAddress = -1;
      requestedBytes = 0;
      writeCount = 0;
      endTransmissionCount = 0;
      lastWrittenValue = 0;
      availableBytes = 0;
      nextReadValue = 0xFF;
    }

    void beginTransmission(const int address) {
      transmissionAddress = address;
    }

    size_t write(const uint8_t value) {
      lastWrittenValue = value;
      ++writeCount;
      return 1;
    }

    uint8_t endTransmission() {
      ++endTransmissionCount;
      return 0;
    }

    uint8_t requestFrom(const int address, const int quantity) {
      requestedAddress = address;
      requestedBytes = quantity;
      return static_cast<uint8_t>(availableBytes);
    }

    int available() const {
      return availableBytes;
    }

    int read() const {
      return nextReadValue;
    }
};

FakeWire fakeWire;
} // namespace

// Replace only this header's Wire object with the test double above.
#define Wire fakeWire
#include <ylib/railroad/PCF.h>
#undef Wire

using ylib::railroad::PCF;

void test_pcf_setup_sets_all_pins_high() {
  constexpr int address = 0x20;
  fakeWire.reset();
  PCF pcf(address);

  pcf.setup();

  TEST_ASSERT_EQUAL_INT(address, fakeWire.transmissionAddress);
  TEST_ASSERT_EQUAL_INT(1, fakeWire.writeCount);
  TEST_ASSERT_EQUAL_INT(1, fakeWire.endTransmissionCount);
  TEST_ASSERT_EQUAL_HEX8(0xFF, fakeWire.lastWrittenValue);
}

void test_pcf_reads_a_byte_and_reports_no_data_as_all_high() {
  constexpr int address = 0x20;
  fakeWire.reset();
  PCF pcf(address);

  fakeWire.availableBytes = 1;
  fakeWire.nextReadValue = 0xA5;
  TEST_ASSERT_EQUAL_HEX8(0xA5, pcf.read());
  TEST_ASSERT_EQUAL_INT(address, fakeWire.requestedAddress);
  TEST_ASSERT_EQUAL_INT(1, fakeWire.requestedBytes);

  fakeWire.availableBytes = 0;
  TEST_ASSERT_EQUAL_HEX8(0xFF, pcf.read());
}

void test_pcf_changes_one_output_bit() {
  fakeWire.reset();
  PCF pcf(0x20);

  pcf.write(0b00000001, 3, HIGH);
  TEST_ASSERT_EQUAL_HEX8(0b00001001, fakeWire.lastWrittenValue);

  pcf.write(0b00001001, 3, LOW);
  TEST_ASSERT_EQUAL_HEX8(0b00000001, fakeWire.lastWrittenValue);
  TEST_ASSERT_EQUAL_INT(2, fakeWire.endTransmissionCount);
}
