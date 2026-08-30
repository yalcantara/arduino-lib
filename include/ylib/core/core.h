#pragma once

namespace ylib::core {

// String manipulation functions
// =========================================================
/**
 * Add spaces before a number until it reaches the requested text width.
 * This is useful for keeping values aligned in serial output.
 */
inline String padWithSpaces(int size, long value) {
  // Ensure minimum valid size
  if (size < 1) size = 1;

  // Construct format string dynamically, e.g., "%6ld"
  char format[10];
  sprintf(format, "%%%dld", size);  // Note: double %% to escape %

  // Allocate buffer with enough space: size + null terminator
  char buffer[size + 1];
  sprintf(buffer, format, value);

  return {buffer};
}

// Helper functions
// =========================================================
/**
 * Read one analog pin several times and return the rounded average.
 * More samples reduce short spikes but also make each call take longer.
 */
inline int avgAnalogRead(const uint8_t pin, const uint8_t samples = 8) {
  float sum = 0.0;
  for (int i = 0; i < samples; i++) {
    sum += static_cast<float>(analogRead(pin));
  }

  return static_cast<int>(round(sum / static_cast<float>(samples)));
}

inline void checkArgument(const long min, const long max, const long value) {
  if (value > max || value < min) {
    for (int i = 1; i <= 3; i++) {
      Serial.print("\n\n>>>>>>>>Value out of range: ");
      Serial.print(value);
      Serial.print(" (");
      Serial.print(min);
      Serial.print(", ");
      Serial.print(max);
      Serial.println(")\n");
      delay(3000);
    }
  }
}

inline void checkArgUnsigned(const unsigned long min, const unsigned long max, const unsigned long value) {
  if (value > max || value < min) {
    Serial.print("\n\n>>>>>>>>Value out of range: ");
    Serial.print(value);
    Serial.print(" (");
    Serial.print(min);
    Serial.print(", ");
    Serial.print(max);
    Serial.println(")\n");
    delay(3000);
  }
}

inline void checkArgFloat(const float min, const float max, const float value) {
  if (value > max || value < min) {
    Serial.print("\n\n>>>>>>>>Value out of range: ");
    Serial.print(value);
    Serial.print(" (");
    Serial.print(min);
    Serial.print(", ");
    Serial.print(max);
    Serial.println(")\n");
    delay(3000);
  }
}
// =========================================================

/**
 * A fixed-size ring that keeps the most recent values.
 *
 * New values replace the oldest values after the ring becomes full. Storage
 * is allocated once when the object is created and released with the object.
 */
template<typename T>
class CircularBuffer {
  T *_arr;
  size_t _size;
  size_t _crtIdx;
  bool _filled;

  public:
    explicit CircularBuffer(const size_t size) {
      _arr = (T *) calloc(sizeof(T), size);
      _size = size;
      _crtIdx = 0;
      _filled = false;
    }

    size_t size() const { return _size; }

    T get(size_t index) const {
      if (index >= _size) {
        Serial.println("CircularBuffer: Index out of range");
        return 0;
      }
      return _arr[index];
    }


    // Store a value at the current position and return the replaced value.
    T next(T value) {
      T prev = _arr[_crtIdx];
      _arr[_crtIdx] = value;

      if (_crtIdx + 1 >= _size) {
        _crtIdx = 0; // Reset the index to 0
        _filled = true;
      } else {
        _crtIdx += 1;
      }

      return prev;
    }

    // Sum up to n recent values, starting with the newest value.
    float sumOfLast(const size_t n) const {
      if (n < 1) {
        return 0.0f;
      }

      size_t filled = _filled ? _size : _crtIdx;
      if (filled == 0) {
        return 0.0f;
      }

      size_t count = n > filled ? filled : n;
      float sum = 0.0f;

      for (size_t i = 0; i < count; ++i) {
        size_t idx = (_crtIdx + _size - 1 - i) % _size;
        sum += static_cast<float>(_arr[idx]);
      }

      return sum;
    }

    // Remove all saved values without changing the buffer capacity.
    void reset() {
      for (size_t i = 0; i < _size; i++) {
        _arr[i] = 0;
      }

      _crtIdx = 0;
      _filled = false;
    }

    size_t getFilledCount() const {
      if (_filled) {
        return _size;
      }
      return _crtIdx;
    }

    bool isFilled() const {
      return _filled;
    }

    T* c_ptr() const {
      return _arr;
    }

    ~CircularBuffer() {
      if (_arr) {
        free(_arr);
      }
    }
};

/**
 * A simple moving average over a CircularBuffer.
 *
 * While the buffer is filling, getAvg() uses only the values received so far.
 * After it is full, each new value replaces the oldest value in the average.
 */
template<typename T>
class SMA {
  // Simple Moving Average

  CircularBuffer<T> _buff;
  float _accSum;

  public:
    explicit SMA(const size_t size) : _buff(size) { _accSum = 0.0; }

     void reset() {
      _buff.reset();
      _accSum = 0.0;
    }

    void next(T value) {
      if (_buff.size() == 1) {
        _buff.next(value);
        _accSum = value;
      } else if (_buff.isFilled()) {
        _accSum -= _buff.next(value);
        _accSum += value;
      } else {
        _buff.next(value);
        _accSum += value;
      }
    }

    size_t size() const { return _buff.size(); }

    float getAccSum() const { return _accSum; }

    float getAvg() const {
      if (_buff.getFilledCount() == 0) {
        return 0.0;
      }

      if (_buff.isFilled()) {
        return _accSum / _buff.size();
      }

      return _accSum / _buff.getFilledCount();
    }

    float getAvgOfLast(size_t n) const {
      if (n < 1) {
        return 0.0;
      }

      return _buff.sumOfLast(n) / n;
    }

    float isFilled() const {
      return _buff.isFilled();
    }
};

/**
 * A small non-blocking timer based on millis().
 *
 * Call next() often from loop(). It returns true once per timeout period and
 * false between timeouts, so the program can keep doing other work.
 */
class Timer {
  const unsigned long _timeout;
  unsigned long _instant = 0; // Instant in milliseconds when the timer was started

  public:
    explicit Timer(const int timeout) : _timeout(timeout) {
      checkArgUnsigned(10, 60000, _timeout);
    }

    bool next() {
      if (_instant == 0) {
        _instant = millis();
        return false; // Timer just started, so no timeout reached yet
      }

      // If the timer was already started, check if the timeout is reached
      if (millis() - _instant >= _timeout) {
        _instant = millis(); // Reset the instant
        return true; // Timeout reached
      }

      return false; // Timeout not reached
    }

};
} // namespace ylib::core
