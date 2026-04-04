---
phase: 1
slug: protocol-data-foundation
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-04-03
---

# Phase 1 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | PlatformIO Unity (firmware native tests) + Node.js scripts (schema validation) |
| **Config file** | `platformio.ini` `[env:native]` section (needs creation — Wave 0) |
| **Quick run command** | `pio test -e native` |
| **Full suite command** | `pio test -e native && node schema/validate-types.js` |
| **Estimated runtime** | ~10 seconds |

---

## Sampling Rate

- **After every task commit:** Run `pio test -e native`
- **After every plan wave:** Run `pio test -e native && node schema/validate-types.js`
- **Before `/gsd:verify-work`:** Full suite must be green
- **Max feedback latency:** 10 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 01-01-01 | 01 | 0 | PROTO-01 | unit | `pio test -e native -f test_sysex_protocol` | ❌ W0 | ⬜ pending |
| 01-02-01 | 02 | 0 | PROTO-02 | unit | `pio test -e native -f test_param_set` | ❌ W0 | ⬜ pending |
| 01-03-01 | 03 | 0 | PROTO-03 | unit | `pio test -e native -f test_config_roundtrip` | ❌ W0 | ⬜ pending |
| 01-04-01 | 04 | 0 | PROTO-04 | unit | `pio test -e native -f test_sysex_validation` | ❌ W0 | ⬜ pending |
| 01-05-01 | 05 | 0 | PROTO-05 | unit | `pio test -e native -f test_version_handshake` | ❌ W0 | ⬜ pending |
| 01-06-01 | 06 | 0 | FWARCH-06 | unit | `pio test -e native -f test_config_manager` | ❌ W0 | ⬜ pending |
| 01-07-01 | 07 | 0 | WEBARCH-05 | unit | `node schema/validate-types.js` | ❌ W0 | ⬜ pending |
| 01-08-01 | 08 | 0 | FWFEAT-03 | unit | `pio test -e native -f test_migration` | ❌ W0 | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `platformio.ini` `[env:native]` section — PlatformIO native test environment for desktop execution
- [ ] `test/test_sysex_protocol/test_main.cpp` — SysEx framing and dispatch tests
- [ ] `test/test_config_manager/test_main.cpp` — ConfigManager load/save/dirty/flush tests
- [ ] `test/test_migration/test_main.cpp` — v103 to v200 migration tests
- [ ] `test/test_config_roundtrip/test_main.cpp` — Full config serialization round-trip
- [ ] `schema/validate-types.js` — Script to validate generated TS types match schema

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| SysEx byte order on real hardware | PROTO-01 | arduino_midi_library byte offsets may differ from Unity mock | Flash firmware, send SysEx via editor, verify log output |
| Flash wear reduction | FWARCH-06 | Cannot measure flash write cycles in native test | Monitor serial debug output during rapid config changes |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 10s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
