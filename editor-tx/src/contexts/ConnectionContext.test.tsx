import { describe, it, expect, vi } from 'vitest'
import { render, screen } from '@testing-library/react'
import { renderHook } from '@testing-library/react'
import { ConnectionProvider } from '@/contexts/ConnectionContext'
import { useConnection } from '@/hooks/useConnection'

// Mock the midi service to avoid real WebMIDI calls
vi.mock('@/services/midi', () => ({
  enableMidi: vi.fn().mockResolvedValue(undefined),
  disableMidi: vi.fn().mockResolvedValue(undefined),
  findDevice: vi.fn().mockReturnValue({ input: null, output: null }),
}))

describe('useConnection', () => {
  it('throws when used outside ConnectionProvider', () => {
    // Suppress console.error for expected error
    const spy = vi.spyOn(console, 'error').mockImplementation(() => {})
    expect(() => renderHook(() => useConnection())).toThrow(
      'useConnection must be used within ConnectionProvider',
    )
    spy.mockRestore()
  })
})

describe('ConnectionProvider', () => {
  it('renders children', () => {
    render(
      <ConnectionProvider>
        <div>test child</div>
      </ConnectionProvider>,
    )
    expect(screen.getByText('test child')).toBeInTheDocument()
  })

  it('provides initial disconnected state', () => {
    const { result } = renderHook(() => useConnection(), {
      wrapper: ConnectionProvider,
    })
    expect(result.current.isConnected).toBe(false)
    expect(result.current.input).toBeNull()
    expect(result.current.output).toBeNull()
    expect(result.current.transport).toBeNull()
    expect(result.current.isDemo).toBe(false)
  })

  it('provides null transport initially', () => {
    const { result } = renderHook(() => useConnection(), {
      wrapper: ConnectionProvider,
    })
    expect(result.current.transport).toBeNull()
  })

  it('provides connect and disconnect functions', () => {
    const { result } = renderHook(() => useConnection(), {
      wrapper: ConnectionProvider,
    })
    expect(typeof result.current.connect).toBe('function')
    expect(typeof result.current.disconnect).toBe('function')
    expect(typeof result.current.setDemo).toBe('function')
  })
})
