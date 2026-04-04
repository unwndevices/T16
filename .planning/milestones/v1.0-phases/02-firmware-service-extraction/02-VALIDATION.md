---
phase: 2
slug: firmware-service-extraction
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-04-03
---

# Phase 2 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Unity (PlatformIO built-in) |
| **Config file** | `platformio.ini` `[env:native]` section (exists from Phase 1) |
| **Quick run command** | `pio test -e native -f test_<name>` |
| **Full suite command** | `pio test -e native` |
| **Estimated runtime** | ~5 seconds |

---

## Sampling Rate

- **After every task commit:** Run `pio test -e native -f test_<relevant>`
- **After every plan wave:** Run `pio test -e native`
- **Before `/gsd:verify-work`:** Full suite must be green + `pio run -e esp32s3` compiles clean
- **Max feedback latency:** 5 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 02-06-01 | 06 | 4 | FWARCH-01 | unit | `pio test -e native -f test_mode_manager` | ❌ W4 | ⬜ pending |
| 02-06-01 | 06 | 4 | FWARCH-05 | unit | `pio test -e native -f test_transport` | ❌ W4 | ⬜ pending |
| 02-06-01 | 06 | 4 | FWBUG-02 | unit | `pio test -e native -f test_slider` | ❌ W4 | ⬜ pending |
| 02-06-02 | 06 | 4 | TEST-01 | unit | `pio test -e native -f test_input_processor` | ❌ W4 | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 4 Test Requirements

- [ ] `test/test_mode_manager/test_main.cpp` — ModeManager transitions, allowed slider modes
- [ ] `test/test_transport/test_main.cpp` — transport abstraction dispatch (mock transports)
- [ ] `test/test_slider/test_main.cpp` — SetPosition bug fix verification (FWBUG-02)
- [ ] `test/test_input_processor/test_main.cpp` — key-to-note mapping
- [ ] Native test stubs may need extension for Mode/SliderMode enums and TouchSlider hardware deps

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| MIDI output works identically across USB/BLE/TRS | FWARCH-05 | Requires physical hardware with all transports | Flash firmware, play notes via each transport, verify correct NoteOn/NoteOff |
| LED pattern transitions don't leak | FWBUG-01 | Memory leak is cumulative over time | Run for 5+ minutes switching patterns, monitor free heap |
| Hardware test timeout | FWBUG-04 | Requires physically broken/missing key | Disconnect one key, run hardware test, verify 3s timeout |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 4 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 4 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 5s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
