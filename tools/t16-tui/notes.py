"""Note mapping and scale logic — ported from firmware Scales.cpp"""

SCALES = [
    [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11],  # CHROMATIC
    [0, 2, 4, 5, 7, 9, 11],                     # IONIAN
    [0, 2, 3, 5, 7, 9, 10],                     # DORIAN
    [0, 1, 3, 5, 7, 8, 10],                     # PHRYGIAN
    [0, 2, 4, 6, 7, 9, 11],                     # LYDIAN
    [0, 2, 4, 5, 7, 9, 10],                     # MIXOLYDIAN
    [0, 2, 3, 5, 7, 8, 10],                     # AEOLIAN
    [0, 1, 3, 5, 6, 8, 10],                     # LOCRIAN
    [0, 2, 4, 7, 9],                             # MAJOR PENTATONIC
    [0, 3, 5, 7, 10],                            # MINOR PENTATONIC
    [0, 3, 5, 6, 7, 10],                         # BLUES
    [0, 2, 4, 6, 8, 10],                         # WHOLE TONE
    [0, 3, 4, 7, 8, 11],                         # AUGMENTED
    [0, 2, 3, 5, 6, 8, 9, 11],                  # DIMINISHED
    [0, 2, 3, 5, 7, 8, 11],                     # HARMONIC MINOR
    [0, 2, 3, 5, 7, 9, 11],                     # MELODIC MINOR
    [0, 1, 5, 7, 8],                             # JAPANESE
]

SCALE_NAMES = [
    "Chromatic", "Ionian", "Dorian", "Phrygian", "Lydian",
    "Mixolydian", "Aeolian", "Locrian", "Maj Penta", "Min Penta",
    "Blues", "Whole Tone", "Augmented", "Diminished", "Harm Minor",
    "Mel Minor", "Japanese",
]

NOTE_NAMES = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']

MODE_NAMES = ["Keyboard", "Strum", "XY Pad", "Strips", "Settings"]

VEL_CURVES = ["Linear", "Exponent", "Logarithm", "Quadratic"]


def compute_note_map(scale_idx, root_note, flip_x=False, flip_y=False,
                     cols=4, rows=4, page=0):
    """Port of SetNoteMap() from Scales.cpp"""
    scale = SCALES[scale_idx]
    scale_len = len(scale)
    num_keys = cols * rows
    note_map = [0] * num_keys

    offset = (page * num_keys) % scale_len
    note_index = offset
    octave = 0

    for i in range(num_keys):
        if note_index >= scale_len:
            note_index = 0
            octave += 1

        x = i % cols
        y = i // cols
        if flip_x:
            x = (cols - 1) - x
        if flip_y:
            y = (rows - 1) - y
        index = y * cols + x

        note_map[index] = root_note + scale[note_index] + (octave * 12)
        note_index += 1

    return note_map


def final_midi(note_map_val, base_octave, koala=False):
    """Compute final MIDI note — matches firmware InputProcessor.cpp:48"""
    oct = base_octave * (16 if koala else 12)
    return note_map_val + oct + 24


def format_note(midi):
    """MIDI note number to display string like 'C 3' or 'F#3'"""
    if midi < 0 or midi > 127:
        return "---"
    name = NOTE_NAMES[midi % 12].ljust(2)
    octave = midi // 12 - 1
    return f"{name}{octave}"
