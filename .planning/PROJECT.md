# T16 Refactor

## What This Is

A fully refactored T16 MIDI controller firmware and web configurator. The T16 is a 16-key FSR MIDI controller with capacitive touch slider, built on ESP32-S3. The firmware uses service-based architecture (AppEngine pattern from eisei), and the web configurator is a TypeScript PWA with a custom design system (Radix + CSS custom properties, matching DROP).

## Core Value

The configurator must feel instant and the firmware must be maintainable — every parameter change should reach the device in under 100ms, and any developer should be able to modify one feature without risking another.

## Current Milestone: v1.1 Variant Support

**Goal:** Add compile-time T16/T32 variant support on top of the v1.0 firmware and editor-tx architecture, leveraging the validated hardware bring-up from `origin/3dot0`.

**Target features:**
- Per-variant pinout headers + `platformio.ini` build envs (`t16_debug/release`, `t32_debug/release`)
- `HardwareVariantConfig` constexpr carrier consumed by `Adc`, `Keyboard`, `LedManager`, `DataManager` in place of macros (`BANK_AMT`, `NUM_LEDS`)
- T32 hardware bring-up: dual-mux ADC scan with shared select pins, validated key permutation, calibration on physical T32
- Config schema bump with `variant` discriminator field, per-variant array sizing, calibration variant tagging, single migration rule
- Editor-tx variant awareness: `ConfigContext` variant state, ajv schema update, conditional UI (4×4 vs 4×8 keyboard, calibration view), flasher variant selection in `Upload.tsx`

**Out of scope for v1.1:** T64 variant (no hardware), `ParameterRegistry`, `FeatureFlagManager` bitmask, `UniversalConfiguration`, reversible migrations, new binary SysEx protocol, triple-redundancy/CRC32, web rewrite (editor-tx is at quality bar).

**Research digest:** `.planning/research/v1.1-variant-research.md`

## Current State

**Shipped:** v1.0 (2026-04-04) — 9 phases, 32 plans, all verified

**Firmware:** Service-based architecture with AppEngine orchestrator (14-line main.cpp), structured SysEx protocol, ConfigManager with lazy persistence, 72+ native unit tests.

**Web Editor:** React 19 + TypeScript strict + Vite 8, custom design system (10 Radix components), ConnectionContext/ConfigContext split, note grid visualizer, MIDI monitor, PWA installable, BLE MIDI transport with SysEx framing/reassembly, config import/export with validation, 89 Vitest tests.

**CI:** GitHub Actions with firmware build/test/format and web typecheck/lint/test/build.

## Requirements

### Validated

- ✓ 16-key FSR keyboard with velocity and aftertouch — existing
- ✓ Capacitive touch slider with multiple modes — existing
- ✓ Multi-transport MIDI output (USB, BLE, TRS) — existing
- ✓ 4-bank configuration with per-bank key/CC settings — existing
- ✓ LED matrix patterns per mode — existing
- ✓ Browser-based configuration via WebMIDI SysEx — existing
- ✓ Custom scale editing — existing
- ✓ Quick settings accessible from device — existing
- ✓ Firmware service extraction from main.cpp — v1.0
- ✓ Structured SysEx command protocol — v1.0
- ✓ Per-parameter config sync (<100ms round-trip) — v1.0
- ✓ Full config dump for backup/restore — v1.0
- ✓ Web app rewrite in TypeScript with custom design system — v1.0
- ✓ Note grid visualizer with scale degree colors — v1.0
- ✓ Context split (ConnectionContext + ConfigContext) — v1.0
- ✓ Feature-domain organization — v1.0
- ✓ Consistent UI/UX across configurator — v1.0
- ✓ LedManager memory leak fix (unique_ptr) — v1.0
- ✓ TouchSlider::SetPosition bug fix — v1.0
- ✓ XY_PAD dead branch removed — v1.0
- ✓ HardwareTest timeout fix — v1.0
- ✓ Header-only implementations moved to .cpp — v1.0
- ✓ Global state encapsulated in services — v1.0
- ✓ PWA installable and offline-capable — v1.0
- ✓ Firmware + web unit tests — v1.0
- ✓ CI pipeline (GitHub Actions) — v1.0
- ✓ Firmware update without bootloader button — v1.0
- ✓ Config import validation against schema — v1.0
- ✓ MIDI monitor with CC visualization — v1.0
- ✓ Browser-based firmware flashing via esptool-js — existing

### Known Gaps (from v1.0 audit)

- ✓ CC per-parameter sync fixed (dedicated 5-byte payload for DOMAIN_BANK_CC) — v1.0 Phase 6
- ✓ Schema 'pal' field added (export-reimport roundtrip works) — v1.0 Phase 6
- ✓ BLE MIDI bridging complete (SysEx framing, reassembly, transport abstraction) — v1.0 Phase 8
- ✓ Calibration/factory reset SysEx commands implemented — v1.0 Phase 7
- ✓ LedManager UpdateTransition bug fixed — v1.0 Phase 7
- ✓ getNoteNameWithOctave duplication removed, SYNC_CONFIRMED dead code removed — v1.0 Phase 7

### Out of Scope

- CMake migration — staying on PlatformIO/Arduino
- WiFi OTA — using esptool-js browser flashing instead
- Web Serial for config — staying on SysEx (MIDI-native protocol)
- Mobile native app — PWA covers mobile use case
- Real-time audio processing — T16 is a MIDI controller, not a synth

## Context

- T16 is an unwn product — a 4x4 FSR MIDI controller with touch slider
- Reference repos: eisei (firmware), DROP (web), unwn-core (C++ conventions)
- Firmware: ~2000 LOC C++17 across service classes, tested with Unity
- Web editor: ~3000 LOC TypeScript, React 19, Vite 8, tested with Vitest
- Deployed via GitHub Pages (web) + esptool-js browser flashing (firmware)
- Hardware: ESP32-S3 custom board (unwn_s3), revision REV_B

## Constraints

- **Build system**: PlatformIO with Arduino framework
- **Protocol**: MIDI SysEx for device-editor communication
- **Config compatibility**: v103 → v200 migration supported
- **Hosting**: GitHub Pages for web configurator
- **Hardware**: ESP32-S3 resource constraints

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| TypeScript + Radix + CSS custom properties | Match DROP, Chakra v2 EOL | ✓ Good |
| Per-parameter SysEx + full dump for backup | Fast edits + reliable backup | ✓ Good |
| Service extraction following eisei patterns | Proven architecture | ✓ Good |
| Keep PlatformIO/Arduino | Ecosystem, no migration cost | ✓ Good |
| GitHub Pages deployment | Free, CI-friendly | ✓ Good |
| React 19 + Vite 8 + ESLint 9 flat config | Match DROP toolchain | ✓ Good |
| ConfigManager lazy persistence (2s idle flush) | Reduces flash wear, batches edits | ✓ Good |
| vite-plugin-pwa with workbox | Standard PWA approach | ✓ Good |
| Web Bluetooth for BLE MIDI | Standard API, no plugins | ✓ Good |

## Evolution

This document evolves at phase transitions and milestone boundaries.

**After each phase transition** (via `/gsd-transition`):
1. Requirements invalidated? → Move to Out of Scope with reason
2. Requirements validated? → Move to Validated with phase reference
3. New requirements emerged? → Add to Active
4. Decisions to log? → Add to Key Decisions
5. "What This Is" still accurate? → Update if drifted

**After each milestone** (via `/gsd-complete-milestone`):
1. Full review of all sections
2. Core Value check — still the right priority?
3. Audit Out of Scope — reasons still valid?
4. Update Context with current state

---
*Last updated: 2026-04-29 — v1.1 milestone started (variant support)*
