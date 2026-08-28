#ifndef __ylib
#define __ylib

uint8_t __YLIB_DEBUG_ENABLED = 0;

// String manipulation functions
// =========================================================
inline String padWithSpaces(int size, long value) {
  // Ensure minimum valid size
  if (size < 1) size = 1;

  // Construct format string dynamically, e.g., "%6ld"
  char format[10];
  sprintf(format, "%%%dld", size);  // Note: double %% to escape %

  // Allocate buffer with enough space: size + null terminator
  char buffer[size + 1];
  sprintf(buffer, format, value);

  return String(buffer);
}

// Helper functions
// =========================================================
inline int avgAnalogRead(const uint8_t pin, const uint8_t samples = 8) {
  float sum = 0.0;
  for (int i = 0; i < samples; i++) {
    sum += analogRead(pin);
  }

  return (int) round( sum / samples);
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


    // Assing the value to the current index and return the previous value.
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

#endif
