#pragma once

#include <stdint.h>
#include <functional>

enum Scale
{
    CHROMATIC = 0,
    IONIAN,
    DORIAN,
    PHRYGIAN,
    LYDIAN,
    MIXOLYDIAN,
    AEOLIAN,
    LOCRIAN,
    MAJOR_PENTATONIC,
    MINOR_PENTATONIC,
    BLUES,
    WHOLE_TONE,
    AUGMENTED,
    DIMINISHED,
    HARMONIC_MINOR,
    MELODIC_MINOR,
    JAPANESE,
    CUSTOM1,
    CUSTOM2,
    SCALE_AMOUNT // Keep this last
};

enum Chord
{
    MAJOR = 0,        // Major chord: root, major third, perfect fifth
    MINOR,            // Minor chord: root, minor third, perfect fifth
    SUSPENDED_FOURTH, // Suspended fourth chord: root, perfect fourth, perfect fifth
    DIMINISH,         // Diminished chord: root, minor third, diminished fifth
    CHORD_AMOUNT
};

// Data arrays — defined in Scales.cpp
extern int8_t scales[SCALE_AMOUNT][16];
extern int8_t chords[CHORD_AMOUNT][4];
extern int8_t strum_chords[CHORD_AMOUNT][7];
extern int8_t scale_chord_map[SCALE_AMOUNT][8];
extern uint8_t note_map[16];
extern uint8_t current_chord_mapping[12];

// Functions — implemented in Scales.cpp
void SetNoteMap(uint8_t scale, uint8_t root_note, bool flip_x, bool flip_y, std::function<void(uint8_t, bool)> markerCallback, uint8_t page = 0);
void SetChordMapping(uint8_t scale);
