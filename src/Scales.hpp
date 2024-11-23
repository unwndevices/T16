#ifndef SCALES_HPP
#define SCALES_HPP

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

// Scales are defined as steps from the root note, 0 being the root note and -1 being the end of the scale (used for scales with less than 12 notes)
int8_t scales[SCALE_AMOUNT][16] = {
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, -1},       // CHROMATIC
    {0, 2, 4, 5, 7, 9, 11, -1, -1, -1, -1, -1},       // IONIAN
    {0, 2, 3, 5, 7, 9, 10, -1, -1, -1, -1, -1},       // DORIAN
    {0, 1, 3, 5, 7, 8, 10, -1, -1, -1, -1, -1},       // PHRYGIAN
    {0, 2, 4, 6, 7, 9, 11, -1, -1, -1, -1, -1},       // LYDIAN
    {0, 2, 4, 5, 7, 9, 10, -1, -1, -1, -1, -1},       // MIXOLYDIAN
    {0, 2, 3, 5, 7, 8, 10, -1, -1, -1, -1, -1},       // AEOLIAN
    {0, 1, 3, 5, 6, 8, 10, -1, -1, -1, -1, -1},       // LOCRIAN
    {0, 2, 4, 7, 9, -1, -1, -1, -1, -1, -1, -1},      // MAJOR_PENTATONIC
    {0, 3, 5, 7, 10, -1, -1, -1, -1, -1, -1, -1},     // MINOR_PENTATONIC
    {0, 3, 5, 6, 7, 10, -1, -1, -1, -1, -1, -1},      // BLUES
    {0, 2, 4, 6, 8, 10, -1, -1, -1, -1, -1, -1},      // WHOLE_TONE
    {0, 3, 4, 7, 8, 11, -1, -1, -1, -1, -1, -1},      // AUGMENTED
    {0, 2, 3, 5, 6, 8, 9, 11, -1, -1, -1, -1},        // DIMINISHED
    {0, 2, 3, 5, 7, 8, 11, -1, -1, -1, -1, -1},       // HARMONIC_MINOR
    {0, 2, 3, 5, 7, 9, 11, -1, -1, -1, -1, -1},       // MELODIC_MINOR
    {0, 1, 5, 7, 8, -1, -1, -1, -1, -1, -1, -1},      // JAPANESE
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // CUSTOM1 Placeholder
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}  // CUSTOM2 Placeholder
};

enum Chord
{
    MAJOR = 0,        // Major chord: root, major third, perfect fifth
    MINOR,            // Minor chord: root, minor third, perfect fifth
    SUSPENDED_FOURTH, // Suspended fourth chord: root, perfect fourth, perfect fifth
    DIMINISH,         // Diminished chord: root, minor third, diminished fifth
    CHORD_AMOUNT
};

int8_t chords[CHORD_AMOUNT][4] = {
    {0, 4, 7, -1}, // MAJOR
    {0, 3, 7, -1}, // MINOR
    {0, 3, 6, 10}, // SUSPENDED_FOURTH
    {0, 3, 6, -1}, // DIMINISH
};

int8_t strum_chords[CHORD_AMOUNT][7] = {
    {0, 4, 7, 12, 16, 19, 24}, // MAJOR
    {0, 3, 7, 12, 15, 19, 24}, // MINOR
    {0, 3, 6, 10, 12, 15, 18}, // SUSPENDED_FOURTH
    {0, 3, 6, 12, 15, 18, 24}, // DIMINISH
};

int8_t scale_chord_map[SCALE_AMOUNT][8] = {
    {0, 0, 0, 0, 0, 0, 0, 0},                             // CHROMATIC
    {MAJOR, MINOR, MINOR, MAJOR, MAJOR, MINOR, DIMINISH}, // IONIAN (Major Scale)
    {MINOR, MINOR, MAJOR, MAJOR, MINOR, DIMINISH, MAJOR}, // DORIAN
    {MINOR, MAJOR, MAJOR, MINOR, DIMINISH, MAJOR, MINOR}, // PHRYGIAN
    {MAJOR, MAJOR, MINOR, DIMINISH, MAJOR, MINOR, MINOR}, // LYDIAN
    {MAJOR, MINOR, DIMINISH, MAJOR, MINOR, MINOR, MAJOR}, // MIXOLYDIAN
    {MINOR, DIMINISH, MAJOR, MINOR, MINOR, MAJOR, MAJOR}, // AEOLIAN (Natural Minor Scale)
    {DIMINISH, MAJOR, MINOR, MINOR, MAJOR, MAJOR, MINOR}, // LOCRIAN
    {MAJOR, MINOR, MAJOR, MAJOR, MINOR, MAJOR, MINOR},    // MAJOR_PENTATONIC
    {MINOR, MAJOR, MINOR, MINOR, MAJOR, MINOR, MAJOR},    // MINOR_PENTATONIC
    {MINOR, MAJOR, MINOR, MINOR, MAJOR, MINOR, MAJOR},    // BLUES
    {MAJOR, MINOR, MINOR, MAJOR, MINOR, MAJOR, MAJOR},    // WHOLE_TONE
    {MAJOR, MAJOR, MINOR, MINOR, MAJOR, MINOR, MINOR},    // AUGMENTED
    {MINOR, MINOR, MAJOR, MAJOR, MINOR, MAJOR, MINOR},    // DIMINISHED
    {MINOR, MAJOR, MINOR, MAJOR, MINOR, MAJOR, MINOR},    // HARMONIC_MINOR
    {MAJOR, MINOR, MAJOR, MINOR, MAJOR, MINOR, MAJOR},    // MELODIC_MINOR
    {MAJOR, MINOR, MAJOR, MINOR, MAJOR, MINOR, MAJOR},    // JAPANESE
    {0, 0, 0, 0, 0, 0, 0},                                // CUSTOM1 Placeholder
    {0, 0, 0, 0, 0, 0, 0}                                 // CUSTOM2 Placeholder

};

uint8_t note_map[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
uint8_t current_chord_mapping[12]; // Stores the current chord mapping for each of the 12 note keys

void SetNoteMap(uint8_t scale, uint8_t root_note, bool flip_x, bool flip_y, std::function<void(uint8_t, bool)> markerCallback, uint8_t page = 0)
{
    int8_t *scale_notes = scales[scale];
    uint8_t note_index = 0;
    uint8_t octave = 0;
    uint8_t x, y, index;
    bool isRootNote;

    // calculate the offset based on the current page and wrap around based on the scale length (that needs to be found by looking at the first -1 element of the scale)
    uint8_t scale_length = 16;
    for (int i = 0; i < 16; i++)
    {
        if (scale_notes[i] == -1)
        {
            scale_length = i;
            break;
        }
    }

    uint8_t offset = (page * NUM_KEYS) % scale_length;
    note_index = note_index + offset;

    for (int i = 0; i < NUM_KEYS; i++)
    {

        // If we've reached the end of the scale (-1), wrap around to the next octave
        if (scale_notes[note_index] == -1 || note_index == 16)
        {
            note_index = 0;
            octave++;
        }

        // Calculate the x and y coordinates for the current index
        x = i % COLS;
        y = i / COLS;
        // Calculate the correct index based on flip_x and flip_y
        if (flip_x)
            x = COLS - 1 - x;
        if (flip_y)
            y = ROWS - 1 - y;
        index = y * COLS + x;

        // Calculate the note value by adding the scale step to the root note
        // and accounting for the octave wrap-around
        note_map[index] = (root_note + scale_notes[note_index] + (octave * 12));

        isRootNote = (note_index == 0);
        // Use the provided callback to update the marker LEDs
        markerCallback(index, isRootNote);

        note_index++;
    }
}

void SetChordMapping(uint8_t scale)
{
    for (int i = 0; i < 12; i++)
    {
        current_chord_mapping[i] = scale_chord_map[scale][i % 7];
    }
}

#endif // SCALES_HPP
