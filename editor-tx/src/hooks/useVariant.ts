import { useContext } from 'react'
import { ConfigContext } from '@/contexts/ConfigContext'
import { TOTAL_KEYS_BY_VARIANT } from '@/constants/capabilities'

/**
 * Typed hook for reading the active variant, its capabilities, and the
 * derived TOTAL_KEYS count. Components MUST go through this hook —
 * direct `useContext(ConfigContext)` access for variant is a lint smell.
 */
export function useVariant() {
  const ctx = useContext(ConfigContext)
  if (!ctx) {
    throw new Error('useVariant must be used inside a ConfigProvider')
  }
  return {
    variant: ctx.variant,
    capabilities: ctx.capabilities,
    totalKeys: TOTAL_KEYS_BY_VARIANT[ctx.variant],
    isHandshakeConfirmed: ctx.isHandshakeConfirmed,
    setVariant: ctx.setVariant,
    setCapabilities: ctx.setCapabilities,
  }
}
