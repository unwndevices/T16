# Phase 5: Feature Polish - Research

**Researched:** 2026-04-04
**Domain:** PWA, BLE MIDI, CSS Grid visualization, real-time MIDI monitoring
**Confidence:** MEDIUM

## Summary

Phase 5 adds three user-facing features to the existing TypeScript/React/Vite web editor: a note grid visualizer showing scale-degree-colored 4x4 key mapping, PWA support with offline capability and BLE MIDI for mobile, and a real-time MIDI monitor page. The codebase is well-structured from Phase 3 with contexts, hooks, services, design system components, and CSS modules -- all three features integrate cleanly into established patterns.

The note grid visualizer is the most straightforward feature -- the firmware's `SetNoteMap` algorithm is already partially replicated in `Dashboard.tsx` (lines 108-116), but the current implementation is a naive linear mapping that ignores scale intervals. The scale interval data from `Scales.cpp` must be ported to TypeScript to produce correct note assignments. PWA via vite-plugin-pwa is mature and well-documented. BLE MIDI via Web Bluetooth API is the highest-risk item due to browser support limitations (Chrome/Edge only, no Firefox/Safari), the BLE MIDI packet format complexity, and the need for chunked SysEx handling over 20-byte BLE MTU (already flagged as a blocker in STATE.md).

**Primary recommendation:** Implement note grid visualizer first (smallest scope, highest visual impact), then MIDI monitor (new page, isolated), then PWA+BLE last (most integration surface, highest risk).

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- Note Grid: CSS Grid 4x4, rendered in Dashboard Keyboard tab, HSL hue rotation for scale degrees, muted gray for out-of-scale keys
- Note Grid: Reuse SCALES/NOTE_NAMES from Dashboard.tsx, extract to shared constants
- Note Grid: Reactive updates via ConfigContext (bank/scale changes)
- PWA: vite-plugin-pwa with workbox, precache all static assets
- PWA: Web App Manifest with T16 branding (theme_color matching design system primary)
- PWA: BLE MIDI via Web Bluetooth API (navigator.bluetooth), connect to "Topo T16" BLE peripheral
- PWA: BLE connection option in ConnectionContext alongside existing USB MIDI
- MIDI Monitor: New page at /monitor route, NavBar link
- MIDI Monitor: CC/Note On/Note Off messages, scrolling log (max 100, most recent at top)
- MIDI Monitor: CC values as horizontal bar (0-127), note velocity as bar width
- MIDI Monitor: Clear button and pause/resume toggle

### Claude's Discretion
- Exact HSL color mapping for scale degrees
- Web Bluetooth GATT service UUIDs for BLE MIDI
- MIDI monitor message formatting details
- PWA icon sizes and splash screen configuration
- Whether to use separate MidiMonitorContext or extend ConnectionContext for raw MIDI listening

### Deferred Ideas (OUT OF SCOPE)
None
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| WEBFEAT-01 | Note grid visualizer showing 4x4 key mapping with scale degree colors for current bank | Scale interval data from firmware Scales.cpp, SetNoteMap algorithm ported to TS, HSL color mapping for degree visualization |
| WEBFEAT-02 | PWA support -- service worker, manifest, offline capability, mobile BLE configuration | vite-plugin-pwa v1.2.0, workbox precache, Web Bluetooth API with BLE MIDI GATT UUIDs, BLE packet framing |
| WEBFEAT-05 | Live MIDI monitor with CC value visualization (real-time message display) | WebMidi.js event listeners for CC/NoteOn/NoteOff, ring buffer pattern for message log, CSS bar visualization |
</phase_requirements>

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| vite-plugin-pwa | 1.2.0 | PWA service worker + manifest generation | De facto standard for Vite PWA; zero-config with workbox integration |
| workbox-window | 7.4.0 (transitive) | Service worker registration in client | Bundled with vite-plugin-pwa, handles update prompts |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| (none needed) | - | Web Bluetooth API is a browser built-in | BLE MIDI uses navigator.bluetooth directly |
| (none needed) | - | WebMidi.js already installed (v3.1.16) | MIDI monitor uses existing input listeners |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| vite-plugin-pwa | Manual service worker | Far more code, no workbox precache manifest generation |
| Web Bluetooth API directly | web-bluetooth-polyfill | No real polyfill exists -- API is Chrome/Edge only regardless |
| Inline scale data | Shared JSON schema | Overkill for 19 static scale definitions, TS constants are simpler |

**Installation:**
```bash
npm install vite-plugin-pwa
```

**Version note:** vite-plugin-pwa v1.2.0 peer dependency lists `vite@^3.1.0 || ^4.0.0 || ^5.0.0 || ^6.0.0 || ^7.0.0` but does NOT yet list Vite 8. A GitHub issue (#918) is open for Vite 8 peer dep support. The plugin works with Vite 8 in practice -- install with `--legacy-peer-deps` or use `overrides` in package.json if npm complains.

## Architecture Patterns

### Recommended Project Structure
```
src/
  constants/
    scales.ts          # Scale intervals, NOTE_NAMES, SCALES, getNoteNameWithOctave
  components/
    NoteGrid/
      NoteGrid.tsx     # 4x4 CSS Grid with scale-degree coloring
      NoteGrid.module.css
  pages/
    Monitor/
      Monitor.tsx      # MIDI message log page
      Monitor.module.css
      index.ts
  hooks/
    useMidiMonitor.ts  # Raw MIDI message listener hook
  services/
    ble.ts             # Web Bluetooth BLE MIDI connection logic
```

### Pattern 1: Scale Interval Data (port from firmware)
**What:** TypeScript port of the firmware `scales[][]` array from `Scales.cpp`
**When to use:** Note grid visualizer needs to compute which MIDI note each key produces
**Example:**
```typescript
// src/constants/scales.ts
// Ported from firmware src/Scales.cpp
// Scale intervals: steps from root note, -1 = end of scale
export const SCALE_INTERVALS: readonly (readonly number[])[] = [
  [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11],  // CHROMATIC
  [0, 2, 4, 5, 7, 9, 11],                     // IONIAN
  [0, 2, 3, 5, 7, 9, 10],                     // DORIAN
  // ... all 19 scales, -1 sentinels removed (use array length instead)
]

export function computeNoteMap(
  scaleIndex: number,
  rootNote: number,
  octaveOffset: number,
  flipX: boolean,
  flipY: boolean,
  customScale1?: readonly number[],
  customScale2?: readonly number[],
): number[] {
  // Port of firmware SetNoteMap() from Scales.cpp
  const intervals = scaleIndex === 17 ? customScale1 ?? SCALE_INTERVALS[17]
                  : scaleIndex === 18 ? customScale2 ?? SCALE_INTERVALS[18]
                  : SCALE_INTERVALS[scaleIndex]
  const baseNote = octaveOffset * 12 + rootNote
  const noteMap = new Array<number>(16)
  let noteIndex = 0
  let octave = 0

  for (let i = 0; i < 16; i++) {
    if (noteIndex >= intervals.length) {
      noteIndex = 0
      octave++
    }
    let col = i % 4
    let row = Math.floor(i / 4)
    if (flipX) col = 3 - col
    if (flipY) row = 3 - row
    const index = row * 4 + col
    noteMap[index] = baseNote + intervals[noteIndex] + (octave * 12)
    noteIndex++
  }
  return noteMap
}
```

**Critical finding:** The existing `Dashboard.tsx` KeyboardTab (lines 108-116) computes `keyNotes` with a naive `baseNote + gridIndex` formula that ignores scale intervals entirely. This means the current key grid shows WRONG notes for non-chromatic scales. The note grid visualizer must use the proper `SetNoteMap` algorithm. This is a bug fix as well as a feature addition.

### Pattern 2: Scale Degree Coloring (HSL hue rotation)
**What:** Each scale degree gets a unique hue based on its position in the scale
**When to use:** NoteGrid component cell coloring
**Example:**
```typescript
// Degree 0 (root) = 0deg (red), distributed evenly across 360
function getScaleDegreeColor(degree: number, scaleLength: number): string {
  const hue = (degree / scaleLength) * 360
  return `hsl(${hue}, 70%, 55%)`
}

// For a note, determine its degree in the current scale:
function getNoteDegree(midiNote: number, rootNote: number, intervals: number[]): number | null {
  const semitoneFromRoot = ((midiNote - rootNote) % 12 + 12) % 12
  const degree = intervals.indexOf(semitoneFromRoot)
  return degree >= 0 ? degree : null  // null = not in scale (show muted gray)
}
```

### Pattern 3: BLE MIDI Connection in ConnectionContext
**What:** Extend ConnectionContext with a `connectBLE()` method alongside existing `connect()` (USB)
**When to use:** Mobile devices that support Web Bluetooth but not Web MIDI via USB
**Example:**
```typescript
// src/services/ble.ts
const BLE_MIDI_SERVICE = '03b80e5a-ede8-4b33-a751-6ce34ec4c700'
const BLE_MIDI_CHAR    = '7772e5db-3868-4112-a1a9-f2669d106bf3'

export async function connectBLE(): Promise<BluetoothRemoteGATTCharacteristic> {
  const device = await navigator.bluetooth.requestDevice({
    filters: [{ name: 'Topo T16' }],
    optionalServices: [BLE_MIDI_SERVICE],
  })
  const server = await device.gatt!.connect()
  const service = await server.getPrimaryService(BLE_MIDI_SERVICE)
  const char = await service.getCharacteristic(BLE_MIDI_CHAR)
  await char.startNotifications()
  return char
}
```

### Pattern 4: MIDI Monitor with Ring Buffer
**What:** Custom hook that listens to raw MIDI messages and maintains a bounded log
**When to use:** Monitor page real-time display
**Example:**
```typescript
// src/hooks/useMidiMonitor.ts
interface MidiMessage {
  id: number
  timestamp: number
  type: 'cc' | 'noteon' | 'noteoff'
  channel: number
  data1: number  // note or CC number
  data2: number  // velocity or CC value
}

const MAX_MESSAGES = 100

function useMidiMonitor(input: Input | null) {
  const [messages, setMessages] = useState<MidiMessage[]>([])
  const [paused, setPaused] = useState(false)
  const idRef = useRef(0)

  useEffect(() => {
    if (!input || paused) return
    const handleCC = (e: /* ControlChangeMessageEvent */) => { /* ... */ }
    const handleNoteOn = (e: /* NoteMessageEvent */) => { /* ... */ }
    const handleNoteOff = (e: /* NoteMessageEvent */) => { /* ... */ }
    input.addListener('controlchange', handleCC)
    input.addListener('noteon', handleNoteOn)
    input.addListener('noteoff', handleNoteOff)
    return () => {
      input.removeListener('controlchange', handleCC)
      input.removeListener('noteon', handleNoteOn)
      input.removeListener('noteoff', handleNoteOff)
    }
  }, [input, paused])

  const clear = () => setMessages([])
  return { messages, paused, setPaused, clear }
}
```

### Anti-Patterns to Avoid
- **Putting scale data in firmware-only format:** Use clean TypeScript arrays with no -1 sentinels. The firmware uses -1 as end-of-array marker, but TypeScript has `.length`.
- **Storing MIDI monitor state in context:** The monitor data is local to the Monitor page. Use a hook, not a context provider. Avoids re-renders across the entire app on every incoming MIDI message.
- **Blocking BLE connection on PWA:** BLE and PWA are separate features. PWA should work without BLE. BLE should work without PWA. Don't gate one on the other.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Service worker generation | Custom SW with cache API | vite-plugin-pwa + workbox | Precache manifest generation, cache versioning, update prompts |
| PWA manifest | Manual JSON file | vite-plugin-pwa manifest config | Integrated with build pipeline, auto-generates icons if configured |
| BLE MIDI packet framing | Custom binary parser | Dedicated `parseBLEMidiPacket()` utility | BLE MIDI has timestamp bytes + running status -- tricky to get right |

**Key insight:** The only genuinely custom code in this phase is the note grid visualization (scale-degree coloring + firmware algorithm port), the BLE MIDI packet parser, and the MIDI monitor UI. Everything else uses existing libraries or browser APIs.

## Common Pitfalls

### Pitfall 1: Wrong Note Map in Existing Code
**What goes wrong:** The current `Dashboard.tsx` KeyboardTab computes notes as `baseNote + gridIndex`, which is only correct for chromatic scale. Non-chromatic scales produce wrong notes.
**Why it happens:** The scale interval lookup was not ported from firmware `SetNoteMap()`.
**How to avoid:** Extract `computeNoteMap()` as a shared utility that mirrors firmware logic exactly, use it in both the existing KeyCard grid and the new NoteGrid visualizer.
**Warning signs:** Note names don't match what the physical device plays.

### Pitfall 2: BLE MIDI Packet Format
**What goes wrong:** BLE MIDI wraps standard MIDI bytes in a packet with timestamp header. Treating raw BLE characteristic values as plain MIDI bytes produces garbage.
**Why it happens:** BLE MIDI spec prepends a header byte (bit 7 set, bits 0-5 = timestamp high) and each MIDI message has a timestamp low byte before the status byte.
**How to avoid:** Implement a dedicated `parseBLEMidiPacket(data: DataView)` that strips BLE MIDI framing before passing to WebMidi.js or your own handler.
**Warning signs:** First byte of every message looks wrong, notes play at wrong pitches.

### Pitfall 3: BLE SysEx Fragmentation
**What goes wrong:** Full config dumps over BLE exceed the 20-byte default MTU. SysEx gets split across multiple BLE notifications.
**Why it happens:** BLE MIDI spec defines fragmentation rules. SysEx messages that span multiple packets use continuation bytes.
**How to avoid:** Implement a SysEx reassembly buffer that accumulates fragments until 0xF7 terminator is received. This was already flagged as a blocker in STATE.md.
**Warning signs:** Config dumps over BLE are truncated or corrupted.

### Pitfall 4: vite-plugin-pwa and Vite 8 Peer Dependency
**What goes wrong:** `npm install vite-plugin-pwa` fails with ERESOLVE due to peer dependency mismatch (plugin lists vite ^3-7, project uses vite 8).
**Why it happens:** Plugin hasn't updated peer deps for Vite 8 yet (issue #918 open).
**How to avoid:** Use `npm install vite-plugin-pwa --legacy-peer-deps` or add `overrides` in package.json.
**Warning signs:** npm install fails with peer dependency conflict.

### Pitfall 5: Web Bluetooth User Gesture Requirement
**What goes wrong:** `navigator.bluetooth.requestDevice()` throws if not triggered by a user gesture.
**Why it happens:** Browser security policy -- same as `navigator.requestMIDIAccess({ sysex: true })`.
**How to avoid:** BLE connect must be initiated from a button click handler. The existing `connect()` pattern in ConnectionContext already handles this correctly for USB MIDI.
**Warning signs:** "Must be handling a user gesture" error in console.

### Pitfall 6: Service Worker Caching During Development
**What goes wrong:** SW caches stale assets during development, changes don't appear.
**Why it happens:** vite-plugin-pwa registers SW in dev mode by default in some configs.
**How to avoid:** Set `devOptions: { enabled: false }` (or omit devOptions entirely -- disabled by default). Only test SW behavior in `vite preview` or production build.
**Warning signs:** Hot reload stops working, old versions persist.

## Code Examples

### vite-plugin-pwa Configuration
```typescript
// vite.config.ts
import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'
import { VitePWA } from 'vite-plugin-pwa'
import { resolve } from 'path'

export default defineConfig({
  plugins: [
    react(),
    VitePWA({
      registerType: 'autoUpdate',
      manifest: {
        name: 'T16 Configurator',
        short_name: 'T16',
        description: 'Configure your Topo T16 MIDI controller',
        theme_color: '#6246ea',  // --primary-800
        background_color: '#19191b', // --gray-900
        display: 'standalone',
        start_url: '/',
        icons: [
          { src: 'icon-192.png', sizes: '192x192', type: 'image/png' },
          { src: 'icon-512.png', sizes: '512x512', type: 'image/png' },
          { src: 'icon-512.png', sizes: '512x512', type: 'image/png', purpose: 'maskable' },
        ],
      },
      workbox: {
        globPatterns: ['**/*.{js,css,html,ico,png,svg,woff2}'],
        cleanupOutdatedCaches: true,
      },
    }),
  ],
  resolve: { alias: { '@': resolve(__dirname, './src') } },
  server: { watch: { usePolling: true } },
})
```

### tsconfig.json Type Addition
```json
{
  "compilerOptions": {
    "types": ["vite-plugin-pwa/client"]
  }
}
```

### NoteGrid Component Structure
```typescript
// src/components/NoteGrid/NoteGrid.tsx
import { useConfig } from '@/hooks/useConfig'
import { computeNoteMap, getScaleDegree, SCALE_INTERVALS } from '@/constants/scales'
import { getNoteNameWithOctave } from '@/constants/scales'
import styles from './NoteGrid.module.css'

export function NoteGrid() {
  const { config, selectedBank } = useConfig()
  const bank = config.banks[selectedBank]

  const noteMap = computeNoteMap(
    bank.scale, bank.note, bank.oct,
    bank.flip_x === 1, bank.flip_y === 1,
    config.global.custom_scale1, config.global.custom_scale2,
  )
  const intervals = SCALE_INTERVALS[bank.scale]

  return (
    <div className={styles.grid}>
      {noteMap.map((midiNote, i) => {
        const degree = getScaleDegree(midiNote, bank.note, intervals)
        const isInScale = degree !== null
        return (
          <div
            key={i}
            className={`${styles.cell} ${!isInScale ? styles.muted : ''}`}
            style={isInScale ? {
              '--degree-hue': `${(degree / intervals.length) * 360}deg`
            } as React.CSSProperties : undefined}
          >
            <span className={styles.noteName}>{getNoteNameWithOctave(midiNote)}</span>
            <span className={styles.noteNumber}>{midiNote}</span>
          </div>
        )
      })}
    </div>
  )
}
```

### BLE MIDI Packet Parsing
```typescript
// src/services/ble.ts
// BLE MIDI packet format:
// [header] [timestamp_low] [midi_status] [midi_data...]
// header: bit 7 = 1, bits 5-0 = timestamp high
// timestamp_low: bit 7 = 1, bits 6-0 = timestamp low
// For SysEx spanning packets: continuation uses specific framing rules

export function parseBLEMidiPacket(data: DataView): Uint8Array[] {
  const messages: Uint8Array[] = []
  let i = 1 // skip header byte
  while (i < data.byteLength) {
    if (data.getUint8(i) & 0x80) {
      i++ // skip timestamp low byte
    }
    const start = i
    // Collect bytes until next timestamp or end
    while (i < data.byteLength && !(data.getUint8(i) & 0x80)) {
      i++
    }
    if (i > start) {
      messages.push(new Uint8Array(data.buffer.slice(start, i)))
    }
  }
  return messages
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Manual SW + cache API | vite-plugin-pwa + workbox | 2023+ | Near-zero config PWA for Vite projects |
| BLE MIDI vendor-specific | Standardized BLE MIDI (Bluetooth SIG) | 2015+ | Universal BLE MIDI UUIDs work across all BLE MIDI devices |
| navigator.requestMIDIAccess for BLE | Web Bluetooth API for BLE, then manual MIDI parsing | Current | Chrome routes BLE MIDI through OS-level MIDI on some platforms, but Web Bluetooth gives direct control |

**Platform-specific BLE MIDI behavior:**
- **Android Chrome:** Web Bluetooth connects to BLE MIDI devices. On some Android versions, Chrome also exposes BLE MIDI through `navigator.requestMIDIAccess()` after OS-level pairing. Web Bluetooth is more reliable for explicit device selection.
- **Desktop Chrome:** BLE MIDI devices paired at OS level appear in Web MIDI API. Web Bluetooth allows connecting without OS pairing.
- **iOS Safari:** No Web Bluetooth support. BLE MIDI only works through OS-level Bluetooth pairing + Web MIDI API (which Safari also has limited support for). PWA on iOS is limited.

## Open Questions

1. **Custom scale note mapping for visualizer**
   - What we know: Custom scales (indices 17, 18) store absolute note values in `custom_scale1`/`custom_scale2` (16 values each). Standard scales store intervals from root.
   - What's unclear: Are custom scale values treated as intervals or absolute MIDI notes in firmware? The firmware `Scales.cpp` shows custom scales initialized to all zeros, but `SetNoteMap` treats them the same as other scales (as intervals).
   - Recommendation: Treat custom scale values the same way firmware does -- as intervals from root note. Verify by checking `SetNoteMap()` logic: it uses `root_note + scale_notes[note_index]` uniformly.

2. **BLE SysEx fragmentation implementation scope**
   - What we know: STATE.md flags "BLE chunked SysEx -- full config dumps exceed 20-byte BLE MTU" as a concern.
   - What's unclear: Whether the firmware already handles SysEx fragmentation over BLE, or if this needs firmware changes too.
   - Recommendation: Implement reassembly buffer on the web side. If firmware doesn't fragment properly, this becomes a firmware task and should be flagged. Per-parameter updates (small SysEx) should work without fragmentation.

3. **PWA icon assets**
   - What we know: `public/` has `icon.svg` and `vite.svg` only. No 192x192 or 512x512 PNG icons.
   - What's unclear: Whether there's a T16 logo available elsewhere.
   - Recommendation: Generate PNG icons from `icon.svg` (or create placeholder icons). Minimum required: 192x192 and 512x512 PNG.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| Node.js | Build + dev | Checking below | - | - |
| npm | Package install | Checking below | - | - |
| Chrome/Edge | Web Bluetooth + Web MIDI | Runtime (user's browser) | - | No fallback for BLE; USB MIDI works everywhere |
| vite-plugin-pwa | PWA feature | Not installed yet | 1.2.0 (npm) | npm install |

**Missing dependencies with no fallback:**
- None blocking -- all features are additive

**Missing dependencies with fallback:**
- vite-plugin-pwa: Install via `npm install vite-plugin-pwa --legacy-peer-deps`
- PWA icons: Generate from existing SVG or create placeholders

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Vitest 4.1.2 + @testing-library/react 16.3.2 |
| Config file | `editor-tx/vitest.config.ts` |
| Quick run command | `cd editor-tx && npx vitest run --reporter=verbose` |
| Full suite command | `cd editor-tx && npm run test` |

### Phase Requirements to Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| WEBFEAT-01a | computeNoteMap produces correct notes for all standard scales | unit | `cd editor-tx && npx vitest run src/constants/scales.test.ts -t "computeNoteMap"` | Wave 0 |
| WEBFEAT-01b | getScaleDegree identifies in-scale vs out-of-scale notes | unit | `cd editor-tx && npx vitest run src/constants/scales.test.ts -t "getScaleDegree"` | Wave 0 |
| WEBFEAT-01c | NoteGrid renders 16 cells with correct note names | unit | `cd editor-tx && npx vitest run src/components/NoteGrid/NoteGrid.test.tsx` | Wave 0 |
| WEBFEAT-02a | vite-plugin-pwa config produces valid manifest | smoke | `cd editor-tx && npx vite build && test -f dist/manifest.webmanifest` | Wave 0 |
| WEBFEAT-02b | BLE MIDI packet parsing extracts correct MIDI bytes | unit | `cd editor-tx && npx vitest run src/services/ble.test.ts -t "parseBLEMidiPacket"` | Wave 0 |
| WEBFEAT-05a | useMidiMonitor adds messages and respects max limit | unit | `cd editor-tx && npx vitest run src/hooks/useMidiMonitor.test.ts` | Wave 0 |
| WEBFEAT-05b | Monitor page renders message log with CC bars | unit | `cd editor-tx && npx vitest run src/pages/Monitor/Monitor.test.tsx` | Wave 0 |

### Sampling Rate
- **Per task commit:** `cd editor-tx && npx vitest run --reporter=verbose`
- **Per wave merge:** `cd editor-tx && npm run test && npm run build`
- **Phase gate:** Full suite green + successful production build before `/gsd:verify-work`

### Wave 0 Gaps
- [ ] `src/constants/scales.test.ts` -- covers WEBFEAT-01a, 01b (computeNoteMap, getScaleDegree)
- [ ] `src/services/ble.test.ts` -- covers WEBFEAT-02b (BLE MIDI packet parsing)
- [ ] `src/hooks/useMidiMonitor.test.ts` -- covers WEBFEAT-05a (message buffer logic)

## Sources

### Primary (HIGH confidence)
- Firmware `src/Scales.cpp` and `src/Scales.hpp` -- scale interval data and `SetNoteMap` algorithm
- Existing codebase: `editor-tx/src/pages/Dashboard/Dashboard.tsx` -- current note grid implementation and data constants
- Existing codebase: `editor-tx/src/contexts/ConnectionContext.tsx` -- USB MIDI connection pattern to extend for BLE
- npm registry: vite-plugin-pwa v1.2.0, workbox-window v7.4.0

### Secondary (MEDIUM confidence)
- [vite-plugin-pwa official docs](https://vite-pwa-org.netlify.app/) -- configuration patterns, workbox integration
- [BLE MIDI specification](https://devzone.nordicsemi.com/guides/short-range-guides/b/bluetooth-low-energy/posts/midi-over-bluetooth-le) -- GATT service UUID `03B80E5A-EDE8-4B33-A751-6CE34EC4C700`, characteristic UUID `7772E5DB-3868-4112-A1A9-F2669D106BF3`
- [Web Bluetooth API MDN](https://developer.mozilla.org/en-US/docs/Web/API/Web_Bluetooth_API) -- requestDevice, GATT connection
- [Chrome Web Bluetooth guide](https://developer.chrome.com/docs/capabilities/bluetooth) -- user gesture requirements, filters
- [vite-plugin-pwa Vite 8 issue #918](https://github.com/vite-pwa/vite-plugin-pwa/issues/918) -- peer dependency gap

### Tertiary (LOW confidence)
- BLE MIDI packet framing details -- based on specification reading, not verified against actual T16 BLE output

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - vite-plugin-pwa is well-established, WebMidi.js already in project
- Architecture: HIGH - extends established codebase patterns (contexts, hooks, CSS modules, design system)
- Note grid algorithm: HIGH - directly ported from firmware source code
- BLE MIDI integration: MEDIUM - Web Bluetooth API is well-documented but BLE MIDI packet framing and SysEx fragmentation are complex
- PWA + Vite 8: MEDIUM - works in practice but peer dep mismatch needs workaround
- Pitfalls: HIGH - identified from direct codebase analysis and specification review

**Research date:** 2026-04-04
**Valid until:** 2026-05-04 (30 days -- stable domain, main risk is vite-plugin-pwa Vite 8 support timeline)
