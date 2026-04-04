// BLE MIDI service — Web Bluetooth connection and BLE MIDI packet parsing
// BLE MIDI spec: https://www.midi.org/specifications/midi-transports-specifications/specification-for-midi-over-bluetooth-low-energy

/** Standard BLE MIDI GATT service UUID (Bluetooth SIG) */
export const BLE_MIDI_SERVICE = '03b80e5a-ede8-4b33-a751-6ce34ec4c700'

/** Standard BLE MIDI GATT characteristic UUID (Bluetooth SIG) */
export const BLE_MIDI_CHAR = '7772e5db-3868-4112-a1a9-f2669d106bf3'

/**
 * Parse a BLE MIDI packet into individual MIDI messages.
 *
 * BLE MIDI packet format:
 *   [header_byte] [timestamp_low] [midi_status data...] [timestamp_low] [midi_status data...] ...
 *
 * - Header byte: bit 7 set, bits 5-0 = timestamp high
 * - Timestamp low bytes: bit 7 set, bits 6-0 = timestamp low
 * - MIDI status bytes: bit 7 set (0x80-0xEF for channel messages)
 * - MIDI data bytes: bit 7 clear (0x00-0x7F)
 *
 * Each MIDI message in the packet is preceded by a timestamp low byte.
 * After the timestamp, the next byte with bit 7 set is the MIDI status.
 * Subsequent bytes with bit 7 clear are data bytes for that message.
 */
export function parseBLEMidiPacket(data: DataView): Uint8Array[] {
  const messages: Uint8Array[] = []
  const len = data.byteLength

  if (len <= 1) return messages

  let i = 1 // skip header byte (byte 0)

  while (i < len) {
    const byte = data.getUint8(i)

    // Timestamp low byte: bit 7 set
    if (byte & 0x80) {
      i++ // skip timestamp

      // Check if next byte is a MIDI status byte
      if (i < len && data.getUint8(i) & 0x80) {
        const msgBytes: number[] = [data.getUint8(i)]
        i++

        // Collect data bytes (bit 7 clear) until next timestamp/status or end
        while (i < len && !(data.getUint8(i) & 0x80)) {
          msgBytes.push(data.getUint8(i))
          i++
        }

        messages.push(new Uint8Array(msgBytes))
      }
    } else {
      // Data byte without preceding timestamp — skip (shouldn't happen in valid packets)
      i++
    }
  }

  return messages
}

/**
 * Connect to a BLE MIDI device via Web Bluetooth.
 * Must be called from a user gesture (button click).
 *
 * Returns the GATT characteristic (for notifications) and the BluetoothDevice.
 */
export async function connectBLE(): Promise<{
  characteristic: BluetoothRemoteGATTCharacteristic
  device: BluetoothDevice
}> {
  const device = await navigator.bluetooth.requestDevice({
    filters: [{ name: 'Topo T16' }],
    optionalServices: [BLE_MIDI_SERVICE],
  })

  const server = await device.gatt!.connect()
  const service = await server.getPrimaryService(BLE_MIDI_SERVICE)
  const characteristic = await service.getCharacteristic(BLE_MIDI_CHAR)
  await characteristic.startNotifications()

  return { characteristic, device }
}
