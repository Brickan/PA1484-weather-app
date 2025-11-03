/**
 * @file Arduino.h
 * @brief Comprehensive Arduino Core API compatibility layer for PC simulation
 *
 * This file provides a complete implementation of Arduino's core functionality
 * allowing Arduino sketches (.ino files) to run on PC without modification.
 */

#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdarg.h>
#include <string>
#include <algorithm>

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// ARDUINO CONSTANTS
// =============================================================================
#define HIGH 0x1
#define LOW  0x0

// On Windows, INPUT conflicts with winuser.h, so we use different names
#ifdef _WIN32
    #define ARDUINO_INPUT 0x0
    #define ARDUINO_OUTPUT 0x1
    #define ARDUINO_INPUT_PULLUP 0x2
    #define ARDUINO_INPUT_PULLDOWN 0x3
    // Don't define INPUT/OUTPUT on Windows to avoid conflicts
#else
    #define INPUT 0x0
    #define OUTPUT 0x1
    #define INPUT_PULLUP 0x2
    #define INPUT_PULLDOWN 0x3
#endif

#define PI 3.1415926535897932384626433832795
#define HALF_PI 1.5707963267948966192313216916398
#define TWO_PI 6.283185307179586476925286766559
#define DEG_TO_RAD 0.017453292519943295769236907684886
#define RAD_TO_DEG 57.295779513082320876798154814105
#define EULER 2.718281828459045235360287471352

#define SERIAL  0x0
#define DISPLAY 0x1

#define LSBFIRST 0
#define MSBFIRST 1

#define CHANGE 1
#define FALLING 2
#define RISING 3

// Number base constants (defined early for Serial class)
#define DEC 10
#define HEX 16
#define OCT 8
#define BIN 2

// =============================================================================
// ARDUINO TIME FUNCTIONS
// =============================================================================
static unsigned long _arduino_start_time = 0;

static inline unsigned long millis() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    unsigned long now = (tv.tv_sec * 1000UL) + (tv.tv_usec / 1000UL);
    if (_arduino_start_time == 0) _arduino_start_time = now;
    return now - _arduino_start_time;
}

static inline unsigned long micros() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    unsigned long now = (tv.tv_sec * 1000000UL) + tv.tv_usec;
    if (_arduino_start_time == 0) _arduino_start_time = now / 1000;
    return now - (_arduino_start_time * 1000);
}

static inline void delay(unsigned long ms) {
    usleep(ms * 1000);
}

static inline void delayMicroseconds(unsigned int us) {
    usleep(us);
}

// ESP32/Arduino yield function - lets system tasks run
static inline void yield() {
    // In simulator, yield is a brief delay to let other threads run
    usleep(1);  // 1 microsecond delay
}

// Also provide Yield (capital Y) for compatibility
static inline void Yield() {
    yield();
}

// Windows compatibility: localtime_r doesn't exist on Windows
#ifdef _WIN32
static inline struct tm* localtime_r(const time_t* timer, struct tm* buf) {
    localtime_s(buf, timer);
    return buf;
}

// Windows compatibility: setenv doesn't exist on Windows
static inline int setenv(const char* name, const char* value, int overwrite) {
    if (!overwrite && getenv(name)) return 0;
    return _putenv_s(name, value);
}
#endif

// =============================================================================
// ARDUINO DIGITAL I/O (MOCK)
// =============================================================================
static inline void pinMode(uint8_t pin, uint8_t mode) {
    // Mock - does nothing on PC
}

static inline void digitalWrite(uint8_t pin, uint8_t val) {
    // Mock - does nothing on PC
}

static inline int digitalRead(uint8_t pin) {
    return LOW; // Mock
}

// =============================================================================
// ARDUINO ANALOG I/O (MOCK)
// =============================================================================
static inline int analogRead(uint8_t pin) {
    return 512; // Mock - middle value
}

static inline void analogWrite(uint8_t pin, int val) {
    // Mock - does nothing on PC
}

static inline void analogReference(uint8_t mode) {
    // Mock
}

// =============================================================================
// ARDUINO MATH & UTILITIES
// =============================================================================
// Don't define min/max as macros - they conflict with C++ STL
// Use inline functions instead
#ifndef abs
#define abs(x) ((x)>0?(x):-(x))
#endif
#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
#define round(x)     ((x)>=0?(long)((x)+0.5):(long)((x)-0.5))
#define radians(deg) ((deg)*DEG_TO_RAD)
#define degrees(rad) ((rad)*RAD_TO_DEG)
#define sq(x) ((x)*(x))

static inline long map(long x, long in_min, long in_max, long out_min, long out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// Arduino random functions - avoid system random() conflict by using different names internally
static inline long _arduino_random_range(long howsmall, long howbig) {
    if (howsmall >= howbig) return howsmall;
    long diff = howbig - howsmall;
    return (rand() % diff) + howsmall;
}

static inline long _arduino_random_max(long howbig) {
    if (howbig == 0) return 0;
    return rand() % howbig;
}

// Overload resolution for Arduino random()
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
static inline long random(long howbig) {
    return _arduino_random_max(howbig);
}

static inline long random(long howsmall, long howbig) {
    return _arduino_random_range(howsmall, howbig);
}

extern "C" {
#endif

static inline void randomSeed(unsigned long seed) {
    srand(seed);
}

// =============================================================================
// INTERRUPTS (MOCK)
// =============================================================================
static inline void attachInterrupt(uint8_t pin, void (*)(void), int mode) {
    // Mock - does nothing on PC
}

static inline void detachInterrupt(uint8_t pin) {
    // Mock
}

static inline void interrupts() {
    // Mock
}

static inline void noInterrupts() {
    // Mock
}

#ifdef __cplusplus
} // extern "C"
#endif

// =============================================================================
// ARDUINO STRING CLASS (C++ only)
// =============================================================================
#ifdef __cplusplus

class String {
private:
    std::string str;

public:
    // Constructors
    String() : str("") {}
    String(const char* s) : str(s ? s : "") {}
    String(const std::string& s) : str(s) {}
    String(const String& s) : str(s.str) {}
    String(char c) : str(1, c) {}
    String(unsigned char c) : str(1, (char)c) {}
    String(int val, int base = 10) {
        char buf[34];
        if (base == 10) snprintf(buf, sizeof(buf), "%d", val);
        else if (base == 16) snprintf(buf, sizeof(buf), "%x", val);
        else if (base == 8) snprintf(buf, sizeof(buf), "%o", val);
        else if (base == 2) {
            int i = 0;
            unsigned int v = (unsigned int)val;
            do { buf[i++] = (v & 1) ? '1' : '0'; v >>= 1; } while (v && i < 32);
            buf[i] = 0;
            for (int j = 0; j < i/2; j++) { char t = buf[j]; buf[j] = buf[i-1-j]; buf[i-1-j] = t; }
        }
        str = buf;
    }
    String(unsigned int val, int base = 10) : String((int)val, base) {}
    String(long val, int base = 10) {
        char buf[34];
        if (base == 10) snprintf(buf, sizeof(buf), "%ld", val);
        else if (base == 16) snprintf(buf, sizeof(buf), "%lx", val);
        else if (base == 8) snprintf(buf, sizeof(buf), "%lo", val);
        str = buf;
    }
    String(unsigned long val, int base = 10) : String((long)val, base) {}
    String(float val, int decimals = 2) {
        char buf[33];
        snprintf(buf, sizeof(buf), "%.*f", decimals, val);
        str = buf;
    }
    String(double val, int decimals = 2) {
        char buf[33];
        snprintf(buf, sizeof(buf), "%.*f", decimals, val);
        str = buf;
    }

    // Memory
    unsigned char reserve(unsigned int size) {
        str.reserve(size);
        return 1;
    }

    // Assignment
    String& operator=(const String& rhs) { str = rhs.str; return *this; }
    String& operator=(const char* cstr) { str = cstr ? cstr : ""; return *this; }
    String& operator=(const std::string& s) { str = s; return *this; }

    // Concatenation
    String& concat(const String& s) { str += s.str; return *this; }
    String& concat(const char* cstr) { if (cstr) str += cstr; return *this; }
    String& concat(char c) { str += c; return *this; }
    String& concat(unsigned char c) { str += (char)c; return *this; }
    String& concat(int num) { return concat(String(num)); }
    String& concat(unsigned int num) { return concat(String(num)); }
    String& concat(long num) { return concat(String(num)); }
    String& concat(unsigned long num) { return concat(String(num)); }
    String& concat(float num) { return concat(String(num)); }
    String& concat(double num) { return concat(String(num)); }

    String& operator+=(const String& rhs) { str += rhs.str; return *this; }
    String& operator+=(const char* cstr) { if (cstr) str += cstr; return *this; }
    String& operator+=(char c) { str += c; return *this; }

    friend String operator+(const String& lhs, const String& rhs) { return String(lhs.str + rhs.str); }
    friend String operator+(const String& lhs, const char* cstr) { return String(lhs.str + (cstr ? cstr : "")); }
    friend String operator+(const char* cstr, const String& rhs) { return String((cstr ? cstr : "") + rhs.str); }
    friend String operator+(const String& lhs, char c) { return String(lhs.str + c); }
    friend String operator+(char c, const String& rhs) { return String(c + rhs.str); }

    // Comparison
    bool equals(const String& s) const { return str == s.str; }
    bool equals(const char* cstr) const { return str == (cstr ? cstr : ""); }
    bool equalsIgnoreCase(const String& s) const {
        if (str.length() != s.str.length()) return false;
        for (size_t i = 0; i < str.length(); i++)
            if (tolower(str[i]) != tolower(s.str[i])) return false;
        return true;
    }

    bool operator==(const String& rhs) const { return str == rhs.str; }
    bool operator==(const char* cstr) const { return str == (cstr ? cstr : ""); }
    bool operator!=(const String& rhs) const { return str != rhs.str; }
    bool operator!=(const char* cstr) const { return str != (cstr ? cstr : ""); }
    bool operator<(const String& rhs) const { return str < rhs.str; }
    bool operator>(const String& rhs) const { return str > rhs.str; }
    bool operator<=(const String& rhs) const { return str <= rhs.str; }
    bool operator>=(const String& rhs) const { return str >= rhs.str; }

    // Character access
    char charAt(unsigned int index) const { return index < str.length() ? str[index] : 0; }
    void setCharAt(unsigned int index, char c) { if (index < str.length()) str[index] = c; }
    char operator[](unsigned int index) const { return str[index]; }
    char& operator[](unsigned int index) { return str[index]; }

    // Conversion
    const char* c_str() const { return str.c_str(); }
    void getBytes(unsigned char* buf, unsigned int len, unsigned int index = 0) const {
        if (!buf || len == 0) return;
        if (index >= str.length()) { buf[0] = 0; return; }
        size_t n = std::min((size_t)(len - 1), str.length() - index);
        memcpy(buf, str.c_str() + index, n);
        buf[n] = 0;
    }
    void toCharArray(char* buf, unsigned int len, unsigned int index = 0) const {
        getBytes((unsigned char*)buf, len, index);
    }

    int toInt() const { return atoi(str.c_str()); }
    float toFloat() const { return atof(str.c_str()); }
    double toDouble() const { return atof(str.c_str()); }

    // Search
    int indexOf(char c, unsigned int fromIndex = 0) const {
        size_t pos = str.find(c, fromIndex);
        return pos != std::string::npos ? (int)pos : -1;
    }
    int indexOf(const String& s, unsigned int fromIndex = 0) const {
        size_t pos = str.find(s.str, fromIndex);
        return pos != std::string::npos ? (int)pos : -1;
    }
    int indexOf(const char* cstr, unsigned int fromIndex = 0) const {
        if (!cstr) return -1;
        size_t pos = str.find(cstr, fromIndex);
        return pos != std::string::npos ? (int)pos : -1;
    }

    int lastIndexOf(char c) const {
        size_t pos = str.rfind(c);
        return pos != std::string::npos ? (int)pos : -1;
    }
    int lastIndexOf(char c, unsigned int fromIndex) const {
        size_t pos = str.rfind(c, fromIndex);
        return pos != std::string::npos ? (int)pos : -1;
    }
    int lastIndexOf(const String& s) const {
        size_t pos = str.rfind(s.str);
        return pos != std::string::npos ? (int)pos : -1;
    }

    // Substring
    String substring(unsigned int beginIndex) const {
        if (beginIndex >= str.length()) return String("");
        return String(str.substr(beginIndex));
    }
    String substring(unsigned int beginIndex, unsigned int endIndex) const {
        if (beginIndex >= str.length()) return String("");
        if (endIndex > str.length()) endIndex = str.length();
        if (endIndex <= beginIndex) return String("");
        return String(str.substr(beginIndex, endIndex - beginIndex));
    }

    // Modification
    void replace(char find, char replace) {
        for (char& c : str) if (c == find) c = replace;
    }
    void replace(const String& find, const String& replace) {
        size_t pos = 0;
        while ((pos = str.find(find.str, pos)) != std::string::npos) {
            str.replace(pos, find.str.length(), replace.str);
            pos += replace.str.length();
        }
    }

    void remove(unsigned int index) {
        if (index < str.length()) str.erase(index);
    }
    void remove(unsigned int index, unsigned int count) {
        if (index < str.length()) str.erase(index, count);
    }

    void toLowerCase() {
        for (char& c : str) c = tolower(c);
    }
    void toUpperCase() {
        for (char& c : str) c = toupper(c);
    }

    void trim() {
        size_t start = str.find_first_not_of(" \t\n\r\f\v");
        if (start == std::string::npos) { str.clear(); return; }
        size_t end = str.find_last_not_of(" \t\n\r\f\v");
        str = str.substr(start, end - start + 1);
    }

    // Properties
    unsigned int length() const { return str.length(); }
    bool startsWith(const String& prefix) const {
        return str.length() >= prefix.str.length() &&
               str.compare(0, prefix.str.length(), prefix.str) == 0;
    }
    bool startsWith(const String& prefix, unsigned int offset) const {
        if (offset >= str.length()) return false;
        return str.compare(offset, prefix.str.length(), prefix.str) == 0;
    }
    bool endsWith(const String& suffix) const {
        if (suffix.str.length() > str.length()) return false;
        return str.compare(str.length() - suffix.str.length(), suffix.str.length(), suffix.str) == 0;
    }

    // Stream compatibility
    friend std::ostream& operator<<(std::ostream& os, const String& s) {
        return os << s.str;
    }
};

// Arduino min/max as inline functions (not macros) to avoid C++ STL conflicts
template<typename T>
inline T min(T a, T b) { return (a < b) ? a : b; }

template<typename T>
inline T max(T a, T b) { return (a > b) ? a : b; }

// =============================================================================
// ARDUINO SERIAL CLASS (MOCK)
// =============================================================================
class HardwareSerial {
public:
    void begin(unsigned long baud) {
        printf("[Serial] Started at %lu baud\n", baud);
        fflush(stdout);
    }
    void begin(unsigned long baud, uint32_t config) { begin(baud); }
    void end() {}

    int available() { return 0; }
    int read() { return -1; }
    int peek() { return -1; }
    void flush() { fflush(stdout); }

    size_t write(uint8_t c) { printf("%c", c); return 1; }
    size_t write(const uint8_t* buffer, size_t size) {
        return fwrite(buffer, 1, size, stdout);
    }

    size_t print(const char* s) { printf("%s", s); fflush(stdout); return strlen(s); }
    size_t print(const String& s) { return print(s.c_str()); }
    size_t print(char c) { printf("%c", c); fflush(stdout); return 1; }
    size_t print(unsigned char c, int base = DEC) { return print((unsigned long)c, base); }
    size_t print(int n, int base = DEC) { return print((long)n, base); }
    size_t print(unsigned int n, int base = DEC) { return print((unsigned long)n, base); }
    size_t print(long n, int base = DEC) {
        char buf[34];
        if (base == DEC) snprintf(buf, sizeof(buf), "%ld", n);
        else if (base == HEX) snprintf(buf, sizeof(buf), "%lX", n);
        else if (base == OCT) snprintf(buf, sizeof(buf), "%lo", n);
        else if (base == BIN) {
            unsigned long v = (unsigned long)n;
            int i = 0;
            do { buf[i++] = (v & 1) ? '1' : '0'; v >>= 1; } while (v && i < 32);
            buf[i] = 0;
            for (int j = 0; j < i/2; j++) { char t = buf[j]; buf[j] = buf[i-1-j]; buf[i-1-j] = t; }
        }
        return print(buf);
    }
    size_t print(unsigned long n, int base = DEC) { return print((long)n, base); }
    size_t print(double n, int digits = 2) {
        char buf[33];
        snprintf(buf, sizeof(buf), "%.*f", digits, n);
        return print(buf);
    }

    size_t println() { printf("\n"); fflush(stdout); return 1; }
    size_t println(const char* s) { size_t n = print(s); println(); return n + 1; }
    size_t println(const String& s) { return println(s.c_str()); }
    size_t println(char c) { print(c); println(); return 2; }
    size_t println(unsigned char c, int base = DEC) { print(c, base); println(); return 2; }
    size_t println(int n, int base = DEC) { print(n, base); println(); return 2; }
    size_t println(unsigned int n, int base = DEC) { print(n, base); println(); return 2; }
    size_t println(long n, int base = DEC) { print(n, base); println(); return 2; }
    size_t println(unsigned long n, int base = DEC) { print(n, base); println(); return 2; }
    size_t println(double n, int digits = 2) { print(n, digits); println(); return 2; }

    void printf(const char* format, ...) {
        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);
        fflush(stdout);
    }

    operator bool() { return true; }
};

// Global Serial object
static HardwareSerial Serial;

#endif // __cplusplus
