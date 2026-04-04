import { useState, useEffect, useRef, useCallback } from 'react'
import type { Input } from 'webmidi'

export interface MidiMessage {
  id: number
  timestamp: number
  type: 'cc' | 'noteon' | 'noteoff'
  channel: number
  data1: number
  data2: number
}

export const MAX_MESSAGES = 100

export function useMidiMonitor(input: Input | null) {
  const [messages, setMessages] = useState<MidiMessage[]>([])
  const [paused, setPaused] = useState(false)
  const idRef = useRef(0)

  useEffect(() => {
    if (!input || paused) return

    const addMessage = (msg: Omit<MidiMessage, 'id' | 'timestamp'>) => {
      idRef.current += 1
      const message: MidiMessage = {
        ...msg,
        id: idRef.current,
        timestamp: Date.now(),
      }
      setMessages((prev) => [message, ...prev].slice(0, MAX_MESSAGES))
    }

    const handleCC = (e: { message: { channel: number }; controller: { number: number }; rawValue: number }) => {
      addMessage({
        type: 'cc',
        channel: e.message.channel,
        data1: e.controller.number,
        data2: e.rawValue,
      })
    }

    const handleNoteOn = (e: { message: { channel: number }; note: { number: number }; rawAttack: number }) => {
      addMessage({
        type: 'noteon',
        channel: e.message.channel,
        data1: e.note.number,
        data2: e.rawAttack,
      })
    }

    const handleNoteOff = (e: { message: { channel: number }; note: { number: number } }) => {
      addMessage({
        type: 'noteoff',
        channel: e.message.channel,
        data1: e.note.number,
        data2: 0,
      })
    }

    input.addListener('controlchange', handleCC as Parameters<Input['addListener']>[1])
    input.addListener('noteon', handleNoteOn as Parameters<Input['addListener']>[1])
    input.addListener('noteoff', handleNoteOff as Parameters<Input['addListener']>[1])

    return () => {
      input.removeListener('controlchange', handleCC as Parameters<Input['removeListener']>[1])
      input.removeListener('noteon', handleNoteOn as Parameters<Input['removeListener']>[1])
      input.removeListener('noteoff', handleNoteOff as Parameters<Input['removeListener']>[1])
    }
  }, [input, paused])

  const clear = useCallback(() => setMessages([]), [])

  return { messages, paused, setPaused, clear }
}
