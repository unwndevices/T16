import { describe, it, expect, vi } from 'vitest'
import { render, screen } from '@testing-library/react'
import { useEffect } from 'react'
import type { ReactNode } from 'react'
import { ConnectionProvider } from '@/contexts/ConnectionContext'
import { ConfigProvider } from '@/contexts/ConfigContext'
import { NoteGrid } from '@/components/NoteGrid/NoteGrid'
import { useVariant } from '@/hooks/useVariant'
import type { T16Configuration } from '@/types/config'

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
  adaptConfigForVariant: vi.fn(
    (c: T16Configuration, v: 'T16' | 'T32'): T16Configuration => ({ ...c, variant: v }),
  ),
}))

function Providers({ children }: { children: ReactNode }) {
  return (
    <ConnectionProvider>
      <ConfigProvider>{children}</ConfigProvider>
    </ConnectionProvider>
  )
}

function ForceVariant({ variant }: { variant: 'T16' | 'T32' }) {
  const { setVariant, setCapabilities } = useVariant()
  useEffect(() => {
    setVariant(variant)
    setCapabilities({ touchSlider: variant === 'T16', koalaMode: variant === 'T16' }, true)
  }, [variant, setVariant, setCapabilities])
  return null
}

describe('NoteGrid (variant-aware)', () => {
  it('T16 default: renders a grid with 16 cells', () => {
    const { container } = render(
      <Providers>
        <NoteGrid />
      </Providers>,
    )
    const cells = container.querySelectorAll('[role="gridcell"]')
    expect(cells).toHaveLength(16)
  })

  it('T32 (handshake-confirmed): renders a grid with 32 cells', async () => {
    const { container, findByRole } = render(
      <Providers>
        <ForceVariant variant="T32" />
        <NoteGrid />
      </Providers>,
    )
    // Wait for the ForceVariant useEffect to flush both setVariant + setCapabilities
    // through React state and rerender NoteGrid against the T32 grid.
    await findByRole('grid', { name: 'Keyboard layout: T32, 32 keys' })
    const cells = container.querySelectorAll('[role="gridcell"]')
    expect(cells).toHaveLength(32)
  })

  it('T32 grid aria-label is exactly "Keyboard layout: T32, 32 keys"', async () => {
    const { findByRole } = render(
      <Providers>
        <ForceVariant variant="T32" />
        <NoteGrid />
      </Providers>,
    )
    expect(await findByRole('grid', { name: 'Keyboard layout: T32, 32 keys' })).toBeInTheDocument()
  })

  it('T16 grid renders MIDI note labels (e.g., C0 from default config)', () => {
    render(
      <Providers>
        <NoteGrid />
      </Providers>,
    )
    // DEFAULT_CONFIG bank 0: scale=0 (Chromatic), oct=0, note=0 → base note 0 = C-1.
    // First cell label = "C-1".
    expect(screen.getAllByText(/^C-1$/).length).toBeGreaterThanOrEqual(1)
  })
})
