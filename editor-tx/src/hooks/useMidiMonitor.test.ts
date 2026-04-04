import { renderHook, act } from '@testing-library/react'
import { describe, it, expect, beforeEach } from 'vitest'
import { useMidiMonitor, MAX_MESSAGES } from './useMidiMonitor'
import type { MidiMessage } from './useMidiMonitor'

// Mock Input that stores listeners and allows manual triggering
type ListenerMap = Record<string, ((...args: unknown[]) => void)[]>

function createMockInput() {
  const listeners: ListenerMap = {}

  return {
    addListener(type: string, handler: (...args: unknown[]) => void) {
      if (!listeners[type]) listeners[type] = []
      listeners[type].push(handler)
    },
    removeListener(type: string, handler: (...args: unknown[]) => void) {
      if (listeners[type]) {
        listeners[type] = listeners[type].filter((h) => h !== handler)
      }
    },
    emit(type: string, event: unknown) {
      for (const handler of listeners[type] ?? []) {
        handler(event)
      }
    },
    getListenerCount(type: string) {
      return listeners[type]?.length ?? 0
    },
  }
}

describe('useMidiMonitor', () => {
  let mockInput: ReturnType<typeof createMockInput>

  beforeEach(() => {
    mockInput = createMockInput()
  })

  it('starts with empty messages and not paused', () => {
    const { result } = renderHook(() => useMidiMonitor(null))
    expect(result.current.messages).toEqual([])
    expect(result.current.paused).toBe(false)
  })

  it('captures CC messages with correct fields', () => {
    const { result } = renderHook(() =>
      useMidiMonitor(mockInput as unknown as Parameters<typeof useMidiMonitor>[0]),
    )

    act(() => {
      mockInput.emit('controlchange', {
        message: { channel: 1 },
        controller: { number: 74 },
        rawValue: 64,
      })
    })

    expect(result.current.messages).toHaveLength(1)
    const msg: MidiMessage = result.current.messages[0]
    expect(msg.type).toBe('cc')
    expect(msg.channel).toBe(1)
    expect(msg.data1).toBe(74)
    expect(msg.data2).toBe(64)
    expect(msg.id).toBe(1)
    expect(typeof msg.timestamp).toBe('number')
  })

  it('captures noteon messages with correct fields', () => {
    const { result } = renderHook(() =>
      useMidiMonitor(mockInput as unknown as Parameters<typeof useMidiMonitor>[0]),
    )

    act(() => {
      mockInput.emit('noteon', {
        message: { channel: 2 },
        note: { number: 60, rawAttack: 100 },
      })
    })

    expect(result.current.messages).toHaveLength(1)
    const msg = result.current.messages[0]
    expect(msg.type).toBe('noteon')
    expect(msg.channel).toBe(2)
    expect(msg.data1).toBe(60)
    expect(msg.data2).toBe(100)
  })

  it('captures noteoff messages with velocity 0', () => {
    const { result } = renderHook(() =>
      useMidiMonitor(mockInput as unknown as Parameters<typeof useMidiMonitor>[0]),
    )

    act(() => {
      mockInput.emit('noteoff', {
        message: { channel: 3 },
        note: { number: 48, rawAttack: 0 },
      })
    })

    expect(result.current.messages).toHaveLength(1)
    const msg = result.current.messages[0]
    expect(msg.type).toBe('noteoff')
    expect(msg.channel).toBe(3)
    expect(msg.data1).toBe(48)
    expect(msg.data2).toBe(0)
  })

  it('caps messages at MAX_MESSAGES (100), oldest dropped', () => {
    const { result } = renderHook(() =>
      useMidiMonitor(mockInput as unknown as Parameters<typeof useMidiMonitor>[0]),
    )

    // Add 101 messages
    for (let i = 0; i < 101; i++) {
      act(() => {
        mockInput.emit('controlchange', {
          message: { channel: 1 },
          controller: { number: i % 128 },
          rawValue: i % 128,
        })
      })
    }

    expect(result.current.messages).toHaveLength(MAX_MESSAGES)
    // Newest message should be first (prepended)
    expect(result.current.messages[0].data2).toBe(100 % 128)
  })

  it('clear() empties the messages array', () => {
    const { result } = renderHook(() =>
      useMidiMonitor(mockInput as unknown as Parameters<typeof useMidiMonitor>[0]),
    )

    act(() => {
      mockInput.emit('controlchange', {
        message: { channel: 1 },
        controller: { number: 1 },
        rawValue: 50,
      })
    })
    expect(result.current.messages).toHaveLength(1)

    act(() => {
      result.current.clear()
    })
    expect(result.current.messages).toEqual([])
  })

  it('does not add messages when paused', () => {
    const { result } = renderHook(() =>
      useMidiMonitor(mockInput as unknown as Parameters<typeof useMidiMonitor>[0]),
    )

    // Pause
    act(() => {
      result.current.setPaused(true)
    })
    expect(result.current.paused).toBe(true)

    // Try to emit -- should not capture
    act(() => {
      mockInput.emit('controlchange', {
        message: { channel: 1 },
        controller: { number: 1 },
        rawValue: 50,
      })
    })
    expect(result.current.messages).toHaveLength(0)
  })

  it('resumes capturing after unpause', () => {
    const { result } = renderHook(() =>
      useMidiMonitor(mockInput as unknown as Parameters<typeof useMidiMonitor>[0]),
    )

    // Pause then unpause
    act(() => {
      result.current.setPaused(true)
    })
    act(() => {
      result.current.setPaused(false)
    })

    act(() => {
      mockInput.emit('noteon', {
        message: { channel: 1 },
        note: { number: 60, rawAttack: 127 },
      })
    })
    expect(result.current.messages).toHaveLength(1)
  })

  it('newest messages are prepended (first in array)', () => {
    const { result } = renderHook(() =>
      useMidiMonitor(mockInput as unknown as Parameters<typeof useMidiMonitor>[0]),
    )

    act(() => {
      mockInput.emit('controlchange', {
        message: { channel: 1 },
        controller: { number: 10 },
        rawValue: 10,
      })
    })
    act(() => {
      mockInput.emit('controlchange', {
        message: { channel: 1 },
        controller: { number: 20 },
        rawValue: 20,
      })
    })

    expect(result.current.messages[0].data1).toBe(20) // newest first
    expect(result.current.messages[1].data1).toBe(10) // oldest second
  })

  it('removes listeners on cleanup', () => {
    const { unmount } = renderHook(() =>
      useMidiMonitor(mockInput as unknown as Parameters<typeof useMidiMonitor>[0]),
    )

    expect(mockInput.getListenerCount('controlchange')).toBe(1)
    expect(mockInput.getListenerCount('noteon')).toBe(1)
    expect(mockInput.getListenerCount('noteoff')).toBe(1)

    unmount()

    expect(mockInput.getListenerCount('controlchange')).toBe(0)
    expect(mockInput.getListenerCount('noteon')).toBe(0)
    expect(mockInput.getListenerCount('noteoff')).toBe(0)
  })
})
