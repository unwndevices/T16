import { describe, it, expect, vi } from 'vitest'
import { render, screen } from '@testing-library/react'
import { useEffect } from 'react'
import type { ReactNode } from 'react'
import { ConnectionProvider } from '@/contexts/ConnectionContext'
import { ConfigProvider } from '@/contexts/ConfigContext'
import { VariantIndicator } from '@/components/VariantIndicator'
import { useVariant } from '@/hooks/useVariant'
import { TooltipProvider } from '@/design-system/Tooltip'
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
  prepareImport: vi.fn(),
  adaptConfigForVariant: vi.fn(
    (c: T16Configuration, v: 'T16' | 'T32'): T16Configuration => ({ ...c, variant: v }),
  ),
}))

function Providers({ children }: { children: ReactNode }) {
  return (
    <TooltipProvider>
      <ConnectionProvider>
        <ConfigProvider>{children}</ConfigProvider>
      </ConnectionProvider>
    </TooltipProvider>
  )
}

function ForceConnected({
  variant,
  caps,
}: {
  variant: 'T16' | 'T32'
  caps: { touchSlider: boolean; koalaMode: boolean }
}) {
  const { setVariant, setCapabilities } = useVariant()
  useEffect(() => {
    setVariant(variant)
    setCapabilities(caps, true)
  }, [variant, caps, setVariant, setCapabilities])
  return null
}

describe('VariantIndicator', () => {
  it('Offline (default): label is "T16 (offline)"', () => {
    render(
      <Providers>
        <VariantIndicator />
      </Providers>,
    )
    expect(screen.getByText('T16 (offline)')).toBeInTheDocument()
  })

  it('Connected T16: chip text is exactly "T16"', async () => {
    const { findByRole } = render(
      <Providers>
        <ForceConnected variant="T16" caps={{ touchSlider: true, koalaMode: true }} />
        <VariantIndicator />
      </Providers>,
    )
    const button = await findByRole('button', {
      name: 'Variant: T16. Detected from device.',
    })
    expect(button.textContent).toBe('T16')
    expect(button).toHaveAttribute('data-variant', 'T16')
  })

  it('Connected T32: chip text is exactly "T32" with data-variant attribute', async () => {
    const { findByRole } = render(
      <Providers>
        <ForceConnected variant="T32" caps={{ touchSlider: false, koalaMode: false }} />
        <VariantIndicator />
      </Providers>,
    )
    const button = await findByRole('button', {
      name: 'Variant: T32. Detected from device.',
    })
    expect(button.textContent).toBe('T32')
    expect(button).toHaveAttribute('data-variant', 'T32')
  })

  it('Connected: chip is disabled (handshake wins)', async () => {
    const { findByRole } = render(
      <Providers>
        <ForceConnected variant="T16" caps={{ touchSlider: true, koalaMode: true }} />
        <VariantIndicator />
      </Providers>,
    )
    const button = await findByRole('button', {
      name: 'Variant: T16. Detected from device.',
    })
    expect(button).toBeDisabled()
  })

  it('Offline: helper copy "Connect a device to override this automatically." is present', () => {
    render(
      <Providers>
        <VariantIndicator />
      </Providers>,
    )
    expect(screen.getByText('Connect a device to override this automatically.')).toBeInTheDocument()
  })
})
