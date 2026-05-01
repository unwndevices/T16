import { describe, it, expect, vi, beforeEach } from 'vitest'
import { renderHook, act } from '@testing-library/react'
import type { ReactNode } from 'react'
import { ConnectionProvider } from '@/contexts/ConnectionContext'
import { ConfigProvider } from '@/contexts/ConfigContext'
import { useVariant } from '@/hooks/useVariant'
import { VARIANT_STORAGE_KEY } from '@/constants/capabilities'

vi.mock('@/services/midi', () => ({
  enableMidi: vi.fn().mockResolvedValue(undefined),
  disableMidi: vi.fn().mockResolvedValue(undefined),
  findDevice: vi.fn().mockReturnValue({ input: null, output: null }),
  parseSysExMessage: vi.fn(),
  parseConfigDump: vi.fn(),
  isConfigResponse: vi.fn().mockReturnValue(false),
  isCapabilitiesResponse: vi.fn().mockReturnValue(false),
  isVersionResponse: vi.fn().mockReturnValue(false),
  isParamAck: vi.fn().mockReturnValue(false),
  parseCapabilitiesPayload: vi.fn().mockReturnValue(null),
  requestCapabilities: vi.fn(),
  sendParamUpdate: vi.fn(),
  sendCCParamUpdate: vi.fn(),
  sendFullConfig: vi.fn(),
  requestConfigDump: vi.fn(),
  requestVersion: vi.fn(),
}))

vi.mock('@/services/configValidator', () => ({
  prepareImport: vi.fn().mockReturnValue({
    valid: false,
    config: null,
    errors: [],
    migrated: false,
  }),
}))

function AllProviders({ children }: { children: ReactNode }) {
  return (
    <ConnectionProvider>
      <ConfigProvider>{children}</ConfigProvider>
    </ConnectionProvider>
  )
}

describe('useVariant', () => {
  beforeEach(() => {
    window.localStorage.clear()
  })

  it('throws when used outside a ConfigProvider', () => {
    const spy = vi.spyOn(console, 'error').mockImplementation(() => {})
    expect(() => renderHook(() => useVariant())).toThrow('must be used inside a ConfigProvider')
    spy.mockRestore()
  })

  it('default precedence: returns T16 with totalKeys=16 and isHandshakeConfirmed=false', () => {
    const { result } = renderHook(() => useVariant(), { wrapper: AllProviders })
    expect(result.current.variant).toBe('T16')
    expect(result.current.totalKeys).toBe(16)
    expect(result.current.isHandshakeConfirmed).toBe(false)
  })

  it('config.variant T16 wins over localStorage T32 (config field is authoritative)', () => {
    window.localStorage.setItem(VARIANT_STORAGE_KEY, 'T32')
    const { result } = renderHook(() => useVariant(), { wrapper: AllProviders })
    // DEFAULT_CONFIG.variant = T16 — config wins absent handshake.
    expect(result.current.variant).toBe('T16')
    expect(result.current.totalKeys).toBe(16)
  })

  it('setVariant("T32") flips runtimeVariant; totalKeys becomes 32 only if handshake confirmed', () => {
    const { result } = renderHook(() => useVariant(), { wrapper: AllProviders })
    act(() => {
      result.current.setVariant('T32')
    })
    // Without a handshake, derivedVariant precedence places config.variant ahead of
    // runtimeVariant — so the visible variant stays T16. setVariant DOES persist to
    // localStorage and updates capabilities.
    expect(window.localStorage.getItem(VARIANT_STORAGE_KEY)).toBe('T32')
    expect(result.current.capabilities.touchSlider).toBe(false)
  })

  it('handshake (setCapabilities + setVariant with fromHandshake=true) flips variant to T32 and totalKeys to 32', () => {
    const { result } = renderHook(() => useVariant(), { wrapper: AllProviders })
    act(() => {
      result.current.setVariant('T32')
      result.current.setCapabilities({ touchSlider: false, koalaMode: false }, true)
    })
    expect(result.current.isHandshakeConfirmed).toBe(true)
    expect(result.current.variant).toBe('T32')
    expect(result.current.totalKeys).toBe(32)
    expect(result.current.capabilities.touchSlider).toBe(false)
  })

  it('totalKeys derivation: T16 → 16, T32 → 32', () => {
    const { result } = renderHook(() => useVariant(), { wrapper: AllProviders })
    expect(result.current.totalKeys).toBe(16) // default T16
    act(() => {
      result.current.setVariant('T32')
      result.current.setCapabilities({ touchSlider: false, koalaMode: false }, true)
    })
    expect(result.current.totalKeys).toBe(32)
  })
})
