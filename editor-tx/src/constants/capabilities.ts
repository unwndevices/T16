import type { Variant, Capabilities } from '@/types/variant'

// Per-variant total-key count — single source of truth for editor-side parametric grids.
export const TOTAL_KEYS_BY_VARIANT = {
  T16: 16,
  T32: 32,
} as const satisfies Record<Variant, number>

// Best-effort offline fallback when no device is connected and no handshake has fired.
// Documented as best-effort until handshake confirms (D14.x capability-driven hiding).
export const FALLBACK_CAPABILITIES: Record<Variant, Capabilities> = {
  T16: { touchSlider: true, koalaMode: true },
  T32: { touchSlider: false, koalaMode: false },
}

// localStorage key for remembering the user's last-selected variant when offline.
export const VARIANT_STORAGE_KEY = 't16-editor.lastVariant'
