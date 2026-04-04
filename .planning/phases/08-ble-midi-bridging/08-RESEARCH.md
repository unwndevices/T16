# Phase 8: BLE MIDI Bridging - Research

**Researched:** 2026-04-04
**Domain:** BLE MIDI protocol, Web Bluetooth API, firmware SysEx transport
**Confidence:** HIGH

## Summary

Phase 8 closes the final gap in WEBFEAT-02: BLE connects but MIDI data does not flow. The root cause is two-fold. On the web side, `connectBLE()` in ConnectionContext sets `isConnected=true` but leaves `input` and `output` null because Web Bluetooth provides a raw GATT characteristic, not a WebMidi `Input`/`Output` object. On the firmware side, the BLE-MIDI library (`BLEMIDI_Transport.h`) already handles SysEx fragmentation internally -- it chunks outgoing SysEx into 64-byte BLE packets and reassembles incoming multi-packet SysEx. The firmware side needs no SysEx chunking work.

The real work is on the web side: building a bridge that wraps the BLE GATT characteristic into objects that the existing `ConfigContext` can consume for sending and receiving SysEx. The web needs to (1) parse incoming BLE MIDI notifications via the existing `parseBLEMidiPacket`, (2) reassemble multi-packet SysEx from the parsed messages, (3) frame outgoing SysEx into BLE MIDI packets for characteristic writes, and (4) expose these through the same `input`/`output`-like interface or an equivalent abstraction that ConfigContext can use without caring about the transport.

**Primary recommendation:** Create a BLE MIDI bridge abstraction on the web side that converts between the BLE GATT characteristic and the SysEx send/receive interface that ConfigContext expects. The firmware already handles BLE SysEx correctly via the BLE-MIDI library.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
None -- discuss phase was skipped per user request. All implementation choices at Claude's discretion.

### Claude's Discretion
All implementation choices are at Claude's discretion -- discuss phase was skipped per user request. Use ROADMAP phase goal, success criteria, and codebase conventions to guide decisions.

### Deferred Ideas (OUT OF SCOPE)
None -- discuss phase skipped.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| WEBFEAT-02 | PWA support -- service worker, manifest, offline capability, mobile BLE configuration | PWA/offline already done in Phase 5. This phase completes the "mobile BLE configuration" part by making BLE MIDI data actually flow. |
</phase_requirements>

## Architecture Patterns

### Current State Analysis

**Firmware BLE SysEx (already works):**
- `MidiProvider::SendSysEx()` sends to both USB and BLE transports
- `BleMidiTransport::sendSysEx()` delegates to `MIDI_BLE.sendSysEx()`
- `BLEMIDI_Transport::write()` auto-chunks into 64-byte packets when buffer fills
- `BLEMIDI_Transport::endTransmission()` properly handles F7 end byte with timestamp
- `BLEMIDI_Transport::receive()` handles SysEx continuation (bit 7 of second byte clear = continuation)
- `SysExHandler::ProcessSysEx()` receives the reassembled complete SysEx -- same handler for USB and BLE
- NimBLE preferred MTU: 255 bytes (from `nimconfig.h`), BLE-MIDI buffer: 64 bytes
- SysEx max size: 2048 bytes (from `CustomSettings::SysExMaxSize`)

**Web BLE Connection (partially implemented):**
- `ble.ts`: `connectBLE()` connects to device, gets GATT characteristic, starts notifications
- `ble.ts`: `parseBLEMidiPacket()` extracts MIDI messages from BLE MIDI notification data
- `ConnectionContext.tsx`: `connectBLE()` calls the service, sets `isConnected=true`, leaves `input`/`output` null
- No outgoing BLE MIDI write implementation exists
- No SysEx reassembly from multi-packet BLE notifications exists
- No bridge between BLE characteristic and ConfigContext's input/output dependency

**ConfigContext dependencies on input/output:**
1. `useEffect` on `[output, isConnected]`: calls `requestConfigDump(output)` and `requestVersion(output)` -- needs a way to send SysEx
2. `useEffect` on `[input, isConnected]`: listens for `input.addListener('sysex', handler)` -- needs a way to receive SysEx events
3. `updateParam()` / `updateCCParam()`: calls `midiSendParamUpdate(output, ...)` -- needs output
4. `importConfig()`: calls `sendFullConfig(output, config)` -- needs output
5. All send functions go through `sysex.ts` which calls `output.sendSysex(manufacturerId, data)`

### Recommended Architecture

**Transport abstraction approach:**

Instead of trying to make BLE look like WebMidi `Input`/`Output` objects (which have complex internal APIs), create a transport-agnostic interface:

```
editor-tx/src/
  services/
    ble.ts                    # Existing: connectBLE, parseBLEMidiPacket
    bleBridge.ts              # NEW: BLE MIDI bridge (SysEx reassembly + framing)
  contexts/
    ConnectionContext.tsx      # Modified: transport abstraction
  protocol/
    sysex.ts                  # Modified: accept transport-agnostic send function
  types/
    midi.ts                   # Modified: transport types
```

### Pattern: Transport-Agnostic MIDI I/O

The key insight: ConfigContext doesn't need full WebMidi `Input`/`Output` objects. It needs two capabilities:
1. **Send SysEx:** `(manufacturerId: number, data: number[]) => void`
2. **Receive SysEx:** event-based notification with `Uint8Array` payload

**Approach:** Abstract the transport in ConnectionContext so it provides `sendSysex` and `onSysex` functions rather than raw `Input`/`Output`. Both USB (via WebMidi) and BLE (via GATT characteristic) implement these two functions.

### BLE MIDI Packet Framing (for outgoing writes)

Per the BLE MIDI spec and the library source code:

**Single-packet SysEx (fits in MTU):**
```
[header] [timestamp] [0xF0] [data...] [timestamp] [0xF7]
```

**Multi-packet SysEx (exceeds MTU):**
```
Packet 1: [header] [timestamp] [0xF0] [data...]        (no F7)
Packet N: [header] [data...]                             (no F0, no timestamp before data, no F7)
Packet Last: [header] [data...] [timestamp] [0xF7]       (timestamp before F7)
```

Key rules:
- Continuation packets: second byte has bit 7 clear (it's data, not a timestamp)
- Only the last packet contains F7 preceded by a timestamp byte
- Header byte always present (bit 7 set, bits 5-0 = timestamp high)
- Web Bluetooth `writeValueWithoutResponse()` for each packet

### BLE SysEx Reassembly (for incoming notifications)

The existing `parseBLEMidiPacket()` extracts individual MIDI messages per notification. For SysEx:
- A notification containing `0xF0` starts a SysEx buffer
- Continuation notifications (data without F0/F7) append to the buffer
- A notification containing `0xF7` completes the message
- The bridge must accumulate across notifications until F7 is seen

### Anti-Patterns to Avoid
- **Faking WebMidi objects:** Don't try to create mock `Input`/`Output` instances -- they have complex internal state. Abstract at a higher level.
- **Blocking writes:** BLE characteristic writes are async. Don't assume synchronous send.
- **Ignoring MTU:** Don't send packets larger than negotiated MTU - 3. Default safe: 20 bytes (23 - 3).
- **Polling for responses:** Use characteristic notifications (already started in `connectBLE()`).

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| BLE MIDI packet parsing | Custom parser | Existing `parseBLEMidiPacket()` in `ble.ts` | Already tested with 7 unit tests |
| Firmware SysEx chunking | Custom chunking | BLE-MIDI library built-in chunking | `BLEMIDI_Transport::write()` already auto-chunks at 64 bytes |
| BLE GATT connection | Custom Bluetooth code | Existing `connectBLE()` in `ble.ts` | Already handles device filter, service discovery, notifications |
| SysEx protocol encoding | New encoder | Existing `sysex.ts` functions | Already mirrors firmware protocol |

**Key insight:** The firmware side is complete. The BLE-MIDI library handles all fragmentation. The work is exclusively on the web side -- building the bridge between GATT characteristic I/O and the app's SysEx-level interface.

## Common Pitfalls

### Pitfall 1: BLE Write Packet Size Exceeds MTU
**What goes wrong:** Writing more than MTU-3 bytes to the characteristic causes the write to fail silently or the device to disconnect.
**Why it happens:** Default BLE ATT MTU is 23 bytes (20 payload). Web Bluetooth may negotiate higher but you can't query the result.
**How to avoid:** Frame outgoing BLE MIDI packets with a conservative max payload of 20 bytes. The Web Bluetooth API handles MTU negotiation transparently, but Chrome doesn't expose the negotiated MTU. Use 20-byte chunks as the safe default. NimBLE on ESP32 will attempt to negotiate up to 255, but the bottleneck is the phone/browser side.
**Warning signs:** Writes succeed for small messages (param updates) but fail for config dumps.

### Pitfall 2: SysEx Reassembly Across Notifications
**What goes wrong:** Partial SysEx messages treated as complete, causing JSON parse failures on config dump.
**Why it happens:** A full config dump (~1KB JSON) arrives as many BLE notifications. Each notification must be accumulated until F7 is seen.
**How to avoid:** Maintain a SysEx accumulation buffer in the bridge. Only emit the complete message when F7 is received.
**Warning signs:** Config dump requests over BLE return parse errors or incomplete JSON.

### Pitfall 3: Async Write Ordering
**What goes wrong:** SysEx packets arrive at firmware out of order.
**Why it happens:** `writeValueWithoutResponse()` is fire-and-forget. Calling multiple writes without awaiting can reorder.
**How to avoid:** Use `writeValueWithResponse()` or serialize writes with await. For SysEx fragments, each chunk must complete before sending the next.
**Warning signs:** Firmware logs show garbled SysEx data.

### Pitfall 4: BLE Disconnect During Config Dump
**What goes wrong:** Partial SysEx buffer left in memory after disconnect, then corrupts next session.
**Why it happens:** BLE connections are less stable than USB. Device can go out of range.
**How to avoid:** Clear the SysEx accumulation buffer on GATT disconnect event. The existing `gattserverdisconnected` listener in ConnectionContext already fires `disconnect()`.
**Warning signs:** Reconnecting after failed config dump shows stale/corrupt config.

### Pitfall 5: Characteristic Write Without Response vs With Response
**What goes wrong:** Using `writeValue()` (with response) blocks and is slower, or using `writeValueWithoutResponse()` loses data.
**Why it happens:** BLE MIDI spec says to use WriteWithoutResponse. But for multi-packet SysEx, you need to ensure ordering.
**How to avoid:** Use `writeValueWithoutResponse()` but `await` each call. Web Bluetooth internally queues writes per characteristic, so awaiting ensures FIFO ordering.
**Warning signs:** Config load over BLE hangs or produces errors on firmware side.

## Code Examples

### BLE MIDI Outgoing Packet Framing

```typescript
// Source: BLE MIDI spec + BLEMIDI_Transport.h analysis
const BLE_PACKET_MAX = 20 // Conservative: default ATT MTU (23) - ATT overhead (3)

function getMidiTimestamp(): [number, number] {
  const ts = Date.now() & 0x1fff
  const header = ((ts >> 7) & 0x3f) | 0x80
  const low = (ts & 0x7f) | 0x80
  return [header, low]
}

/**
 * Frame a SysEx message (without F0/F7) into BLE MIDI packets.
 * Input: raw SysEx data bytes (no F0/F7 wrapping, matching webmidi.js convention)
 * Output: array of Uint8Array packets ready for characteristic.writeValueWithoutResponse()
 */
function frameSysExForBLE(manufacturerId: number, data: number[]): Uint8Array[] {
  const fullMessage = [0xf0, manufacturerId, ...data, 0xf7]
  const packets: Uint8Array[] = []

  let offset = 0
  while (offset < fullMessage.length) {
    const [header, tsLow] = getMidiTimestamp()
    const isFirst = offset === 0
    const remaining = fullMessage.length - offset

    if (isFirst) {
      // First packet: [header] [timestamp] [F0] [data...]
      const capacity = BLE_PACKET_MAX - 2 // header + timestamp
      const chunkSize = Math.min(remaining, capacity)
      const packet = new Uint8Array(2 + chunkSize)
      packet[0] = header
      packet[1] = tsLow
      for (let i = 0; i < chunkSize; i++) packet[2 + i] = fullMessage[offset + i]

      // If this contains F7 (complete message fits), insert timestamp before F7
      if (offset + chunkSize >= fullMessage.length) {
        // Entire message fits -- need timestamp before F7
        const pkt = new Uint8Array(3 + chunkSize - 1 + 1) // header + ts + data... + ts + F7
        // Simplified: for short messages, just use the single-packet format
        // [header] [ts] [F0 mfr data...] [ts] [F7]
        // This gets complex; see full implementation
      }

      packets.push(packet)
      offset += chunkSize
    } else {
      // Continuation: [header] [data...] (no timestamp before data, bit 7 clear on byte 1)
      const isLast = offset + (BLE_PACKET_MAX - 1) >= fullMessage.length
      const capacity = BLE_PACKET_MAX - 1 // header only
      const chunkSize = Math.min(remaining, capacity)
      const packet = new Uint8Array(1 + chunkSize)
      packet[0] = header
      for (let i = 0; i < chunkSize; i++) packet[1 + i] = fullMessage[offset + i]

      // If last packet contains F7, insert timestamp before it
      // (F7 must be preceded by timestamp in BLE MIDI)

      packets.push(packet)
      offset += chunkSize
    }
  }

  return packets
}
```

Note: The above is a structural sketch. The actual implementation needs careful handling of the timestamp-before-F7 rule. See the `BLEMIDI_Transport::endTransmission()` in the library for the canonical pattern.

### SysEx Reassembly from BLE Notifications

```typescript
// Source: BLE MIDI spec analysis + parseBLEMidiPacket pattern
class BLESysExReassembler {
  private buffer: number[] = []
  private collecting = false

  /**
   * Feed parsed MIDI messages from a BLE notification.
   * Returns complete SysEx messages (if any finished in this batch).
   */
  feed(messages: Uint8Array[]): Uint8Array[] {
    const complete: Uint8Array[] = []

    for (const msg of messages) {
      if (msg.length === 0) continue

      if (msg[0] === 0xf0) {
        // Start of SysEx
        this.buffer = Array.from(msg)
        this.collecting = true

        // Check if complete (contains F7)
        if (msg[msg.length - 1] === 0xf7) {
          complete.push(new Uint8Array(this.buffer))
          this.reset()
        }
      } else if (this.collecting) {
        // Continuation data
        this.buffer.push(...msg)

        if (msg[msg.length - 1] === 0xf7) {
          complete.push(new Uint8Array(this.buffer))
          this.reset()
        }
      }
      // Non-SysEx messages during collection: ignore (or handle as real-time)
    }

    return complete
  }

  reset(): void {
    this.buffer = []
    this.collecting = false
  }
}
```

### Transport Abstraction Types

```typescript
// Source: analysis of ConfigContext dependencies
interface MidiTransport {
  sendSysex(manufacturerId: number, data: number[]): void
  addSysexListener(handler: (data: Uint8Array) => void): void
  removeSysexListener(handler: (data: Uint8Array) => void): void
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| WebMidi Input/Output only | Transport abstraction over WebMidi + Web Bluetooth | This phase | Enables BLE config without changing ConfigContext logic |
| Manual SysEx chunking on firmware | BLE-MIDI library auto-chunks | Already in place (BLE-MIDI 2.2.0) | No firmware changes needed |

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Vitest 4.1.2 |
| Config file | `editor-tx/vitest.config.ts` |
| Quick run command | `cd editor-tx && npx vitest run` |
| Full suite command | `cd editor-tx && npx vitest run` |

### Phase Requirements -> Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| WEBFEAT-02a | BLE SysEx framing produces valid BLE MIDI packets | unit | `cd editor-tx && npx vitest run src/services/bleBridge.test.ts -t "frameSysEx"` | Wave 0 |
| WEBFEAT-02b | BLE SysEx reassembly accumulates across notifications | unit | `cd editor-tx && npx vitest run src/services/bleBridge.test.ts -t "reassemble"` | Wave 0 |
| WEBFEAT-02c | BLE transport sends/receives via characteristic | unit | `cd editor-tx && npx vitest run src/services/bleBridge.test.ts -t "transport"` | Wave 0 |
| WEBFEAT-02d | ConnectionContext exposes transport for both USB and BLE | unit | `cd editor-tx && npx vitest run src/contexts/ConnectionContext.test.tsx` | Exists (extend) |

### Sampling Rate
- **Per task commit:** `cd editor-tx && npx vitest run`
- **Per wave merge:** `cd editor-tx && npx vitest run`
- **Phase gate:** Full suite green before `/gsd:verify-work`

### Wave 0 Gaps
- [ ] `editor-tx/src/services/bleBridge.test.ts` -- covers WEBFEAT-02a, 02b, 02c (BLE MIDI framing, reassembly, transport)
- Existing `ble.test.ts` covers packet parsing (parseBLEMidiPacket) -- no changes needed
- Existing `ConnectionContext.test.tsx` exists -- extend for BLE transport path

## Open Questions

1. **Negotiated MTU visibility in Web Bluetooth**
   - What we know: Chrome negotiates MTU with the BLE peripheral but does not expose the negotiated value via Web Bluetooth API
   - What's unclear: Whether Chrome's Web Bluetooth internally handles write fragmentation at the GATT level (L2CAP layer), separate from BLE MIDI application-level chunking
   - Recommendation: Use 20-byte safe default for BLE MIDI packet payload. Even if Chrome handles L2CAP fragmentation, the BLE MIDI spec requires application-level SysEx framing across BLE packets.

2. **Config dump size over BLE**
   - What we know: Config JSON is approximately 1-1.5KB. At 20 bytes per BLE MIDI packet (minus headers), this is 50-100+ BLE write/notification round trips.
   - What's unclear: Whether this is fast enough for acceptable UX
   - Recommendation: Implement and measure. If too slow, explore requesting higher MTU via NimBLE (already defaults to 255) and using larger BLE MIDI packets if the connection allows.

## Sources

### Primary (HIGH confidence)
- BLE-MIDI library source: `.pio/libdeps/esp32s3/BLE-MIDI/src/BLEMIDI_Transport.h` -- SysEx chunking implementation
- BLE-MIDI NimBLE backend: `.pio/libdeps/esp32s3/BLE-MIDI/src/hardware/BLEMIDI_ESP32_NimBLE.h` -- GATT characteristic setup
- NimBLE config: `.pio/libdeps/esp32s3/NimBLE-Arduino/src/nimconfig.h` -- MTU defaults (255)
- Firmware MidiProvider: `src/Libs/MidiProvider.cpp` -- BLE transport wiring
- Web BLE service: `editor-tx/src/services/ble.ts` -- existing connection and parsing
- Web ConnectionContext: `editor-tx/src/contexts/ConnectionContext.tsx` -- current BLE gap
- Web ConfigContext: `editor-tx/src/contexts/ConfigContext.tsx` -- input/output dependencies

### Secondary (MEDIUM confidence)
- [BLE MIDI Spec (PDF)](https://www.hangar42.nl/wp-content/uploads/2017/10/BLE-MIDI-spec.pdf) -- SysEx multi-packet framing rules
- [SparkFun BLE MIDI Tutorial](https://learn.sparkfun.com/tutorials/midi-ble-tutorial/encapsulating-midi-data) -- Packet structure reference
- [Nordic DevZone BLE MIDI Guide](https://devzone.nordicsemi.com/guides/short-range-guides/b/bluetooth-low-energy/posts/midi-over-bluetooth-le) -- Protocol overview

### Tertiary (LOW confidence)
- MTU negotiation behavior in Chrome Web Bluetooth -- no official docs confirming internal L2CAP handling

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH -- all libraries already in the project, no new dependencies needed
- Architecture: HIGH -- clear analysis of existing code, well-understood gap
- Pitfalls: HIGH -- directly derived from BLE MIDI spec and library source code analysis

**Research date:** 2026-04-04
**Valid until:** 2026-05-04 (stable domain, spec-based)
