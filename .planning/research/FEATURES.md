# Feature Landscape

**Domain:** FSR MIDI controller firmware + browser-based configurator
**Researched:** 2026-04-03

## Table Stakes

Features users expect from any MIDI controller with a configurator. Missing = product feels incomplete or broken.

### Firmware

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| Per-key velocity curve selection | Every FSR/pad controller offers this (Sensel, OpenDeck, EMP-16). Players need curves matched to their touch. | Low | Already exists (4 curves). Keep as-is. |
| Per-key aftertouch curve selection | Same as velocity -- aftertouch is meaningless without curve control | Low | Already exists (4 curves). Keep as-is. |
| Multi-transport MIDI (USB, BLE, TRS) | Standard for modern ESP32-based controllers. Users expect plug-and-play on any connection. | Low | Already exists. Factor into transport abstraction during refactor. |
| Bank/preset switching | Every controller from Morningstar to OpenDeck has banks. Users switch between songs/instruments. | Low | Already exists (4 banks). Make bank count configurable in refactor. |
| Key calibration routine | FSR sensors drift. Sensel, OpenDeck all have calibration. Without it, velocity feels random. | Low | Already exists. Fix the infinite-loop-on-broken-key bug. |
| Configuration persistence (survive reboot) | Obvious. OpenDeck, Morningstar, every controller saves config to flash. | Low | Already exists via LittleFS. Fix the 11-read-write-cycle bottleneck. |
| Scale/note mapping | Pad controllers without scale mapping are just drum pads. All note-grid controllers (Linnstrument, Launchpad Pro, Sensel) have this. | Low | Already exists with 17 built-in scales + 2 custom. Solid. |
| Quick settings from device (no computer needed) | Users need to change channel/octave/scale at a gig without a laptop. Morningstar, Grid, all have on-device editing. | Low | Already exists via button + LED UI. |
| Sync status indicator | User must know whether changes are saved to device. Morningstar editor shows sync state. Without it, users send config repeatedly "just in case." | Low | Already exists (`syncStatus` in MidiProvider). Clean up in refactor. |

### Web Configurator

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| WebMIDI SysEx connection | The configurator's reason for existing. OpenDeck, Morningstar, Sensel all have browser-based config. | Low | Already exists. Refactor into dedicated connection service. |
| Full config dump/restore (device <-> editor) | Every configurator (OpenDeck, Morningstar, Sensel) loads current config on connect and writes it back. | Low | Already exists but sends entire JSON every time (~3s). Must become per-parameter. |
| Config backup/restore to file | OpenDeck has backup/restore. Morningstar has it. Users expect to save their setup before firmware updates. | Low | Already exists (.topo file export/import). Add schema validation on import. |
| Per-bank keyboard settings | Channel, octave, scale, root note, velocity curve, aftertouch curve per bank. All multi-bank controllers have this. | Low | Already exists. Fix the CC page bug (hardcoded to bank 0). |
| CC mapping editor | 8 CC slots with channel + ID per bank. Standard for any controller with faders/encoders. | Low | Already exists. |
| Global settings (mode, brightness, sensitivity, transport enables) | Basic device configuration. Every configurator has a settings page. | Low | Already exists. |
| Custom scale editor | Linnstrument, Sensel, and any serious note-grid controller supports custom scales. | Low | Already exists (2 custom scale slots). |
| Browser-based firmware update | Morningstar has desktop + web firmware update. OpenDeck has web-based update. Users expect it. | Med | Already exists via esptool-js, but requires bootloader button hold. Must fix the UX. |
| Connection status feedback | Toast notifications, connected/disconnected state. Every WebMIDI tool does this. | Low | Already exists. |
| Demo/offline mode | Users want to explore the configurator before buying. Morningstar editor works without a device. | Low | Already exists (`isDemo` state). |

## Differentiators

Features that set T16 apart. Not universally expected, but add significant value.

### Firmware

| Feature | Value Proposition | Complexity | Notes |
|---------|-------------------|------------|-------|
| Per-parameter SysEx sync (<100ms) | Most controllers send full config dumps. Per-parameter sync makes the editor feel instant -- like turning a physical knob. This is the core value proposition stated in PROJECT.md. | High | Does not exist. Requires structured SysEx command protocol, parameter IDs, firmware-side per-field handlers. This is the single most impactful new feature. |
| Serial command interface for diagnostics | eisei has `SerialCommandManager` with modular command categories. Enables runtime debugging, factory testing, automated QA. No competing DIY controller has this level of diagnostics. | Med | Does not exist. Follow eisei pattern: `DiagnosticCommands`, `ParameterCommands`, etc. |
| Task scheduler for timing control | eisei uses `TaskScheduler` for consistent scan rates and priority processing. Eliminates timing jitter in key scanning and LED updates. | Med | Does not exist. Allows power optimization and consistent behavior. |
| Config version migration (non-destructive) | Current firmware silently overwrites config on version mismatch. Proper migration preserves user settings across firmware updates. Morningstar does this well. | Med | Does not exist. Implement incremental migration functions. |
| Structured SysEx manufacturer ID | OpenDeck and Morningstar use proper manufacturer ID prefixes. Prevents accidental config overwrites from other MIDI devices on the bus. | Low | Does not exist. Add unwn manufacturer ID prefix + command byte structure. |

### Web Configurator

| Feature | Value Proposition | Complexity | Notes |
|---------|-------------------|------------|-------|
| Note grid visualizer with scale overlay | A 4x4 visual representation showing which notes map to which keys, colored by scale degree. No competing pad configurator does this well. The broken `ScaleDisplay` already hints at intent. | Med | Partially exists (broken). Must show the actual note layout for current scale + octave + root note on the 4x4 grid. Huge UX win. |
| PWA support for mobile BLE configuration | Configure T16 from phone while it's connected via BLE. No cable needed. DROP has `PWAService.ts` as reference. Morningstar has a mobile app, but PWA avoids app store friction. | Med | Does not exist. Service worker + manifest + offline caching. BLE connection from mobile browser. |
| Live MIDI monitor with CC visualization | See MIDI messages in real-time as you play. Intech Grid editor has this. Existing `MidiMonitor.jsx` and CC message tracking provide foundation. | Low | Partially exists (`ccMessages` state tracks CC values). Build visual display. |
| TypeScript with shared config types | Config shape defined once, used in both firmware serialization and web editor. Eliminates the current class of bugs where field names mismatch between firmware JSON keys and web defaults. | Med | Does not exist. Single source of truth for config schema. |
| Firmware update without bootloader button | Current flow requires holding a physical button during power-on. Seamless update flow (click "update" in editor, device enters bootloader automatically via SysEx command) is a significant UX improvement. | Med | Does not exist. Requires firmware-side SysEx handler that triggers ESP32 bootloader mode. |
| Config sharing / community presets | Export configs as shareable files with metadata (name, description, use case). Morningstar has cloud preset sharing. Even a simple "copy link" with encoded config is valuable. | Low | Partially exists (.topo export). Add metadata (preset name, description, author) to export format. |

## Anti-Features

Features to explicitly NOT build. Each has been considered and rejected for stated reasons.

| Anti-Feature | Why Avoid | What to Do Instead |
|--------------|-----------|-------------------|
| WiFi OTA firmware updates | Requires WiFi credentials on device, adds attack surface, ESP32-S3 WiFi + BLE simultaneous use is flaky. esptool-js over USB is simpler and more reliable. | Keep browser-based esptool-js flashing. Improve UX with auto-bootloader SysEx command. |
| Web Serial for configuration | Would require a second protocol alongside SysEx. MIDI is the native transport -- config should travel over the same bus as music data. Web Serial adds browser permission complexity. | Stay on SysEx. Implement structured protocol with proper framing. |
| Mobile native app | App store approval, maintenance burden, two codebases. PWA covers the mobile use case with zero distribution friction. | PWA with service worker and BLE WebMIDI. |
| Lua/scripting engine for custom behaviors | Massive complexity for a 16-key controller. Grid editor has Lua, but Grid is a modular system. T16's modes (keyboard, strum, XY, faders) are fixed and sufficient. | Keep mode system. Add mode-specific parameters instead of general scripting. |
| Real-time audio processing / synth engine | T16 is a MIDI controller, not a synth. Adding audio would compete with resources on ESP32-S3 and blur the product identity. | Stay MIDI-only. Let DAWs and synths handle audio. |
| Dynamic bank count (user-configurable) | Adds config complexity, SysEx protocol complexity, and UI complexity for marginal gain. 4 banks is plenty for a 16-key controller. | Keep 4 banks fixed. Use the simplicity as a feature. |
| MIDI learn (controller -> editor) | Where you play a note and the editor auto-fills the mapping. Sounds useful but creates confusing UX for a pad controller where keys are positional, not arbitrary. MIDI learn makes sense for knob controllers, not note grids. | Keep explicit mapping in editor. Clear is better than clever. |
| Cloud sync / accounts | Requires server infrastructure, authentication, privacy policy. Overkill for a hardware configurator. | Local file export/import. Simple, private, works offline. |
| Display/screen on device | Hardware change, not firmware. T16's LED matrix + quick settings UI is sufficient for on-device control. | LED patterns for mode indication + quick settings via button combos. |

## Feature Dependencies

```
Per-parameter SysEx sync
  -> Structured SysEx protocol (manufacturer ID, command bytes, parameter IDs)
  -> Config schema with parameter registry (firmware)
  -> Per-field change detection (web)
  -> Split MidiProvider into connection + config + sync services (web)

Note grid visualizer
  -> Scale data shared between firmware and web (scale definitions in both codebases)
  -> Bank-aware config state (must know current bank's scale + octave + root)

PWA support
  -> Service worker + manifest
  -> Offline-capable UI (works without device connected -- demo mode already exists)
  -> BLE WebMIDI support in browser (Chrome Android supports this)

Firmware update UX improvement
  -> SysEx command to trigger ESP32 bootloader mode (firmware)
  -> Auto-detect bootloader state in esptool-js flow (web)

Serial command interface
  -> Service extraction from main.cpp (commands need isolated services to call)
  -> Firmware architecture refactor (prerequisite)

Config version migration
  -> Config schema versioning (firmware)
  -> Migration functions per version bump (firmware)
  -> Must happen before any config schema changes ship

TypeScript migration
  -> Prerequisite for shared config types
  -> Must happen before or alongside web app rewrite
```

## MVP Recommendation

Prioritize (in order):

1. **Structured SysEx protocol** -- Foundation for per-parameter sync. Without this, every other firmware-web improvement is blocked by the 3-second full-dump bottleneck.
2. **Per-parameter config sync** -- The stated core value: "every parameter change should reach the device in under 100ms." This is the product's primary UX promise.
3. **Note grid visualizer (working)** -- Highest-impact visual feature. Already partially built. Shows the user what their 4x4 grid actually plays.
4. **Config backup with schema validation** -- Already exists but needs validation. Low effort, prevents data corruption.
5. **Firmware update UX** -- Remove the bootloader button hold requirement. Major reduction in support burden.
6. **TypeScript migration** -- Enables compile-time safety for config sync protocol, which is the most bug-prone surface area.

Defer:
- **PWA support**: Valuable but not blocking. BLE-from-phone is a nice-to-have, not a must-have for initial rewrite.
- **Serial command interface**: Developer tooling. Important for maintainability but not user-facing.
- **Task scheduler**: Internal improvement. Existing tight-loop works (with the ADC delay fix). Schedule for after service extraction.
- **Config sharing with metadata**: Community feature. Build after the core edit-sync loop is solid.

## Sources

- [OpenDeck Configurable Features Wiki](https://github.com/shanteacontrols/OpenDeck/wiki/Configurable-features) -- Comprehensive feature list for a competing open-source MIDI controller platform
- [Morningstar Editor User Guide](https://manuals.morningstar.io/mc-midi-controller/Editor-User-Guide-(Version-1.3.6).1035304961.html) -- Preset management, cloud storage, backup/restore patterns
- [SenselApp Documentation](https://guide.sensel.com/app/) -- Overlay mapping, pressure visualization, firmware update flow
- [Intech Studio Grid Editor](https://intech.studio/products/editor) -- Lua scripting, MIDI monitor, profile cloud, page-based config storage
- [WebMIDI SysEx Editor Framework (MIDI.org)](https://midi.org/innovation-award/webmidi-sysex-editor-framework) -- SysEx editor patterns and browser-based configuration standards
- T16 existing codebase analysis (MidiProvider.jsx, Configuration.hpp, main.cpp, Dashboard.jsx, Settings.jsx, ControlChange.jsx)
- eisei firmware reference (PresetManager, CalibrationManager, SerialCommandManager, TaskScheduler, CommService, FirmwareUpdateService)
- DROP web reference (services/, contexts/, hooks/, design-system/, PWAService.ts)
