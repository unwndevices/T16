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
