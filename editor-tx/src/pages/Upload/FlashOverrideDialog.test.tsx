import { describe, it, expect, vi } from 'vitest'
import { render, screen, fireEvent, waitFor } from '@testing-library/react'
import { FlashOverrideDialog } from './FlashOverrideDialog'

describe('FlashOverrideDialog', () => {
  const baseProps = {
    open: true,
    onConfirm: vi.fn(),
    onCancel: vi.fn(),
  }

  it('Title is exactly "Flash a different variant?"', () => {
    render(
      <FlashOverrideDialog
        {...baseProps}
        deviceVariant="T16"
        targetVariant="T32"
      />,
    )
    expect(screen.getByText('Flash a different variant?')).toBeInTheDocument()
  })

  it('Body interpolates both variants exactly', () => {
    render(
      <FlashOverrideDialog
        {...baseProps}
        deviceVariant="T16"
        targetVariant="T32"
      />,
    )
    expect(
      screen.getByText(
        'Your connected device reports T16. You selected T32. Flashing the wrong variant will require a recalibration and may render keys non-functional until the matching firmware is reflashed.',
      ),
    ).toBeInTheDocument()
  })

  it('Cancel CTA copy is "Keep T16" (when deviceVariant=T16)', () => {
    render(
      <FlashOverrideDialog
        {...baseProps}
        deviceVariant="T16"
        targetVariant="T32"
      />,
    )
    expect(screen.getByRole('button', { name: 'Keep T16' })).toBeInTheDocument()
  })

  it('Confirm CTA copy is "Flash T32 anyway" with destructive class', () => {
    render(
      <FlashOverrideDialog
        {...baseProps}
        deviceVariant="T16"
        targetVariant="T32"
      />,
    )
    const confirm = screen.getByRole('button', { name: 'Flash T32 anyway' })
    expect(confirm).toBeInTheDocument()
    expect(confirm.className).toMatch(/destructive/)
  })

  it('Cancel autoFocused after open', async () => {
    render(
      <FlashOverrideDialog
        {...baseProps}
        deviceVariant="T16"
        targetVariant="T32"
      />,
    )
    await waitFor(() => {
      expect(screen.getByRole('button', { name: 'Keep T16' })).toHaveFocus()
    })
  })

  it('ESC fires onCancel', () => {
    const onCancel = vi.fn()
    render(
      <FlashOverrideDialog
        {...baseProps}
        onCancel={onCancel}
        deviceVariant="T16"
        targetVariant="T32"
      />,
    )
    fireEvent.keyDown(document.body, { key: 'Escape', code: 'Escape' })
    expect(onCancel).toHaveBeenCalled()
  })

  it('Confirm fires onConfirm', () => {
    const onConfirm = vi.fn()
    render(
      <FlashOverrideDialog
        {...baseProps}
        onConfirm={onConfirm}
        deviceVariant="T16"
        targetVariant="T32"
      />,
    )
    fireEvent.click(screen.getByRole('button', { name: 'Flash T32 anyway' }))
    expect(onConfirm).toHaveBeenCalledTimes(1)
  })
})
