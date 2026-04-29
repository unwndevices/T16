import { describe, it, expect, vi } from 'vitest'
import { render, screen } from '@testing-library/react'
import { KeyboardGrid } from './KeyboardGrid'

describe('KeyboardGrid', () => {
  it('T16 4×4: renders 16 cells', () => {
    const { container } = render(
      <KeyboardGrid
        keys={16}
        cols={4}
        rows={4}
        renderKey={(i) => <span>{i}</span>}
        aria-label="Keyboard layout: T16, 16 keys"
      />,
    )
    const cells = container.querySelectorAll('[class*="cell"]')
    expect(cells).toHaveLength(16)
  })

  it('T32 4×8: renders 32 cells and sets data-keys="32"', () => {
    const { container } = render(
      <KeyboardGrid
        keys={32}
        cols={4}
        rows={8}
        renderKey={(i) => <span>{i}</span>}
        aria-label="Keyboard layout: T32, 32 keys"
      />,
    )
    const cells = container.querySelectorAll('[class*="cell"]')
    expect(cells).toHaveLength(32)
    const grid = container.querySelector('[role="grid"]')
    expect(grid?.getAttribute('data-keys')).toBe('32')
  })

  it('forwards aria-label to grid root', () => {
    render(
      <KeyboardGrid
        keys={32}
        cols={4}
        rows={8}
        renderKey={(i) => <span>{i}</span>}
        aria-label="Keyboard layout: T32, 32 keys"
      />,
    )
    expect(
      screen.getByRole('grid', { name: 'Keyboard layout: T32, 32 keys' }),
    ).toBeInTheDocument()
  })

  it('cols invariant: T32 still renders with repeat(4, 1fr)', () => {
    const { container } = render(
      <KeyboardGrid
        keys={32}
        cols={4}
        rows={8}
        renderKey={(i) => <span>{i}</span>}
        aria-label="Keyboard layout: T32, 32 keys"
      />,
    )
    const grid = container.querySelector('[role="grid"]') as HTMLElement
    expect(grid.style.gridTemplateColumns).toBe('repeat(4, 1fr)')
  })

  it('renderKey receives each integer in [0, keys) exactly once', () => {
    const renderKey = vi.fn((i: number) => <span data-i={i}>{i}</span>)
    render(
      <KeyboardGrid
        keys={16}
        cols={4}
        rows={4}
        renderKey={renderKey}
        aria-label="Keyboard layout: T16, 16 keys"
      />,
    )
    expect(renderKey).toHaveBeenCalledTimes(16)
    const calledIndices = renderKey.mock.calls.map((c) => c[0]).sort((a, b) => a - b)
    expect(calledIndices).toEqual([0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15])
  })

  it('dev-only console.error when cols * rows !== keys', () => {
    const errSpy = vi.spyOn(console, 'error').mockImplementation(() => {})
    render(
      <KeyboardGrid
        keys={16}
        cols={4}
        rows={8}
        renderKey={(i) => <span>{i}</span>}
        aria-label="Keyboard layout: bad"
      />,
    )
    const matched = errSpy.mock.calls.some(
      (call) =>
        typeof call[0] === 'string' &&
        call[0].includes('cols (4) * rows (8) !== keys (16)'),
    )
    expect(matched).toBe(true)
    errSpy.mockRestore()
  })
})
