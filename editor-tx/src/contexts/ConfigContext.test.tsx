import { describe, it, expect, vi } from 'vitest'
import { render, screen, act } from '@testing-library/react'
import { renderHook } from '@testing-library/react'
import type { ReactNode } from 'react'
import { ConnectionProvider } from '@/contexts/ConnectionContext'
import { ConfigProvider, DEFAULT_CONFIG } from '@/contexts/ConfigContext'
import { useConfig } from '@/hooks/useConfig'
import { DOMAIN, FIELD_GLOBAL } from '@/protocol/sysex'

// Mock all MIDI service functions
vi.mock('@/services/midi', () => ({
  enableMidi: vi.fn().mockResolvedValue(undefined),
  disableMidi: vi.fn().mockResolvedValue(undefined),
  findDevice: vi.fn().mockReturnValue({ input: null, output: null }),
  parseSysExMessage: vi.fn(),
  parseConfigDump: vi.fn(),
  isConfigResponse: vi.fn(),
  sendParamUpdate: vi.fn(),
  sendFullConfig: vi.fn(),
  requestConfigDump: vi.fn(),
  requestVersion: vi.fn(),
}))

// Mock configValidator
vi.mock('@/services/configValidator', () => ({
  prepareImport: vi.fn().mockReturnValue({
    valid: false,
    config: null,
    errors: [],
    migrated: false,
  }),
}))

// Wrapper that provides both ConnectionProvider and ConfigProvider
function AllProviders({ children }: { children: ReactNode }) {
  return (
    <ConnectionProvider>
      <ConfigProvider>{children}</ConfigProvider>
    </ConnectionProvider>
  )
}

describe('useConfig', () => {
  it('throws when used outside ConfigProvider', () => {
    const spy = vi.spyOn(console, 'error').mockImplementation(() => {})
    expect(() => renderHook(() => useConfig())).toThrow(
      'useConfig must be used within ConfigProvider'
    )
    spy.mockRestore()
  })
})

describe('ConfigProvider', () => {
  it('renders children', () => {
    render(
      <AllProviders>
        <div>config child</div>
      </AllProviders>
    )
    expect(screen.getByText('config child')).toBeInTheDocument()
  })

  it('provides default config', () => {
    const { result } = renderHook(() => useConfig(), {
      wrapper: AllProviders,
    })
    expect(result.current.config.version).toBe(DEFAULT_CONFIG.version)
    expect(result.current.config.global.mode).toBe(0)
    expect(result.current.selectedBank).toBe(0)
    expect(result.current.isSynced).toBe(false)
  })

  it('updates selectedBank via setBank', () => {
    const { result } = renderHook(() => useConfig(), {
      wrapper: AllProviders,
    })

    act(() => {
      result.current.setBank(2)
    })

    expect(result.current.selectedBank).toBe(2)
  })

  it('updates config field via updateParam', () => {
    const { result } = renderHook(() => useConfig(), {
      wrapper: AllProviders,
    })

    act(() => {
      result.current.updateParam(DOMAIN.GLOBAL, 0, FIELD_GLOBAL.BRIGHTNESS, 5)
    })

    expect(result.current.config.global.brightness).toBe(5)
  })

  it('updates config via setConfig', () => {
    const { result } = renderHook(() => useConfig(), {
      wrapper: AllProviders,
    })

    const newConfig = {
      ...DEFAULT_CONFIG,
      global: { ...DEFAULT_CONFIG.global, mode: 2 },
    }

    act(() => {
      result.current.setConfig(newConfig)
    })

    expect(result.current.config.global.mode).toBe(2)
  })
})
