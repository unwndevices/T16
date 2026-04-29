import { describe, it, expect, vi } from 'vitest'
import { render, screen, fireEvent, waitFor } from '@testing-library/react'
import { CrossVariantAdaptDialog } from '@/components/CrossVariantAdaptDialog'
import { DEFAULT_CONFIG } from '@/contexts/ConfigContext'

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

describe('CrossVariantAdaptDialog', () => {
  const baseProps = {
    open: true,
    fileConfig: DEFAULT_CONFIG,
    onAdapt: vi.fn(),
    onCancel: vi.fn(),
  }

  it('Title is exactly "Variant mismatch"', () => {
    render(
      <CrossVariantAdaptDialog
        {...baseProps}
        fileVariant="T16"
        deviceVariant="T32"
      />,
    )
    expect(screen.getByText('Variant mismatch')).toBeInTheDocument()
  })

  it('Body line 1 interpolates variants ("This config is for T16. You\'re connected to T32.")', () => {
    render(
      <CrossVariantAdaptDialog
        {...baseProps}
        fileVariant="T16"
        deviceVariant="T32"
      />,
    )
    expect(
      screen.getByText("This config is for T16. You're connected to T32."),
    ).toBeInTheDocument()
  })

  it('T16 → T32: body line 2 is exact pad copy', () => {
    render(
      <CrossVariantAdaptDialog
        {...baseProps}
        fileVariant="T16"
        deviceVariant="T32"
      />,
    )
    expect(
      screen.getByText(
        'Adapting will pad per-key settings (notes, scales, CC) from 16 to 32 keys with default values for keys 17–32. Global settings are preserved.',
      ),
    ).toBeInTheDocument()
  })

  it('T32 → T16: body line 2 is exact truncate copy in destructive style', () => {
    const { container } = render(
      <CrossVariantAdaptDialog
        {...baseProps}
        fileVariant="T32"
        deviceVariant="T16"
      />,
    )
    const p = screen.getByText(
      'Adapting will discard per-key settings for keys 17–32. This cannot be undone for the loaded file.',
    )
    expect(p).toBeInTheDocument()
    expect(p.className).toMatch(/destructive/)
    void container
  })

  it('Cancel autoFocused after dialog opens', async () => {
    render(
      <CrossVariantAdaptDialog
        {...baseProps}
        fileVariant="T16"
        deviceVariant="T32"
      />,
    )
    await waitFor(() => {
      expect(screen.getByRole('button', { name: 'Cancel' })).toHaveFocus()
    })
  })

  it('Adapt CTA fires onAdapt', () => {
    const onAdapt = vi.fn()
    render(
      <CrossVariantAdaptDialog
        {...baseProps}
        onAdapt={onAdapt}
        fileVariant="T16"
        deviceVariant="T32"
      />,
    )
    fireEvent.click(screen.getByRole('button', { name: 'Adapt and load' }))
    expect(onAdapt).toHaveBeenCalledTimes(1)
  })

  it('Cancel CTA fires onCancel', () => {
    const onCancel = vi.fn()
    render(
      <CrossVariantAdaptDialog
        {...baseProps}
        onCancel={onCancel}
        fileVariant="T16"
        deviceVariant="T32"
      />,
    )
    fireEvent.click(screen.getByRole('button', { name: 'Cancel' }))
    expect(onCancel).toHaveBeenCalledTimes(1)
  })

  it('ESC fires onCancel via Radix Dialog onOpenChange(false)', () => {
    const onCancel = vi.fn()
    render(
      <CrossVariantAdaptDialog
        {...baseProps}
        onCancel={onCancel}
        fileVariant="T16"
        deviceVariant="T32"
      />,
    )
    fireEvent.keyDown(document.body, { key: 'Escape', code: 'Escape' })
    expect(onCancel).toHaveBeenCalled()
  })
})
