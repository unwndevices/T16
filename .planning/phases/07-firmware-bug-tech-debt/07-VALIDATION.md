---
phase: 7
slug: firmware-bug-tech-debt
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-04-04
---

# Phase 7 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework (firmware)** | Unity (PlatformIO native env) |
| **Framework (web)** | Vitest |
| **Config file (firmware)** | `platformio.ini` [env:native] |
| **Config file (web)** | `editor-tx/vitest.config.ts` |
| **Quick run (firmware)** | `pio test -e native` |
| **Quick run (web)** | `cd editor-tx && npx vitest run` |
| **Full suite** | `pio test -e native && cd editor-tx && npx vitest run` |
| **Estimated runtime** | ~10 seconds |

---

## Sampling Rate

- **After every task commit:** `cd editor-tx && npx tsc --noEmit && npx vitest run` (web) / `pio test -e native` (firmware)
- **After every plan wave:** Full suite
- **Before `/gsd:verify-work`:** Full suite must be green
- **Max feedback latency:** 10 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 07-01-01 | 01 | 1 | FWBUG-01 | code-review | `grep -c "unique_ptr" src/Libs/Leds/LedManager.hpp` | N/A | ⬜ pending |
| 07-01-02 | 01 | 1 | N/A | unit | `pio test -e native` | ✅ | ⬜ pending |
| 07-02-01 | 02 | 1 | N/A | unit | `cd editor-tx && npx vitest run src/services/midi.test.ts` | ✅ | ⬜ pending |
| 07-02-02 | 02 | 1 | N/A | unit | `cd editor-tx && npx tsc --noEmit` | N/A | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- Existing infrastructure covers all phase requirements. No new test framework needed.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Calibration SysEx triggers restart | N/A | Requires physical device | Press calibration button in web editor, verify device restarts and enters calibration mode |
| Factory reset SysEx resets config | N/A | Requires physical device | Press factory reset button, verify device restarts with default config |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 10s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
