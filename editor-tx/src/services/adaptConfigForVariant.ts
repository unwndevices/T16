import type { T16Configuration } from '@/types/config'
import type { Variant } from '@/types/variant'
import { TOTAL_KEYS_BY_VARIANT } from '@/constants/capabilities'

/**
 * Pure: returns a NEW config adapted to `target` variant. Pads or truncates
 * any per-key arrays and rewrites the `variant` field. Global non-per-key
 * fields (custom_scale1/2 — scale-degree definitions, length always 16 per
 * v201 schema) are passed through unchanged.
 *
 * If `config.variant === target`, returns the config unchanged (referential
 * equality preserved — useful as an idempotent guard at call sites).
 *
 * v201 has no per-key arrays beyond the implicit grid coordinates;
 * custom_scale1/2 are fixed-length 16 by schema. Adapt = variant rewrite.
 * When v202+ introduces per-key note maps / scales, pad/truncate logic
 * lands HERE — adapt site is single-point-of-change.
 */
export function adaptConfigForVariant(config: T16Configuration, target: Variant): T16Configuration {
  if (config.variant === target) return config

  const targetKeys = TOTAL_KEYS_BY_VARIANT[target]
  // Reserved for future per-key array padding/truncation in v202+.
  void targetKeys

  return { ...config, variant: target }
}
