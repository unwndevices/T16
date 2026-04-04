import { describe, it, expect } from 'vitest'
import {
  validateConfig,
  migrateV103,
  prepareImport,
} from '@/services/configValidator'
import { DEFAULT_CONFIG } from '@/contexts/ConfigContext'

describe('validateConfig', () => {
  it('returns valid for a correct T16Configuration', () => {
    const result = validateConfig(DEFAULT_CONFIG)
    expect(result.valid).toBe(true)
    expect(result.errors).toEqual([])
  })

  it('returns invalid for an object missing required "global" field', () => {
    const bad = { version: 200, banks: DEFAULT_CONFIG.banks }
    const result = validateConfig(bad)
    expect(result.valid).toBe(false)
    expect(result.errors.some(e => e.field === 'global')).toBe(true)
  })

  it('returns field-path errors for invalid bank field value', () => {
    const bad = structuredClone(DEFAULT_CONFIG)
    // Set scale to a string to trigger validation error
    ;(bad.banks[0] as Record<string, unknown>).scale = 'not-a-number'
    const result = validateConfig(bad)
    expect(result.valid).toBe(false)
    expect(result.errors.some(e => e.field.includes('banks'))).toBe(true)
  })

  it('returns invalid for a completely wrong object', () => {
    const result = validateConfig({ foo: 'bar' })
    expect(result.valid).toBe(false)
    expect(result.errors.length).toBeGreaterThan(0)
  })
})

describe('migrateV103', () => {
  const v103Config = {
    version: 103,
    mode: 0,
    sensitivity: 1,
    brightness: 1,
    midi_trs: 0,
    trs_type: 0,
    passthrough: 0,
    midi_ble: 0,
    custom_scale1: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15],
    custom_scale2: [0, 3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45],
    banks: [
      { ch: 1, scale: 0, oct: 0, note: 0, vel: 0, at: 0, flip_x: 0, flip_y: 0, koala_mode: 0, chs: [1,1,1,1,1,1,1,1], ids: [13,14,15,16,17,18,19,20] },
      { ch: 1, scale: 0, oct: 0, note: 0, vel: 0, at: 0, flip_x: 0, flip_y: 0, koala_mode: 0, chs: [1,1,1,1,1,1,1,1], ids: [21,22,23,24,25,26,27,28] },
      { ch: 1, scale: 0, oct: 0, note: 0, vel: 0, at: 0, flip_x: 0, flip_y: 0, koala_mode: 0, chs: [1,1,1,1,1,1,1,1], ids: [21,22,23,24,25,26,27,28] },
      { ch: 1, scale: 0, oct: 0, note: 0, vel: 0, at: 0, flip_x: 0, flip_y: 0, koala_mode: 0, chs: [1,1,1,1,1,1,1,1], ids: [31,32,33,34,35,36,37,38] },
    ],
  }

  it('converts a v103-format config to v200 nested format', () => {
    const result = migrateV103(v103Config)
    expect(result).not.toBeNull()
    expect(result!.version).toBe(200)
    expect(result!.global.mode).toBe(0)
    expect(result!.global.sensitivity).toBe(1)
    expect(result!.global.brightness).toBe(1)
    expect(result!.global.midi_trs).toBe(0)
    expect(result!.global.midi_ble).toBe(0)
  })

  it('preserves bank data during migration', () => {
    const result = migrateV103(v103Config)
    expect(result).not.toBeNull()
    expect(result!.banks).toHaveLength(4)
    expect(result!.banks[0].ch).toBe(1)
    expect(result!.banks[0].ids).toEqual([13,14,15,16,17,18,19,20])
    expect(result!.banks[3].ids).toEqual([31,32,33,34,35,36,37,38])
  })
})

describe('prepareImport', () => {
  const v103Config = {
    version: 103,
    mode: 0,
    sensitivity: 1,
    brightness: 1,
    midi_trs: 0,
    trs_type: 0,
    passthrough: 0,
    midi_ble: 0,
    custom_scale1: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15],
    custom_scale2: [0, 3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45],
    banks: [
      { ch: 1, scale: 0, oct: 0, note: 0, vel: 0, at: 0, flip_x: 0, flip_y: 0, koala_mode: 0, chs: [1,1,1,1,1,1,1,1], ids: [13,14,15,16,17,18,19,20] },
      { ch: 1, scale: 0, oct: 0, note: 0, vel: 0, at: 0, flip_x: 0, flip_y: 0, koala_mode: 0, chs: [1,1,1,1,1,1,1,1], ids: [21,22,23,24,25,26,27,28] },
      { ch: 1, scale: 0, oct: 0, note: 0, vel: 0, at: 0, flip_x: 0, flip_y: 0, koala_mode: 0, chs: [1,1,1,1,1,1,1,1], ids: [21,22,23,24,25,26,27,28] },
      { ch: 1, scale: 0, oct: 0, note: 0, vel: 0, at: 0, flip_x: 0, flip_y: 0, koala_mode: 0, chs: [1,1,1,1,1,1,1,1], ids: [31,32,33,34,35,36,37,38] },
    ],
  }

  it('runs migration then validation for version < 200, returns valid for well-formed v103', () => {
    const result = prepareImport(v103Config)
    expect(result.valid).toBe(true)
    expect(result.migrated).toBe(true)
    expect(result.config).not.toBeNull()
    expect(result.config!.version).toBe(200)
  })

  it('skips migration for version >= 200, just validates', () => {
    const result = prepareImport(DEFAULT_CONFIG)
    expect(result.valid).toBe(true)
    expect(result.migrated).toBe(false)
    expect(result.config).not.toBeNull()
  })

  it('rejects v103 config that fails migration with version-specific error', () => {
    // A v103 config missing required keys for migration
    const broken = { version: 103 }
    const result = prepareImport(broken)
    expect(result.valid).toBe(false)
    expect(result.errors.some(e => e.field === 'version')).toBe(true)
    expect(result.errors.some(e => e.message.includes('103'))).toBe(true)
  })
})
