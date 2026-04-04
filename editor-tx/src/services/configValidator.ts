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

export function migrateV103(data: Record<string, unknown>): T16Configuration | null {
  try {
    // Validate that required flat keys exist for migration
    const hasGlobalKeys = GLOBAL_KEYS.every(
      key => key in data || (key === 'custom_scale1' || key === 'custom_scale2'),
    )
    const hasBanks = Array.isArray(data.banks) && (data.banks as unknown[]).length === 4

    if (!hasGlobalKeys || !hasBanks) {
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
      banks: data.banks as T16Configuration['banks'],
    }
  } catch {
    return null
  }
}

export interface ImportResult {
  valid: boolean
  config: T16Configuration | null
  errors: ValidationError[]
  migrated: boolean
}

export function prepareImport(data: unknown): ImportResult {
  const obj = data as Record<string, unknown>
  const version = typeof obj?.version === 'number' ? obj.version : 200

  if (version < 200) {
    const migrated = migrateV103(obj)
    if (!migrated) {
      return {
        valid: false,
        config: null,
        errors: [{ field: 'version', message: `Config version ${version} could not be migrated` }],
        migrated: false,
      }
    }
    const result = validateConfig(migrated)
    return {
      valid: result.valid,
      config: result.valid ? migrated : null,
      errors: result.errors,
      migrated: true,
    }
  }

  const result = validateConfig(data)
  return {
    valid: result.valid,
    config: result.valid ? (data as T16Configuration) : null,
    errors: result.errors,
    migrated: false,
  }
}
