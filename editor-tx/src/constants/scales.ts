/**
 * Scale interval data and note mapping utilities.
 *
 * Ported from firmware src/Scales.cpp — intervals match the firmware
 * definition exactly (with -1 sentinels removed; array length encodes
 * scale size instead).
 */

/** Intervals for each of the 19 scales, indexed by Scale enum value. */
export const SCALE_INTERVALS: readonly (readonly number[])[] = [
  [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11], // CHROMATIC
  [0, 2, 4, 5, 7, 9, 11], // IONIAN
  [0, 2, 3, 5, 7, 9, 10], // DORIAN
  [0, 1, 3, 5, 7, 8, 10], // PHRYGIAN
  [0, 2, 4, 6, 7, 9, 11], // LYDIAN
  [0, 2, 4, 5, 7, 9, 10], // MIXOLYDIAN
  [0, 2, 3, 5, 7, 8, 10], // AEOLIAN
  [0, 1, 3, 5, 6, 8, 10], // LOCRIAN
  [0, 2, 4, 7, 9], // MAJOR_PENTATONIC
  [0, 3, 5, 7, 10], // MINOR_PENTATONIC
  [0, 3, 5, 6, 7, 10], // BLUES
  [0, 2, 4, 6, 8, 10], // WHOLE_TONE
  [0, 3, 4, 7, 8, 11], // AUGMENTED
  [0, 2, 3, 5, 6, 8, 9, 11], // DIMINISHED
  [0, 2, 3, 5, 7, 8, 11], // HARMONIC_MINOR
  [0, 2, 3, 5, 7, 9, 11], // MELODIC_MINOR
  [0, 1, 5, 7, 8], // JAPANESE
  [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0], // CUSTOM1
  [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0], // CUSTOM2
] as const

/** Human-readable scale names, matching firmware enum order. */
export const SCALES: readonly string[] = [
  'Chromatic',
  'Ionian',
  'Dorian',
  'Phrygian',
  'Lydian',
  'Mixolydian',
  'Aeolian',
  'Locrian',
  'Major Pentatonic',
  'Minor Pentatonic',
  'Blues',
  'Whole Tone',
  'Augmented',
  'Diminished',
  'Harmonic Minor',
  'Melodic Minor',
  'Japanese',
  'Custom Scale 1',
  'Custom Scale 2',
] as const

/** Chromatic note names (C = 0, B = 11). */
export const NOTE_NAMES: readonly string[] = [
  'C',
  'C#',
  'D',
  'D#',
  'E',
  'F',
  'F#',
  'G',
  'G#',
  'A',
  'A#',
  'B',
] as const

/**
 * Convert a MIDI note number to a human-readable name with octave.
 * MIDI note 60 = C4, note 0 = C-1.
 */
export function getNoteNameWithOctave(midiNote: number): string {
  const name = NOTE_NAMES[midiNote % 12]
  const octave = Math.floor(midiNote / 12) - 1
  return `${name}${octave}`
}

/**
 * Compute the per-key note map for a given scale configuration.
 *
 * Port of firmware `SetNoteMap()` from `src/Scales.cpp` with `page = 0`.
 * Returns a TOTAL_KEYS-element array where `result[gridIndex]` is the MIDI note number.
 *
 * The grid is always 4 columns wide (T16 = 4×4, T32 = 4×8) — `cols = 4` is
 * an invariant per Phase 14 D14.3 (physical mapping). `totalKeys` controls
 * the row count via `rows = totalKeys / 4`.
 */
export function computeNoteMap(
  scaleIndex: number,
  rootNote: number,
  octaveOffset: number,
  flipX: boolean,
  flipY: boolean,
  customScale1?: readonly number[],
  customScale2?: readonly number[],
  totalKeys: number = 16,
): number[] {
  // Select intervals — use custom scale data for indices 17/18 if provided
  let intervals: readonly number[]
  if (scaleIndex === 17 && customScale1) {
    intervals = customScale1
  } else if (scaleIndex === 18 && customScale2) {
    intervals = customScale2
  } else {
    intervals = SCALE_INTERVALS[scaleIndex]
  }

  const scaleLength = intervals.length
  const baseNote = octaveOffset * 12 + rootNote
  const noteMap = new Array<number>(totalKeys)
  const cols = 4
  const rows = totalKeys / cols
  const lastRow = rows - 1

  let noteIndex = 0
  let octave = 0

  for (let i = 0; i < totalKeys; i++) {
    // Wrap at end of scale (mirrors firmware -1 sentinel check)
    if (noteIndex >= scaleLength) {
      noteIndex = 0
      octave++
    }

    // Grid position with optional axis flipping
    let x = i % cols
    let y = Math.floor(i / cols)
    if (flipX) x = cols - 1 - x
    if (flipY) y = lastRow - y
    const index = y * cols + x

    noteMap[index] = baseNote + intervals[noteIndex] + octave * 12
    noteIndex++
  }

  return noteMap
}

/**
 * Determine the scale degree of a MIDI note within a given scale.
 *
 * Returns the 0-based degree index (0 = root), or `null` if the note
 * is not part of the scale.
 */
export function getScaleDegree(
  midiNote: number,
  rootNote: number,
  intervals: readonly number[],
): number | null {
  // Reduce both the lookup key and each interval to mod-12 so multi-octave
  // custom scales (e.g. custom_scale2 default = [0, 3, 6, ..., 45]) match
  // their pitch class. Without this, indexOf(0..11) misses any interval
  // value >= 12 and the NoteGrid renders in-scale notes as out-of-scale
  // (gray/muted), losing the hue ramp. Fix per WR-02 (Phase 14 review).
  const semitoneFromRoot = (((midiNote - rootNote) % 12) + 12) % 12
  const degree = intervals.findIndex((iv) => ((iv % 12) + 12) % 12 === semitoneFromRoot)
  return degree === -1 ? null : degree
}
