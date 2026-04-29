import { describe, it, expect } from 'vitest'
import {
  validateConfig,
  migrateV103,
  migrateV200ToV201,
  adaptConfigForVariant,
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
    const bad = { version: 201, variant: 'T16', banks: DEFAULT_CONFIG.banks }
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

  it('validates config with pal field from device dump', () => {
    const config = structuredClone(DEFAULT_CONFIG)
    config.banks[0].pal = 0
    config.banks[1].pal = 1
    config.banks[2].pal = 2
    config.banks[3].pal = 3
    const result = validateConfig(config)
    expect(result.valid).toBe(true)
    expect(result.errors).toEqual([])
  })

  it('rejects config with pal out of range', () => {
    const config = structuredClone(DEFAULT_CONFIG)
    ;(config.banks[0] as Record<string, unknown>).pal = 10
    const result = validateConfig(config)
    expect(result.valid).toBe(false)
    expect(result.errors.some(e => e.field.includes('banks'))).toBe(true)
  })

  it('rejects a config missing the variant field', () => {
    const bad = structuredClone(DEFAULT_CONFIG) as unknown as Record<string, unknown>
    delete bad.variant
    const result = validateConfig(bad)
    expect(result.valid).toBe(false)
    expect(result.errors.some(e => e.field === 'variant')).toBe(true)
  })

  it('rejects a config with an out-of-enum variant', () => {
    const bad = structuredClone(DEFAULT_CONFIG) as unknown as Record<string, unknown>
    bad.variant = 'T64'
    const result = validateConfig(bad)
    expect(result.valid).toBe(false)
    expect(result.errors.some(e => e.field === 'variant' || e.field.includes('variant'))).toBe(true)
  })

  it('accepts a valid v201 T32 config', () => {
    const config = structuredClone(DEFAULT_CONFIG)
    ;(config as unknown as Record<string, unknown>).variant = 'T32'
    const result = validateConfig(config)
    expect(result.valid).toBe(true)
    expect(result.errors).toEqual([])
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
      { ch: 1, scale: 0, oct: 0, note: 0, vel: 0, at: 0, flip_x: 0, flip_y: 0, koala_mode: 0, pal: 0, chs: [1,1,1,1,1,1,1,1], ids: [13,14,15,16,17,18,19,20] },
      { ch: 1, scale: 0, oct: 0, note: 0, vel: 0, at: 0, flip_x: 0, flip_y: 0, koala_mode: 0, pal: 1, chs: [1,1,1,1,1,1,1,1], ids: [21,22,23,24,25,26,27,28] },
      { ch: 1, scale: 0, oct: 0, note: 0, vel: 0, at: 0, flip_x: 0, flip_y: 0, koala_mode: 0, pal: 2, chs: [1,1,1,1,1,1,1,1], ids: [21,22,23,24,25,26,27,28] },
      { ch: 1, scale: 0, oct: 0, note: 0, vel: 0, at: 0, flip_x: 0, flip_y: 0, koala_mode: 0, pal: 3, chs: [1,1,1,1,1,1,1,1], ids: [31,32,33,34,35,36,37,38] },
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

  it('migrateV103 adds pal defaults to banks', () => {
    // Create a v103 config without pal in banks
    const noPalConfig = {
      ...v103Config,
      banks: v103Config.banks.map(({ pal: _pal, ...rest }) => rest),
    }
    const result = migrateV103(noPalConfig)
    expect(result).not.toBeNull()
    expect(result!.banks[0].pal).toBe(0)
    expect(result!.banks[1].pal).toBe(1)
    expect(result!.banks[2].pal).toBe(2)
    expect(result!.banks[3].pal).toBe(3)
  })
})

describe('migrateV200ToV201', () => {
  const v200Config = (): Record<string, unknown> => {
    const cloned = structuredClone(DEFAULT_CONFIG) as unknown as Record<string, unknown>
    cloned.version = 200
    delete cloned.variant
    return cloned
  }

  it('migrates a valid v200 config to v201 with default variant T16', () => {
    const result = migrateV200ToV201(v200Config())
    expect(result).not.toBeNull()
    expect(result!.version).toBe(201)
    expect(result!.variant).toBe('T16')
  })

  it('respects an explicit T32 default variant', () => {
    const result = migrateV200ToV201(v200Config(), 'T32')
    expect(result).not.toBeNull()
    expect(result!.variant).toBe('T32')
  })

  it('returns null for non-v200 input (v201)', () => {
    const v201 = structuredClone(DEFAULT_CONFIG) as unknown as Record<string, unknown>
    expect(migrateV200ToV201(v201)).toBeNull()
  })

  it('returns null for non-v200 input (v103)', () => {
    expect(migrateV200ToV201({ version: 103 })).toBeNull()
  })

  it('preserves global and banks fields verbatim', () => {
    const input = v200Config()
    const result = migrateV200ToV201(input)
    expect(result).not.toBeNull()
    expect(result!.global).toEqual(input.global)
    expect(result!.banks).toEqual(input.banks)
  })
})

describe('adaptConfigForVariant', () => {
  it('returns the same config when target variant matches current', () => {
    const input = structuredClone(DEFAULT_CONFIG)
    const result = adaptConfigForVariant(input, 'T16')
    expect(result.variant).toBe('T16')
    expect(result.global).toEqual(input.global)
  })

  it('rewrites variant to T32 when adapting from T16', () => {
    const input = structuredClone(DEFAULT_CONFIG)
    const result = adaptConfigForVariant(input, 'T32')
    expect(result.variant).toBe('T32')
    expect(result).not.toBe(input)
  })

  it('rewrites variant to T16 when adapting from T32', () => {
    const input = structuredClone(DEFAULT_CONFIG)
    ;(input as unknown as Record<string, unknown>).variant = 'T32'
    const result = adaptConfigForVariant(input, 'T16')
    expect(result.variant).toBe('T16')
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
      { ch: 1, scale: 0, oct: 0, note: 0, vel: 0, at: 0, flip_x: 0, flip_y: 0, koala_mode: 0, pal: 0, chs: [1,1,1,1,1,1,1,1], ids: [13,14,15,16,17,18,19,20] },
      { ch: 1, scale: 0, oct: 0, note: 0, vel: 0, at: 0, flip_x: 0, flip_y: 0, koala_mode: 0, pal: 1, chs: [1,1,1,1,1,1,1,1], ids: [21,22,23,24,25,26,27,28] },
      { ch: 1, scale: 0, oct: 0, note: 0, vel: 0, at: 0, flip_x: 0, flip_y: 0, koala_mode: 0, pal: 2, chs: [1,1,1,1,1,1,1,1], ids: [21,22,23,24,25,26,27,28] },
      { ch: 1, scale: 0, oct: 0, note: 0, vel: 0, at: 0, flip_x: 0, flip_y: 0, koala_mode: 0, pal: 3, chs: [1,1,1,1,1,1,1,1], ids: [31,32,33,34,35,36,37,38] },
    ],
  }

  it('runs migration then validation for version < 200, returns valid for well-formed v103', () => {
    const result = prepareImport(v103Config)
    expect(result.valid).toBe(true)
    expect(result.migrated).toBe(true)
    expect(result.config).not.toBeNull()
    expect(result.config!.version).toBe(201)
    expect(result.config!.variant).toBe('T16')
  })

  it('skips migration for version === 201, just validates', () => {
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

describe('prepareImport (v200→v201)', () => {
  it('migrates and validates a v200 input', () => {
    const v200 = structuredClone(DEFAULT_CONFIG) as unknown as Record<string, unknown>
    v200.version = 200
    delete v200.variant
    const result = prepareImport(v200)
    expect(result.valid).toBe(true)
    expect(result.migrated).toBe(true)
    expect(result.config?.version).toBe(201)
    expect(result.config?.variant).toBe('T16')
  })

  it('rejects a v202 forward-version input', () => {
    const v202 = { ...structuredClone(DEFAULT_CONFIG), version: 202 } as Record<string, unknown>
    const result = prepareImport(v202)
    expect(result.valid).toBe(false)
    expect(result.errors.some(e => e.field === 'version')).toBe(true)
    expect(
      result.errors.some(e => /202/.test(e.message) && /not supported/i.test(e.message)),
    ).toBe(true)
  })

  it('chains v103 → v200 → v201', () => {
    const v103 = {
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
        { ch: 1, scale: 0, oct: 0, note: 0, vel: 0, at: 0, flip_x: 0, flip_y: 0, koala_mode: 0, pal: 0, chs: [1,1,1,1,1,1,1,1], ids: [13,14,15,16,17,18,19,20] },
        { ch: 1, scale: 0, oct: 0, note: 0, vel: 0, at: 0, flip_x: 0, flip_y: 0, koala_mode: 0, pal: 1, chs: [1,1,1,1,1,1,1,1], ids: [21,22,23,24,25,26,27,28] },
        { ch: 1, scale: 0, oct: 0, note: 0, vel: 0, at: 0, flip_x: 0, flip_y: 0, koala_mode: 0, pal: 2, chs: [1,1,1,1,1,1,1,1], ids: [21,22,23,24,25,26,27,28] },
        { ch: 1, scale: 0, oct: 0, note: 0, vel: 0, at: 0, flip_x: 0, flip_y: 0, koala_mode: 0, pal: 3, chs: [1,1,1,1,1,1,1,1], ids: [31,32,33,34,35,36,37,38] },
      ],
    }
    const result = prepareImport(v103)
    expect(result.valid).toBe(true)
    expect(result.migrated).toBe(true)
    expect(result.config?.version).toBe(201)
    expect(result.config?.variant).toBe('T16')
  })
})
