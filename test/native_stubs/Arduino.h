#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdio>

typedef uint8_t byte;

// Minimal millis() stub
unsigned long millis();

// Allow tests to control fake time
void set_fake_millis(unsigned long ms);

// Minimal log_d stub (printf on native)
#define log_d(fmt, ...) printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)

// Minimal Serial stub for ArduinoJson serialization compatibility
class FakeSerial
{
public:
    size_t write(uint8_t c) { return 1; }
    size_t write(const uint8_t* buf, size_t size) { return size; }
    size_t print(const char* s) { return printf("%s", s); }
    size_t println(const char* s) { return printf("%s\n", s); }
};

extern FakeSerial Serial;
