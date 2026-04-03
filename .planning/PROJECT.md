# T16 Refactor

## What This Is

A full refactor of the T16 MIDI controller firmware and web configurator to bring both codebases to the same quality bar as eisei (firmware architecture) and DROP (web architecture). The T16 is a 16-key FSR MIDI controller with capacitive touch slider, built on ESP32-S3, configured via a browser-based WebMIDI editor.

## Core Value

The configurator must feel instant and the firmware must be maintainable — every parameter change should reach the device in under 100ms, and any developer should be able to modify one feature without risking another.

## Requirements

### Validated

- ✓ 16-key FSR keyboard with velocity and aftertouch — existing
- ✓ Capacitive touch slider with multiple modes (pitch bend, CC, mod) — existing
- ✓ Multi-transport MIDI output (USB, BLE, TRS) — existing
- ✓ 4-bank configuration with per-bank key/CC settings — existing
- ✓ LED matrix patterns per mode (keyboard, XY pad, strum, strips) — existing
- ✓ Browser-based configuration via WebMIDI SysEx — existing
- ✓ Browser-based firmware flashing via esptool-js — existing
- ✓ Custom scale editing — existing
- ✓ Quick settings accessible from device — existing

### Active

- [ ] Firmware service extraction from main.cpp (eisei architecture pattern)
- [ ] Structured SysEx command protocol (replace magic byte matching)
- [ ] Per-parameter config sync (send only what changed, <100ms round-trip)
- [ ] Full config dump for backup/restore (optimized serialization)
- [ ] Web app rewrite in TypeScript with custom design system (match DROP)
- [ ] Working note grid visualizer with scale overlay (4x4 grid, colored by scale degree)
- [ ] Split MidiProvider god-context into connection, config, and sync concerns
- [ ] Web app organized by feature domain (contexts/, hooks/, services/, components/)
- [ ] Consistent, polished UI/UX across all configurator pages
- [ ] Fix memory leak in LedManager pattern transitions
- [ ] Fix TouchSlider::SetPosition no-op bug
- [ ] Fix unreachable XY_PAD branch in loop()
- [ ] Fix HardwareTest infinite loop on broken key
- [ ] Move header-only implementations to .cpp files
- [ ] Encapsulate global state into service classes
- [ ] PWA support for mobile configuration over BLE
- [ ] Automated tests (firmware unit tests, web component/integration tests)
- [ ] CI pipeline (build, lint, test)
- [ ] Proper firmware update flow (no bootloader button hold)

### Out of Scope

- CMake migration — staying on PlatformIO/Arduino
- WiFi OTA — using esptool-js browser flashing instead
- Web Serial for config — staying on SysEx (MIDI-native protocol)
- Mobile native app — PWA covers mobile use case
- Real-time audio processing — T16 is a MIDI controller, not a synth

## Context

- T16 is an unwn product — a 4x4 FSR MIDI controller with touch slider
- Reference repos for architecture patterns: eisei (firmware services, command handlers, HAL), DROP (TypeScript, design system, feature domains, PWA), unwn-core (C++ conventions, testing)
- Current firmware is a monolithic 887-line main.cpp with all logic, global state, and no tests
- Current web editor is JavaScript with Chakra UI v2, a 500-line god-context, flat component structure, and a broken scale visualizer
- Config sync is slow (~3sec) because it sends entire JSON config on every change with 11 filesystem read-write cycles per save on the firmware side
- Web configurator is deployed via GitHub Pages
- Hardware: ESP32-S3 custom board (unwn_s3), hardware revision REV_B active

## Constraints

- **Build system**: PlatformIO with Arduino framework — must stay (no CMake migration)
- **Protocol**: MIDI SysEx for device-editor communication — MIDI-native, not switching to Web Serial
- **Config compatibility**: Must handle migration from existing config format (version 103) to new format without data loss
- **Hosting**: GitHub Pages for web configurator
- **Hardware**: ESP32-S3 resource constraints (memory, flash, CPU)

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| TypeScript + custom design system + Radix (match DROP) | Consistency across unwn web projects, Chakra v2 is EOL | — Pending |
| Per-parameter SysEx + full dump for backup | Fast live edits (<100ms) + reliable backup/restore | — Pending |
| Service extraction following eisei patterns | Proven architecture in sibling project, maintainability | — Pending |
| Keep PlatformIO/Arduino | Existing ecosystem, custom board support, no migration cost | ✓ Good |
| GitHub Pages deployment | Already in use, free, CI-friendly | ✓ Good |

## Evolution

This document evolves at phase transitions and milestone boundaries.

**After each phase transition** (via `/gsd:transition`):
1. Requirements invalidated? → Move to Out of Scope with reason
2. Requirements validated? → Move to Validated with phase reference
3. New requirements emerged? → Add to Active
4. Decisions to log? → Add to Key Decisions
5. "What This Is" still accurate? → Update if drifted

**After each milestone** (via `/gsd:complete-milestone`):
1. Full review of all sections
2. Core Value check — still the right priority?
3. Audit Out of Scope — reasons still valid?
4. Update Context with current state

---
*Last updated: 2026-04-03 after initialization*
