import { describe, it, expect } from 'vitest'
import { adaptConfigForVariant } from '@/services/adaptConfigForVariant'
import type { T16Configuration } from '@/types/config'

const T16_FIXTURE: T16Configuration = {
  version: 201,
  variant: 'T16',
  global: {
    mode: 0,
    sensitivity: 1,
    brightness: 1,
    midi_trs: 0,
    trs_type: 0,
    passthrough: 0,
    midi_ble: 0,
    custom_scale1: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15],
    custom_scale2: [0, 3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45],
  },
  banks: [
    {
      ch: 1, scale: 0, oct: 0, note: 0, vel: 0, at: 0,
      flip_x: 0, flip_y: 0, koala_mode: 0, pal: 0,
      chs: [1, 1, 1, 1, 1, 1, 1, 1],
      ids: [13, 14, 15, 16, 17, 18, 19, 20],
    },
    {
      ch: 1, scale: 0, oct: 0, note: 0, vel: 0, at: 0,
      flip_x: 0, flip_y: 0, koala_mode: 0, pal: 1,
      chs: [1, 1, 1, 1, 1, 1, 1, 1],
      ids: [21, 22, 23, 24, 25, 26, 27, 28],
    },
    {
      ch: 1, scale: 0, oct: 0, note: 0, vel: 0, at: 0,
      flip_x: 0, flip_y: 0, koala_mode: 0, pal: 2,
      chs: [1, 1, 1, 1, 1, 1, 1, 1],
      ids: [21, 22, 23, 24, 25, 26, 27, 28],
    },
    {
      ch: 1, scale: 0, oct: 0, note: 0, vel: 0, at: 0,
      flip_x: 0, flip_y: 0, koala_mode: 0, pal: 3,
      chs: [1, 1, 1, 1, 1, 1, 1, 1],
      ids: [31, 32, 33, 34, 35, 36, 37, 38],
    },
  ],
}

describe('adaptConfigForVariant', () => {
  it('Idempotent same-variant: returns same reference', () => {
    expect(adaptConfigForVariant(T16_FIXTURE, 'T16')).toBe(T16_FIXTURE)
  })

  it('T16 → T32 rewrites variant field only (v201 schema)', () => {
    const result = adaptConfigForVariant(T16_FIXTURE, 'T32')
    expect(result.variant).toBe('T32')
    expect({ ...result, variant: 'T16' as const }).toEqual(T16_FIXTURE)
  })

  it('T32 → T16 rewrites variant field only (v201 schema)', () => {
    const t32 = { ...T16_FIXTURE, variant: 'T32' as const }
    const result = adaptConfigForVariant(t32, 'T16')
    expect(result.variant).toBe('T16')
    expect({ ...result, variant: 'T32' as const }).toEqual(t32)
  })

  it('No mutation of input config across multiple calls', () => {
    const snapshot = JSON.parse(JSON.stringify(T16_FIXTURE))
    adaptConfigForVariant(T16_FIXTURE, 'T32')
    adaptConfigForVariant(T16_FIXTURE, 'T16')
    expect(JSON.parse(JSON.stringify(T16_FIXTURE))).toEqual(snapshot)
  })

  it('round-trip is identity: adapt(adapt(c, T32), T16) deep-equals c', () => {
    const there = adaptConfigForVariant(T16_FIXTURE, 'T32')
    const back = adaptConfigForVariant(there, 'T16')
    expect(back).toEqual(T16_FIXTURE)
  })
})
