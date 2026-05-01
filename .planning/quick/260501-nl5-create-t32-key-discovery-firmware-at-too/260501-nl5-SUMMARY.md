---
phase: 260501-nl5
plan: 01
subsystem: tools
tags: [t32, firmware, discovery, fastled, tools]
tech_stack:
  added:
    - "PlatformIO standalone env: t32_key_discovery (espressif32 / arduino / unwn_s3)"
    - "FastLED ^3.6.0 (only dep — no MIDI/USB/BLE/ArduinoJson baggage)"
  patterns:
    - "Single-pass discovery state machine with per-channel hold timer"
    - "Largest-delta channel selection to disambiguate neighbour crosstalk"
    - "Debounced MODE re-arm without restarting the whole flow"
key_files:
  created:
    - tools/key_discovery/platformio.ini
    - tools/key_discovery/src/main.cpp
    - tools/key_discovery/README.md
  modified: []
decisions:
  - "Standalone PlatformIO project, not extending env_common, to keep the binary free of TinyUSB/BLE/MIDI/JSON deps"
  - "Pick largest-delta channel when multiple channels exceed kPressDelta — handles neighbour-key crosstalk deterministically"
  - "waitForRelease() between keys to prevent a still-held key from polluting the next-key wait"
  - "MODE in-loop press re-arms the current key (no restart needed); MODE held at boot calls ESP.restart()"
metrics:
  duration_minutes: 5
  tasks_completed: 2
  files_changed: 3
  completed: "2026-05-01T15:07:00Z"
---

# Phase 260501-nl5 Plan 01: T32 Key Discovery Firmware Summary

Standalone PlatformIO firmware under `tools/key_discovery/` that empirically maps T32 matrix key index (0..31) to electrical mux channel (`mux*16 + ch`) and emits a copy-paste-ready `k3dot0Keys[32]` array on Serial.

## What was built

A two-task quick task delivering a single-purpose discovery tool:

1. **Standalone PlatformIO project** (`platformio.ini` + `README.md`) — uses `board=unwn_s3`, FastLED-only `lib_deps`, `-I ../../src` to resolve `pinout_t32.h`, monitor speed 115200 with the `esp32_exception_decoder` filter. Does **not** extend the root `env_common`, so it pulls in zero MIDI/USB/BLE/JSON dependencies.
2. **Discovery firmware** (`src/main.cpp`, 346 lines) — baseline scan, per-key red→green flow, MODE handling (boot-time `ESP.restart()` + in-loop re-arm), final array emission, and slow-green completion blink.

### Build command and result

```sh
cd tools/key_discovery && pio run
# ========================= [SUCCESS] Took 45.14 seconds =========================
# Flash: 30.8% (403,332 / 1,310,720 bytes)
# RAM:    7.2% ( 23,504 /   327,680 bytes)
```

The main firmware build was re-run after both commits to confirm coexistence:

```sh
pio run -e t16_release
# t16_release    SUCCESS   00:01:20.112
```

## Commits

- `845d705` — feat(260501-nl5): scaffold standalone T32 key-discovery PlatformIO project
- `956981f` — feat(260501-nl5): implement T32 key-discovery firmware

## Operator workflow (recap)

1. `cd tools/key_discovery && pio run -t upload`
2. `pio device monitor` (115200 baud)
3. Release MODE after upload. (If MODE was held at boot, the firmware logs `MODE held at boot — restarting` and reboots — a deliberate guardrail.)
4. For each red LED, press the physical key directly under it. The LED turns green and Serial prints `key i -> mux=X ch=Y (electrical=Z)`. Release.
5. Mistake? Press MODE during a wait to re-arm that index. The current LED stays red and the firmware re-listens.
6. After all 32 keys are mapped, copy the printed `inline constexpr uint8_t k3dot0Keys[32] = { ... };` block from Serial and paste it over the existing array inside `namespace detail` in `src/variant_t32.hpp`.

## Deviations from Plan

None — the plan was executed as written. Two small clarifications:

- The verify command for Task 1 (`! grep -q "TinyUSB\|BLE-MIDI\|NimBLE" tools/key_discovery/platformio.ini`) initially failed because the file's leading comment listed those tokens by name. The comment was rewritten to refer to "USB-stack, Bluetooth, MIDI, and JSON dependencies" instead — same intent, satisfies the negative grep.
- Build succeeded on the first attempt with no compiler diagnostics.

## Caveats

- **`pinout::t32::COM2 = GPIO17` is a placeholder** pending T32 schematic verification (Plan 12.03.5.A). It maps to ESP32-S3 ADC2_CH6. ADC2 conflicts with WiFi on classic ESP32, but on ESP32-S3 ADC2 is usable when WiFi is off — and this firmware never starts WiFi, so the conflict is irrelevant for discovery. Confirm GPIO17 against the T32 schematic before trusting Mux 1 readings on real hardware.
- **`static_assert` coupling in `src/variant_t32.hpp`:** the existing `static_assert` blocks validate the `k3dot0Keys[]` permutation against each `kMuxes[].keyMapping` array at compile time. When Phase 12 hardware bring-up consumes this output, the new `k3dot0Keys[]` and the `kMuxes[].keyMapping` arrays will need to be updated together — the build will refuse to link otherwise. This is a feature, not a bug.
- **MODE = GPIO0** is the same line the bootloader uses for download mode. Holding it at boot puts the chip into download mode *before* this firmware runs; once running, the firmware reads it as an active-low input with internal pull-up. The boot-time `ESP.restart()` guard exists so the operator gets a clean re-boot if they accidentally held MODE through the upload reset cycle.
- **No automated hardware verification:** this is bench-tool firmware. The flash + UAT step belongs to the operator running the discovery flow against a real T32 board.

## Verification

- `tools/key_discovery/platformio.ini` exists with `board = unwn_s3`, `FastLED` in `lib_deps`, `monitor_speed = 115200`, and contains no TinyUSB/BLE-MIDI/NimBLE references.
- `tools/key_discovery/src/main.cpp` builds cleanly via `pio run` (SUCCESS in 45 s, 30.8% flash).
- `tools/key_discovery/README.md` documents build/flash/monitor/paste workflow with caveats.
- `pio run -e t16_release` still succeeds (80 s) — main firmware build is unaffected.
- `git diff` confirms `src/variant_t32.hpp` and root `platformio.ini` are unmodified.

## Self-Check: PASSED

Files verified to exist:
- FOUND: tools/key_discovery/platformio.ini
- FOUND: tools/key_discovery/src/main.cpp
- FOUND: tools/key_discovery/README.md

Commits verified in `git log`:
- FOUND: 845d705 (Task 1 — scaffold)
- FOUND: 956981f (Task 2 — implement firmware)
