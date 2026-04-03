---
phase: 4
slug: integration-ci
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-04-04
---

# Phase 4 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework (firmware)** | Unity (PlatformIO native) |
| **Framework (web)** | Vitest 3.x + React Testing Library |
| **Config file (firmware)** | `platformio.ini` [env:native] |
| **Config file (web)** | `editor-tx/vitest.config.ts` |
| **Quick run command (firmware)** | `pio test -e native` |
| **Quick run command (web)** | `cd editor-tx && npm test` |
| **Full suite command** | `pio test -e native && cd editor-tx && npm test` |
| **Estimated runtime** | ~30 seconds |

---

## Sampling Rate

- **After every task commit:** Run relevant quick command (firmware or web depending on changes)
- **After every plan wave:** Run full suite command
- **Before `/gsd:verify-work`:** Full suite must be green
- **Max feedback latency:** 30 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| TBD | TBD | TBD | FWFEAT-02 | integration | `pio test -e native` | ❌ W0 | ⬜ pending |
| TBD | TBD | TBD | WEBFEAT-03 | component | `cd editor-tx && npm test` | ❌ W0 | ⬜ pending |
| TBD | TBD | TBD | WEBFEAT-04 | unit | `cd editor-tx && npm test` | ❌ W0 | ⬜ pending |
| TBD | TBD | TBD | TEST-03 | integration | `gh act --list` | ❌ W0 | ⬜ pending |
| TBD | TBD | TBD | TEST-04 | lint | `clang-format --dry-run` | ❌ W0 | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] Config validation tests for WEBFEAT-04 (`editor-tx/src/services/config-validation.test.ts`)
- [ ] Bootloader SysEx command test for FWFEAT-02 (`test/test_sysex_bootloader/test_main.cpp`)
- [ ] Upload component test for WEBFEAT-03 (`editor-tx/src/pages/Upload/Upload.test.tsx`)

*Existing test infrastructure covers framework setup — Wave 0 only needs test stubs.*

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| ESP32-S3 enters bootloader via SysEx | FWFEAT-02 | Requires physical hardware | Send BOOTLOADER SysEx, verify device re-enumerates as serial |
| esptool-js auto-connects after bootloader | WEBFEAT-03 | Requires USB + browser | Click Update, verify flash progress starts without manual serial select |
| Round-trip <100ms measured | SC-1 | Requires live device | Send param update, measure ACK return time in console |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 30s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
