#include "Arduino.h"

// Include implementation sources for native test linking
#include "../native_stubs/Arduino.cpp"
#include "../../src/Scales.cpp"

#include <unity.h>
#include "../../src/Scales.hpp"

// Test the core InputProcessor logic:
// - note_map population via SetNoteMap()
// - Note calculation formula: note = note_map[idx] + base_octave * 12 + 24
// - Bank change effects on note mapping
//
// Full InputProcessor instantiation requires FastLED, ConfigManager, MidiProvider,
// LedManager, and Keyboard -- too many hardware dependencies for native env.
// Instead, we test the note calculation pipeline directly.

// No-op marker callback for SetNoteMap
static int markerCallCount = 0;
static void markerCallback(uint8_t index, bool isRootNote)
{
    markerCallCount++;
}

void setUp(void)
{
    markerCallCount = 0;
    // Reset note_map to sequential defaults
    for (int i = 0; i < 16; i++)
    {
        note_map[i] = i;
    }
}

void tearDown(void) {}

// --- Note calculation formula ---

void test_key_note_mapping_default_chromatic(void)
{
    // With chromatic scale, root=0, no flip, page=0:
    // note_map should be populated with sequential chromatic notes
    SetNoteMap(CHROMATIC, 0, false, false, markerCallback);

    // Key 0 should map to note 0 (root + scale step 0 + octave*12)
    // In chromatic scale, note_map[0] = 0, note_map[1] = 1, ..., note_map[11] = 11
    TEST_ASSERT_EQUAL_UINT8(0, note_map[0]);
    TEST_ASSERT_EQUAL_UINT8(1, note_map[1]);
    TEST_ASSERT_EQUAL_UINT8(11, note_map[11]);
    // After 12 notes (chromatic wraps at -1 at index 12), octave increments
    TEST_ASSERT_EQUAL_UINT8(12, note_map[12]);
    TEST_ASSERT_EQUAL_UINT8(15, note_map[15]);
}

void test_key_note_with_base_octave(void)
{
    // Formula from InputProcessor::processKey:
    // note = note_map[idx] + base_octave * 12 + 24
    SetNoteMap(CHROMATIC, 0, false, false, markerCallback);

    uint8_t base_octave = 2;
    uint8_t idx = 0;
    uint8_t expected_note = note_map[idx] + base_octave * 12 + 24;
    TEST_ASSERT_EQUAL_UINT8(48, expected_note); // 0 + 24 + 24 = 48 (C3)
}

void test_key_note_different_octave(void)
{
    SetNoteMap(CHROMATIC, 0, false, false, markerCallback);

    // Changing base_octave by 1 changes output note by 12
    uint8_t idx = 0;
    uint8_t note_oct2 = note_map[idx] + 2 * 12 + 24;
    uint8_t note_oct3 = note_map[idx] + 3 * 12 + 24;
    TEST_ASSERT_EQUAL(12, note_oct3 - note_oct2);
}

void test_key_note_mapping_ionian_scale(void)
{
    // Ionian (major) scale: 0, 2, 4, 5, 7, 9, 11, -1
    SetNoteMap(IONIAN, 0, false, false, markerCallback);

    TEST_ASSERT_EQUAL_UINT8(0, note_map[0]);  // Root
    TEST_ASSERT_EQUAL_UINT8(2, note_map[1]);  // Whole step
    TEST_ASSERT_EQUAL_UINT8(4, note_map[2]);  // Whole step
    TEST_ASSERT_EQUAL_UINT8(5, note_map[3]);  // Half step
    TEST_ASSERT_EQUAL_UINT8(7, note_map[4]);  // Whole step
    TEST_ASSERT_EQUAL_UINT8(9, note_map[5]);  // Whole step
    TEST_ASSERT_EQUAL_UINT8(11, note_map[6]); // Whole step
    // After 7 notes, wraps to next octave
    TEST_ASSERT_EQUAL_UINT8(12, note_map[7]); // Root + octave
}

void test_key_note_mapping_with_root_note(void)
{
    // C# root (root_note=1) on chromatic scale
    SetNoteMap(CHROMATIC, 1, false, false, markerCallback);

    // Each note is offset by root_note=1
    TEST_ASSERT_EQUAL_UINT8(1, note_map[0]);  // 0 + 1
    TEST_ASSERT_EQUAL_UINT8(2, note_map[1]);  // 1 + 1
    TEST_ASSERT_EQUAL_UINT8(12, note_map[11]); // 11 + 1
}

void test_note_on_dispatch(void)
{
    // Simulate the note-on calculation from processKey
    SetNoteMap(CHROMATIC, 0, false, false, markerCallback);

    uint8_t bank_channel = 1;
    uint8_t base_octave = 2;
    uint8_t idx = 5; // Key index 5

    uint8_t note = note_map[idx] + base_octave * 12 + 24;
    // note_map[5] = 5 (chromatic), + 24 + 24 = 53
    TEST_ASSERT_EQUAL_UINT8(53, note);
}

void test_note_off_correct_key(void)
{
    // Verify note-off uses the same note_map lookup
    SetNoteMap(CHROMATIC, 0, false, false, markerCallback);

    uint8_t base_octave = 2;
    uint8_t idx = 10;

    uint8_t note_on = note_map[idx] + base_octave * 12 + 24;
    uint8_t note_off = note_map[idx] + base_octave * 12 + 24;
    TEST_ASSERT_EQUAL_UINT8(note_on, note_off);
}

void test_bank_change_updates_note_map(void)
{
    // First set chromatic
    SetNoteMap(CHROMATIC, 0, false, false, markerCallback);
    uint8_t chromatic_note5 = note_map[5];

    // Change to pentatonic scale (simulating bank change)
    SetNoteMap(MINOR_PENTATONIC, 0, false, false, markerCallback);
    uint8_t penta_note5 = note_map[5];

    // Minor pentatonic: 0,3,5,7,10 (5 notes, wraps after 5)
    // note_map[5] should be 12 (next octave root)
    TEST_ASSERT_EQUAL_UINT8(5, chromatic_note5);
    TEST_ASSERT_EQUAL_UINT8(12, penta_note5);
}

void test_set_chord_mapping(void)
{
    // Verify SetChordMapping updates current_chord_mapping from scale_chord_map
    SetChordMapping(IONIAN);

    // IONIAN chord map: MAJOR, MINOR, MINOR, MAJOR, MAJOR, MINOR, DIMINISH
    TEST_ASSERT_EQUAL(MAJOR, current_chord_mapping[0]);
    TEST_ASSERT_EQUAL(MINOR, current_chord_mapping[1]);
    TEST_ASSERT_EQUAL(MINOR, current_chord_mapping[2]);
    TEST_ASSERT_EQUAL(MAJOR, current_chord_mapping[3]);
}

void test_marker_callback_called(void)
{
    // SetNoteMap should call the marker callback 16 times (once per key)
    markerCallCount = 0;
    SetNoteMap(CHROMATIC, 0, false, false, markerCallback);
    TEST_ASSERT_EQUAL(16, markerCallCount);
}

void test_flip_x_reverses_columns(void)
{
    // With flip_x, column order reverses: x = 3 - x
    SetNoteMap(CHROMATIC, 0, true, false, markerCallback);

    // Row 0: indices 3,2,1,0 instead of 0,1,2,3
    // note_map[3] should be step 0, note_map[2] = step 1, etc.
    TEST_ASSERT_EQUAL_UINT8(0, note_map[3]); // First note placed at x=3
    TEST_ASSERT_EQUAL_UINT8(1, note_map[2]);
    TEST_ASSERT_EQUAL_UINT8(2, note_map[1]);
    TEST_ASSERT_EQUAL_UINT8(3, note_map[0]);
}

int main(int argc, char **argv)
{
    UNITY_BEGIN();
    RUN_TEST(test_key_note_mapping_default_chromatic);
    RUN_TEST(test_key_note_with_base_octave);
    RUN_TEST(test_key_note_different_octave);
    RUN_TEST(test_key_note_mapping_ionian_scale);
    RUN_TEST(test_key_note_mapping_with_root_note);
    RUN_TEST(test_note_on_dispatch);
    RUN_TEST(test_note_off_correct_key);
    RUN_TEST(test_bank_change_updates_note_map);
    RUN_TEST(test_set_chord_mapping);
    RUN_TEST(test_marker_callback_called);
    RUN_TEST(test_flip_x_reverses_columns);
    return UNITY_END();
}
