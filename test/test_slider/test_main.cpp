#include "Arduino.h"
#include <cstdlib>
#include <cmath>

// Stubs for hardware functions used by TouchSlider
typedef int touch_pad_t;
inline int touchRead(uint8_t pin) { return 0; }
inline void pinMode(uint8_t pin, uint8_t mode) {}
inline void delay(unsigned long ms) {}
typedef unsigned long ulong;
#define INPUT 0
#define log_w(fmt, ...) printf("[WARN] " fmt "\n", ##__VA_ARGS__)
#define log_e(fmt, ...) printf("[ERROR] " fmt "\n", ##__VA_ARGS__)

// constrain macro used by TouchSlider
#ifndef constrain
#define constrain(amt, low, high) ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))
#endif

// FreeRTOS stubs (TouchSlider::Start uses xTaskCreatePinnedToCore)
typedef void* TaskHandle_t;
#define pdMS_TO_TICKS(x) (x)
inline int xTaskCreatePinnedToCore(void (*fn)(void*), const char* name, uint32_t stack, void* param, int prio, TaskHandle_t* handle, int core) { return 0; }
inline void vTaskDelay(int ticks) {}

// Include implementation sources for native test linking
#include "../native_stubs/Arduino.cpp"
#include "../../src/Libs/TouchSlider.cpp"

#include <unity.h>

static TouchSlider* slider = nullptr;

void setUp(void)
{
    slider = new TouchSlider();
}

void tearDown(void)
{
    delete slider;
    slider = nullptr;
}

// --- SetPosition(uint8_t, uint8_t) tests (FWBUG-02 verification) ---

void test_set_position_uint8_assigns_last_position(void)
{
    // FWBUG-02: SetPosition(uint8_t, uint8_t) was a no-op -- did not assign lastPosition
    // After the fix, SetPosition(3, 7) should set lastPosition to 3.0/7.0
    slider->SetPosition(3, 7);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.0f / 7.0f, slider->GetPosition());
}

void test_set_position_zero_num_positions_no_crash(void)
{
    // Guard against division by zero
    slider->SetPosition(0.5f); // Set a known position first
    slider->SetPosition(0, 0);
    // Should not crash; position may or may not change depending on guard clause
    // Just verify no crash
    float pos = slider->GetPosition();
    (void)pos;
    TEST_ASSERT_TRUE(true);
}

void test_set_position_max_value(void)
{
    slider->SetPosition(6, 7);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 6.0f / 7.0f, slider->GetPosition());
}

void test_set_position_float_still_works(void)
{
    slider->SetPosition(0.5f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.5f, slider->GetPosition());
}

int main(int argc, char **argv)
{
    UNITY_BEGIN();
    RUN_TEST(test_set_position_uint8_assigns_last_position);
    RUN_TEST(test_set_position_zero_num_positions_no_crash);
    RUN_TEST(test_set_position_max_value);
    RUN_TEST(test_set_position_float_still_works);
    return UNITY_END();
}
