import { describe, it, expect, vi } from 'vitest'
import { render, screen } from '@testing-library/react'
import { useEffect } from 'react'
import type { ReactNode } from 'react'
import { ConnectionProvider } from '@/contexts/ConnectionContext'
import { ConfigProvider } from '@/contexts/ConfigContext'
import { SliderCard } from '@/components/SliderCard/SliderCard'
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
  prepareImport: vi.fn(),
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

function Force({
  variant,
  caps,
  fromHandshake,
}: {
  variant: 'T16' | 'T32'
  caps: { touchSlider: boolean; koalaMode: boolean }
  fromHandshake: boolean
}) {
  const { setVariant, setCapabilities } = useVariant()
  useEffect(() => {
    setVariant(variant)
    setCapabilities(caps, fromHandshake)
  }, [variant, caps, fromHandshake, setVariant, setCapabilities])
  return null
}

describe('SliderCard capability hiding (touchSlider)', () => {
  it('Renders slider UI when touchSlider=true (default T16)', () => {
    render(
      <Providers>
        <SliderCard
          label="Sensitivity"
          value={5}
          onValueChange={() => {}}
          min={0}
          max={10}
          capabilityKey="touchSlider"
        />
      </Providers>,
    )
    expect(screen.getByText('Sensitivity')).toBeInTheDocument()
  })

  it('Renders placeholder when touchSlider=false and handshake confirmed (T32)', async () => {
    render(
      <Providers>
        <Force variant="T32" caps={{ touchSlider: false, koalaMode: false }} fromHandshake={true} />
        <SliderCard
          label="Sensitivity"
          value={5}
          onValueChange={() => {}}
          min={0}
          max={10}
          capabilityKey="touchSlider"
        />
      </Providers>,
    )
    expect(await screen.findByText('Not available on T32')).toBeInTheDocument()
    expect(
      screen.getByText(
        "Touch slider is hardware-specific to T16 and isn't supported on this variant.",
      ),
    ).toBeInTheDocument()
  })

  it('Placeholder body says "will be confirmed when a device connects" when offline (no handshake)', async () => {
    render(
      <Providers>
        <Force
          variant="T32"
          caps={{ touchSlider: false, koalaMode: false }}
          fromHandshake={false}
        />
        <SliderCard
          label="Sensitivity"
          value={5}
          onValueChange={() => {}}
          min={0}
          max={10}
          capabilityKey="touchSlider"
        />
      </Providers>,
    )
    expect(
      await screen.findByText(
        'Touch slider availability will be confirmed when a device connects.',
      ),
    ).toBeInTheDocument()
  })

  it('Without capabilityKey: renders the slider unconditionally (no breaking existing usage)', () => {
    render(
      <Providers>
        <SliderCard label="Brightness" value={2} onValueChange={() => {}} min={0} max={5} />
      </Providers>,
    )
    expect(screen.getByText('Brightness')).toBeInTheDocument()
  })
})
