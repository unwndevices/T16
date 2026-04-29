import { describe, it, expect, vi } from 'vitest'

vi.mock('webmidi', () => ({
  WebMidi: {
    enable: vi.fn().mockResolvedValue(undefined),
    disable: vi.fn().mockResolvedValue(undefined),
    getInputByName: vi.fn(),
    getOutputByName: vi.fn(),
    octaveOffset: 0,
  },
}))

import { parseCapabilitiesPayload } from '@/services/midi'

function makePayload(json: string): Uint8Array {
  const header = [0x07, 0x02, 0x00] // CMD_CAPABILITIES, SUB.RESPONSE, status OK
  const body = Array.from(json).map((c) => c.charCodeAt(0))
  return new Uint8Array([...header, ...body])
}

describe('parseCapabilitiesPayload', () => {
  it('Happy path T16: valid JSON with touchSlider+koalaMode true', () => {
    const data = makePayload(
      '{"variant":"T16","capabilities":{"touchSlider":true,"koalaMode":true}}',
    )
    const result = parseCapabilitiesPayload(data)
    expect(result).toEqual({
      variant: 'T16',
      capabilities: { touchSlider: true, koalaMode: true },
    })
  })

  it('Happy path T32: valid JSON with both capabilities false', () => {
    const data = makePayload(
      '{"variant":"T32","capabilities":{"touchSlider":false,"koalaMode":false}}',
    )
    const result = parseCapabilitiesPayload(data)
    expect(result).toEqual({
      variant: 'T32',
      capabilities: { touchSlider: false, koalaMode: false },
    })
  })

  it('Malformed JSON returns null', () => {
    const data = makePayload('{not-json')
    expect(parseCapabilitiesPayload(data)).toBeNull()
  })

  it('Unknown variant string T64 returns null', () => {
    const data = makePayload(
      '{"variant":"T64","capabilities":{"touchSlider":true,"koalaMode":true}}',
    )
    expect(parseCapabilitiesPayload(data)).toBeNull()
  })

  it('Missing capabilities object returns null', () => {
    const data = makePayload('{"variant":"T16"}')
    expect(parseCapabilitiesPayload(data)).toBeNull()
  })

  it('Wrong type on touchSlider returns null', () => {
    const data = makePayload(
      '{"variant":"T16","capabilities":{"touchSlider":"yes","koalaMode":true}}',
    )
    expect(parseCapabilitiesPayload(data)).toBeNull()
  })

  it('Truncated buffer (length < 4) returns null', () => {
    const data = new Uint8Array([0x07, 0x02])
    expect(parseCapabilitiesPayload(data)).toBeNull()
  })
})
