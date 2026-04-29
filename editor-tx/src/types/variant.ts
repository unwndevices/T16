// Variant discriminator used across config, context, and SysEx capabilities handshake.
// Mirrors schema/t16-config.schema.json `variant` enum.
export type Variant = 'T16' | 'T32'

// Hardware capability flags reported by the firmware via CMD_CAPABILITIES (Phase 11).
export type Capabilities = {
  touchSlider: boolean
  koalaMode: boolean
}

export const ALL_VARIANTS: readonly Variant[] = ['T16', 'T32'] as const

export function isVariant(value: unknown): value is Variant {
  return value === 'T16' || value === 'T32'
}
