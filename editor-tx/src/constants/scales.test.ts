import { describe, it, expect } from 'vitest'
import {
  SCALE_INTERVALS,
  SCALES,
  NOTE_NAMES,
  getNoteNameWithOctave,
  computeNoteMap,
  getScaleDegree,
} from '@/constants/scales'

describe('SCALE_INTERVALS', () => {
  it('has exactly 19 entries', () => {
    expect(SCALE_INTERVALS).toHaveLength(19)
  })

  it('Chromatic has 12 elements: [0..11]', () => {
    expect(SCALE_INTERVALS[0]).toEqual([0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11])
  })

  it('Ionian has 7 elements: [0,2,4,5,7,9,11]', () => {
    expect(SCALE_INTERVALS[1]).toEqual([0, 2, 4, 5, 7, 9, 11])
  })

  it('Minor Pentatonic has 5 elements: [0,3,5,7,10]', () => {
    expect(SCALE_INTERVALS[9]).toEqual([0, 3, 5, 7, 10])
  })

  it('Custom scales (17, 18) have 16 elements of zeros', () => {
    expect(SCALE_INTERVALS[17]).toHaveLength(16)
    expect(SCALE_INTERVALS[18]).toHaveLength(16)
    expect(SCALE_INTERVALS[17].every((v) => v === 0)).toBe(true)
  })
})

describe('SCALES', () => {
  it('has 19 scale names', () => {
    expect(SCALES).toHaveLength(19)
  })

  it('first is Chromatic, second is Ionian', () => {
    expect(SCALES[0]).toBe('Chromatic')
    expect(SCALES[1]).toBe('Ionian')
  })

  it('matches firmware order: Augmented at 12, Diminished at 13', () => {
    expect(SCALES[12]).toBe('Augmented')
    expect(SCALES[13]).toBe('Diminished')
  })
})

describe('NOTE_NAMES', () => {
  it('has 12 note names starting with C', () => {
    expect(NOTE_NAMES).toHaveLength(12)
    expect(NOTE_NAMES[0]).toBe('C')
    expect(NOTE_NAMES[11]).toBe('B')
  })
})

describe('getNoteNameWithOctave', () => {
  it('returns C4 for MIDI note 60', () => {
    expect(getNoteNameWithOctave(60)).toBe('C4')
  })

  it('returns A4 for MIDI note 69', () => {
    expect(getNoteNameWithOctave(69)).toBe('A4')
  })

  it('returns C-1 for MIDI note 0', () => {
    expect(getNoteNameWithOctave(0)).toBe('C-1')
  })

  it('returns C2 for MIDI note 24', () => {
    expect(getNoteNameWithOctave(24)).toBe('C2')
  })
})

describe('computeNoteMap', () => {
  it('Chromatic from C2 no flip: sequential from 24', () => {
    const result = computeNoteMap(0, 0, 2, false, false)
    // Chromatic: 12 notes then wraps. From C2 (24):
    // i=0..11 -> notes 24..35, i=12..15 -> octave wraps -> 36..39
    expect(result[0]).toBe(24) // C2
    expect(result[1]).toBe(25) // C#2
    expect(result[2]).toBe(26) // D2
    expect(result[3]).toBe(27) // D#2
    // Check last 4 wrap to next octave
    expect(result[12]).toBe(36) // C3
    expect(result[15]).toBe(39) // D#3
  })

  it('Ionian from C2 no flip: correct major scale intervals', () => {
    const result = computeNoteMap(1, 0, 2, false, false)
    // Ionian intervals: [0,2,4,5,7,9,11], 7 notes, wraps at 7
    // i=0: 24+0=24 (C2), i=1: 24+2=26 (D2), i=2: 24+4=28 (E2)
    // i=3: 24+5=29 (F2), i=4: 24+7=31 (G2), i=5: 24+9=33 (A2)
    // i=6: 24+11=35 (B2), i=7: octave++ -> 24+0+12=36 (C3)
    // i=8: 24+2+12=38 (D3), ...
    expect(result).toEqual([24, 26, 28, 29, 31, 33, 35, 36, 38, 40, 41, 43, 45, 47, 48, 50])
  })

  it('flip_x mirrors columns within each row', () => {
    const normal = computeNoteMap(0, 0, 2, false, false)
    const flipped = computeNoteMap(0, 0, 2, true, false)
    // Row 0: normal indices 0,1,2,3 -> flipped indices 3,2,1,0
    expect(flipped[3]).toBe(normal[0])
    expect(flipped[2]).toBe(normal[1])
    expect(flipped[1]).toBe(normal[2])
    expect(flipped[0]).toBe(normal[3])
  })

  it('flip_y mirrors rows', () => {
    const normal = computeNoteMap(0, 0, 2, false, false)
    const flipped = computeNoteMap(0, 0, 2, false, true)
    // Row 0 (indices 0-3) goes to row 3 (indices 12-15)
    expect(flipped[12]).toBe(normal[0])
    expect(flipped[13]).toBe(normal[1])
    expect(flipped[14]).toBe(normal[2])
    expect(flipped[15]).toBe(normal[3])
  })

  it('Minor Pentatonic from E3: correct intervals', () => {
    // Minor Pentatonic: [0,3,5,7,10], 5 notes
    // Root E=4, oct=3 -> base = 3*12 + 4 = 40 (E3)
    const result = computeNoteMap(9, 4, 3, false, false)
    // i=0: 40+0=40(E3), i=1: 40+3=43(G3), i=2: 40+5=45(A3)
    // i=3: 40+7=47(B3), i=4: 40+10=50(D4)
    // i=5: wraps -> 40+0+12=52(E4), i=6: 40+3+12=55(G4)
    // i=7: 40+5+12=57(A4), i=8: 40+7+12=59(B4), i=9: 40+10+12=62(D5)
    // i=10: wraps again -> 40+0+24=64(E5), ...
    expect(result[0]).toBe(40) // E3
    expect(result[1]).toBe(43) // G3
    expect(result[2]).toBe(45) // A3
    expect(result[3]).toBe(47) // B3
    expect(result[4]).toBe(50) // D4
    expect(result[5]).toBe(52) // E4
  })

  it('both flips active reverses entire grid', () => {
    const normal = computeNoteMap(1, 0, 2, false, false)
    const flipped = computeNoteMap(1, 0, 2, true, true)
    // i=0 normal -> index 0, but with both flips: x=3-0=3, y=3-0=3 -> index 15
    // So note assigned at i=0 ends up at index 15 in flipped
    expect(flipped[15]).toBe(normal[0])
    expect(flipped[0]).toBe(normal[15])
  })
})

describe('getScaleDegree', () => {
  const ionianIntervals = [0, 2, 4, 5, 7, 9, 11]

  it('returns 0 for root note (C in C Ionian)', () => {
    expect(getScaleDegree(60, 0, ionianIntervals)).toBe(0) // C4 in C Ionian
  })

  it('returns correct degree for D in C Ionian', () => {
    expect(getScaleDegree(62, 0, ionianIntervals)).toBe(1) // D is 2nd degree
  })

  it('returns correct degree for E in C Ionian', () => {
    expect(getScaleDegree(64, 0, ionianIntervals)).toBe(2) // E is 3rd degree
  })

  it('returns null for C# in C Ionian (not in scale)', () => {
    expect(getScaleDegree(61, 0, ionianIntervals)).toBeNull()
  })

  it('works across octaves', () => {
    expect(getScaleDegree(72, 0, ionianIntervals)).toBe(0) // C5 is still root
  })

  it('handles non-C root notes', () => {
    // G major = Ionian from G (root=7)
    expect(getScaleDegree(67, 7, ionianIntervals)).toBe(0) // G4 is root
    expect(getScaleDegree(69, 7, ionianIntervals)).toBe(1) // A4 is 2nd degree (semitone 2 from G)
  })
})
