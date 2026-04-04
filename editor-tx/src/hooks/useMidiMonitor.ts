import { useState, useEffect, useRef, useCallback } from 'react'
import type { Input, ControlChangeMessageEvent, NoteMessageEvent } from 'webmidi'

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

    const handleCC = (e: ControlChangeMessageEvent) => {
      addMessage({
        type: 'cc',
        channel: e.message.channel,
        data1: e.controller.number,
        data2: e.rawValue ?? Math.round((e.value as number) * 127),
      })
    }

    const handleNoteOn = (e: NoteMessageEvent) => {
      addMessage({
        type: 'noteon',
        channel: e.message.channel,
        data1: e.note.number,
        data2: e.note.rawAttack,
      })
    }

    const handleNoteOff = (e: NoteMessageEvent) => {
      addMessage({
        type: 'noteoff',
        channel: e.message.channel,
        data1: e.note.number,
        data2: 0,
      })
    }

    input.addListener('controlchange', handleCC)
    input.addListener('noteon', handleNoteOn)
    input.addListener('noteoff', handleNoteOff)

    return () => {
      input.removeListener('controlchange', handleCC)
      input.removeListener('noteon', handleNoteOn)
      input.removeListener('noteoff', handleNoteOff)
    }
  }, [input, paused])

  const clear = useCallback(() => setMessages([]), [])

  return { messages, paused, setPaused, clear }
}
