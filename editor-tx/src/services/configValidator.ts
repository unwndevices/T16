import Ajv from 'ajv/dist/2020'
import schema from '../../../schema/t16-config.schema.json'
import type { T16Configuration } from '@/types/config'

const ajv = new Ajv({ allErrors: true })
const validate = ajv.compile(schema)

export interface ValidationError {
  field: string
  message: string
}

export interface ValidationResult {
  valid: boolean
  errors: ValidationError[]
}

export function validateConfig(data: unknown): ValidationResult {
  const valid = validate(data)
  if (valid) return { valid: true, errors: [] }
  return {
    valid: false,
    errors: (validate.errors ?? []).map(err => ({
      field: err.instancePath
        ? err.instancePath.slice(1).replace(/\//g, '.')
        : (err.params?.missingProperty as string) ?? 'root',
      message: err.message ?? 'validation failed',
    })),
  }
}

const GLOBAL_KEYS = [
  'mode', 'sensitivity', 'brightness', 'midi_trs',
  'trs_type', 'passthrough', 'midi_ble',
  'custom_scale1', 'custom_scale2',
] as const

// Required global keys for v103 → v200 migration. custom_scale1 and
// custom_scale2 are excluded — they are filled with defaults below if missing
// (see WR-05). Splitting the lists makes the predicate's intent explicit and
// prevents future "simplification" from breaking the default-fill guarantee.
const REQUIRED_GLOBAL_KEYS_V103 = GLOBAL_KEYS.filter(
  k => k !== 'custom_scale1' && k !== 'custom_scale2',
)

// Required scalar fields each bank must carry in a v103 file. If any are missing
// the migration returns null rather than producing a partially-typed object that
// would slip past the type cast and only fail later at ajv validation (WR-06).
const REQUIRED_BANK_KEYS_V103 = [
  'ch', 'scale', 'oct', 'note', 'vel', 'at',
  'flip_x', 'flip_y', 'koala_mode', 'chs', 'ids',
] as const

// migrateV103 produces a v200-shaped object (no `variant` field). Callers are
// expected to chain it through migrateV200ToV201 to land on a fully valid
// T16Configuration. The return type is intentionally loose because v200 output
// is not assignable to T16Configuration's `version: 201` literal.
export type V200Config = {
  version: 200
  global: T16Configuration['global']
  banks: T16Configuration['banks']
}

export function migrateV103(data: Record<string, unknown>): V200Config | null {
  try {
    // Validate that required (non-scale) flat keys exist for migration. Custom
    // scales are filled with defaults below, so they are not required here.
    const hasGlobalKeys = REQUIRED_GLOBAL_KEYS_V103.every(key => key in data)
    const hasBanks = Array.isArray(data.banks) && (data.banks as unknown[]).length === 4

    if (!hasGlobalKeys || !hasBanks) {
      return null
    }

    // Validate each bank carries the required scalar fields before the cast.
    // Without this check a v103 file with missing bank fields would produce a
    // partially-shaped object that the type assertion would silently accept
    // and ajv would later reject (WR-06).
    const banks = data.banks as Record<string, unknown>[]
    const allBanksValid = banks.every(bank =>
      REQUIRED_BANK_KEYS_V103.every(key => key in bank),
    )
    if (!allBanksValid) {
      return null
    }

    const global: Record<string, unknown> = {}
    for (const key of GLOBAL_KEYS) {
      if (key in data) {
        global[key] = data[key]
      }
    }

    // Ensure custom scales exist with defaults if missing
    if (!global.custom_scale1) {
      global.custom_scale1 = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15]
    }
    if (!global.custom_scale2) {
      global.custom_scale2 = [0, 3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45]
    }

    return {
      version: 200,
      global: global as T16Configuration['global'],
      banks: banks.map((bank, i) => ({
        ...bank,
        pal: typeof bank.pal === 'number' ? bank.pal : i,
      })) as T16Configuration['banks'],
    }
  } catch {
    return null
  }
}

// migrateV200ToV201 default-injects `variant`. v200 only existed in T16 builds
// (CONTEXT.md D13.2), so 'T16' is the safe default when the caller has no
// device context. Editor-tx may pass an explicit variant when adapting after
// device connection.
//
// Edge: a v200 input with a stray `variant` field is treated as malformed —
// migration overwrites it with the caller's defaultVariant. This is correct
// because `variant` only became part of the schema at v201.
export function migrateV200ToV201(
  data: Record<string, unknown>,
  defaultVariant: 'T16' | 'T32' = 'T16',
): T16Configuration | null {
  if (typeof data.version !== 'number' || data.version !== 200) return null
  const next = { ...data, version: 201, variant: defaultVariant } as unknown as T16Configuration
  return next
}

// adaptConfigForVariant moved to ./adaptConfigForVariant.ts (Phase 14-04). Re-export
// here for backward compatibility with existing imports.
export { adaptConfigForVariant } from './adaptConfigForVariant'

export interface ImportResult {
  valid: boolean
  config: T16Configuration | null
  errors: ValidationError[]
  migrated: boolean
}

export function prepareImport(data: unknown): ImportResult {
  const obj = data as Record<string, unknown>
  const version = typeof obj?.version === 'number' ? obj.version : 201

  // Reject non-integer versions explicitly (WR-04). The dispatch below assumes
  // integer-valued schema versions (200, 201, …); a float like 200.5 would
  // otherwise fall through every branch into the "unreachable" guard with a
  // misleading error message.
  if (!Number.isInteger(version)) {
    return {
      valid: false,
      config: null,
      errors: [{ field: 'version', message: `Config version ${version} is not a valid schema version (must be an integer)` }],
      migrated: false,
    }
  }

  // v201 direct path: validate only
  if (version === 201) {
    const result = validateConfig(data)
    return {
      valid: result.valid,
      config: result.valid ? (data as T16Configuration) : null,
      errors: result.errors,
      migrated: false,
    }
  }

  // Forward-incompatible (D13.2 symmetry — firmware logs+defaults, editor rejects loudly)
  if (version > 201) {
    return {
      valid: false,
      config: null,
      errors: [{ field: 'version', message: `Config version ${version} is not supported by this editor` }],
      migrated: false,
    }
  }

  // v200 → v201
  if (version === 200) {
    const v201 = migrateV200ToV201(obj)
    if (!v201) {
      return { valid: false, config: null, errors: [{ field: 'version', message: 'Failed to migrate v200 → v201' }], migrated: false }
    }
    const result = validateConfig(v201)
    return {
      valid: result.valid,
      config: result.valid ? v201 : null,
      errors: result.errors,
      migrated: true,
    }
  }

  // v1xx → v200 → v201
  if (version < 200) {
    const v200 = migrateV103(obj)
    if (!v200) {
      return { valid: false, config: null, errors: [{ field: 'version', message: `Config version ${version} could not be migrated` }], migrated: false }
    }
    const v201 = migrateV200ToV201(v200 as unknown as Record<string, unknown>)
    if (!v201) {
      return { valid: false, config: null, errors: [{ field: 'version', message: 'Failed v200 → v201 step' }], migrated: false }
    }
    const result = validateConfig(v201)
    return {
      valid: result.valid,
      config: result.valid ? v201 : null,
      errors: result.errors,
      migrated: true,
    }
  }

  // Unreachable, but keep TypeScript exhaustive
  return { valid: false, config: null, errors: [{ field: 'version', message: 'Unhandled version' }], migrated: false }
}
