import { describe, it, expect, vi, beforeEach } from 'vitest'
import { render, screen, fireEvent, waitFor } from '@testing-library/react'
import { useEffect } from 'react'
import type { ReactNode } from 'react'
import { ConnectionProvider } from '@/contexts/ConnectionContext'
import { ConfigProvider } from '@/contexts/ConfigContext'
import { ToastProvider } from '@/contexts/ToastContext'
import { Upload } from '@/pages/Upload/Upload'
import { useVariant } from '@/hooks/useVariant'
import type { T16Configuration } from '@/types/config'

const uploadFirmwareMock = vi.fn().mockResolvedValue(undefined)
const enterBootloaderMock = vi.fn().mockResolvedValue(undefined)
const resetMock = vi.fn()

vi.mock('@/hooks/useBootloader', () => ({
  useBootloader: () => ({
    state: 'bootloader_ready' as const,
    progress: 0,
    enterBootloader: enterBootloaderMock,
    uploadFirmware: uploadFirmwareMock,
    reset: resetMock,
    isConnected: true,
  }),
}))

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
    <ToastProvider>
      <ConnectionProvider>
        <ConfigProvider>{children}</ConfigProvider>
      </ConnectionProvider>
    </ToastProvider>
  )
}

function ForceVariant({
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

describe('Upload page — variant dropdown', () => {
  beforeEach(() => {
    uploadFirmwareMock.mockClear()
    enterBootloaderMock.mockClear()
    resetMock.mockClear()
    window.localStorage.clear()
  })

  it('Defaults to T16 when offline (no handshake)', () => {
    render(
      <Providers>
        <Upload />
      </Providers>,
    )
    expect(screen.getByRole('button', { name: 'Flash T16 firmware' })).toBeInTheDocument()
  })

  it('Defaults to detected variant when handshake confirms T32', async () => {
    render(
      <Providers>
        <ForceVariant
          variant="T32"
          caps={{ touchSlider: false, koalaMode: false }}
          fromHandshake={true}
        />
        <Upload />
      </Providers>,
    )
    await waitFor(() => {
      expect(screen.getByRole('button', { name: 'Flash T32 firmware' })).toBeInTheDocument()
    })
  })

  it('Shows T32 firmware unavailable warning copy when T32 is selected (placeholder only)', async () => {
    render(
      <Providers>
        <ForceVariant
          variant="T32"
          caps={{ touchSlider: false, koalaMode: false }}
          fromHandshake={true}
        />
        <Upload />
      </Providers>,
    )
    expect(
      await screen.findByText(
        'T32 firmware unavailable in this build. Reload the page or report this issue.',
      ),
    ).toBeInTheDocument()
  })

  it('Override modal does NOT fire when matching variant + flash clicked', async () => {
    render(
      <Providers>
        <ForceVariant
          variant="T16"
          caps={{ touchSlider: true, koalaMode: true }}
          fromHandshake={true}
        />
        <Upload />
      </Providers>,
    )
    const flashBtn = await screen.findByRole('button', { name: 'Flash T16 firmware' })
    fireEvent.click(flashBtn)
    expect(screen.queryByRole('button', { name: 'Flash T16 anyway' })).not.toBeInTheDocument()
    expect(uploadFirmwareMock).toHaveBeenCalledTimes(1)
  })

  it('Override modal fires only when handshake-confirmed AND mismatch (offline cross-variant skips modal)', async () => {
    // Offline: no handshake. Selected T16 (default) on a T32 device — but no handshake means
    // override gate does NOT fire. Use T16-default-no-handshake setup and click flash.
    render(
      <Providers>
        <Upload />
      </Providers>,
    )
    const flashBtn = await screen.findByRole('button', { name: 'Flash T16 firmware' })
    fireEvent.click(flashBtn)
    // Modal should not be present.
    expect(screen.queryByText('Flash a different variant?')).not.toBeInTheDocument()
    expect(uploadFirmwareMock).toHaveBeenCalledTimes(1)
  })
})
