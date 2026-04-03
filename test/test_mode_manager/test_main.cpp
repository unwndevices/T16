#include "Arduino.h"

// Include implementation sources for native test linking
#include "../native_stubs/Arduino.cpp"
#include "../../src/services/ModeManager.cpp"

#include <unity.h>
#include "../../src/Types.hpp"
#include "../../src/services/ModeManager.hpp"

static t16::ModeManager mm;

void setUp(void)
{
    mm = t16::ModeManager();
}

void tearDown(void) {}

// --- Initial state ---

void test_initial_mode_is_keyboard(void)
{
    TEST_ASSERT_EQUAL(KEYBOARD, mm.getMode());
}

void test_initial_slider_mode_is_bend(void)
{
    TEST_ASSERT_EQUAL(BEND, mm.getSliderMode());
}

// --- setMode transitions ---

void test_set_mode_strum(void)
{
    mm.setMode(STRUM);
    TEST_ASSERT_EQUAL(STRUM, mm.getMode());
    TEST_ASSERT_EQUAL(STRUMMING, mm.getSliderMode());
}

void test_set_mode_xy_pad(void)
{
    mm.setMode(XY_PAD);
    TEST_ASSERT_EQUAL(XY_PAD, mm.getMode());
    TEST_ASSERT_EQUAL(SLEW, mm.getSliderMode());
}

// --- cycleSliderMode ---

void test_cycle_slider_keyboard(void)
{
    // KEYBOARD mode: BEND -> MOD -> OCTAVE -> BANK -> BEND
    TEST_ASSERT_EQUAL(BEND, mm.getSliderMode());

    mm.cycleSliderMode();
    TEST_ASSERT_EQUAL(MOD, mm.getSliderMode());

    mm.cycleSliderMode();
    TEST_ASSERT_EQUAL(OCTAVE, mm.getSliderMode());

    mm.cycleSliderMode();
    TEST_ASSERT_EQUAL(BANK, mm.getSliderMode());

    mm.cycleSliderMode();
    TEST_ASSERT_EQUAL(BEND, mm.getSliderMode());
}

void test_cycle_slider_strum(void)
{
    mm.setMode(STRUM);
    // STRUM mode: STRUMMING -> OCTAVE -> BANK -> STRUMMING
    TEST_ASSERT_EQUAL(STRUMMING, mm.getSliderMode());

    mm.cycleSliderMode();
    TEST_ASSERT_EQUAL(OCTAVE, mm.getSliderMode());

    mm.cycleSliderMode();
    TEST_ASSERT_EQUAL(BANK, mm.getSliderMode());

    mm.cycleSliderMode();
    TEST_ASSERT_EQUAL(STRUMMING, mm.getSliderMode());
}

// --- getAllowedSliderModes ---

void test_allowed_modes_keyboard(void)
{
    size_t count = 0;
    const SliderMode* modes = mm.getAllowedSliderModes(count);
    TEST_ASSERT_EQUAL(4, count);
    TEST_ASSERT_NOT_NULL(modes);
    TEST_ASSERT_EQUAL(BEND, modes[0]);
    TEST_ASSERT_EQUAL(MOD, modes[1]);
    TEST_ASSERT_EQUAL(OCTAVE, modes[2]);
    TEST_ASSERT_EQUAL(BANK, modes[3]);
}

void test_allowed_modes_quick(void)
{
    mm.setMode(QUICK_SETTINGS);
    size_t count = 0;
    const SliderMode* modes = mm.getAllowedSliderModes(count);
    TEST_ASSERT_EQUAL(1, count);
    TEST_ASSERT_NOT_NULL(modes);
    TEST_ASSERT_EQUAL(QUICK, modes[0]);
}

int main(int argc, char **argv)
{
    UNITY_BEGIN();
    RUN_TEST(test_initial_mode_is_keyboard);
    RUN_TEST(test_initial_slider_mode_is_bend);
    RUN_TEST(test_set_mode_strum);
    RUN_TEST(test_set_mode_xy_pad);
    RUN_TEST(test_cycle_slider_keyboard);
    RUN_TEST(test_cycle_slider_strum);
    RUN_TEST(test_allowed_modes_keyboard);
    RUN_TEST(test_allowed_modes_quick);
    return UNITY_END();
}
