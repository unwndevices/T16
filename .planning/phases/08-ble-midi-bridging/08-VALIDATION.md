---
phase: 8
slug: ble-midi-bridging
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-04-04
---

# Phase 8 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Vitest 4.1.2 |
| **Config file** | `editor-tx/vitest.config.ts` |
| **Quick run command** | `cd editor-tx && npx vitest run` |
| **Full suite command** | `cd editor-tx && npx vitest run` |
| **Estimated runtime** | ~5 seconds |

---

## Sampling Rate

- **After every task commit:** `cd editor-tx && npx vitest run`
- **After every plan wave:** `cd editor-tx && npx vitest run`
- **Before `/gsd:verify-work`:** Full suite must be green
- **Max feedback latency:** 5 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 08-01-01 | 01 | 1 | WEBFEAT-02a | unit | `cd editor-tx && npx vitest run src/services/bleBridge.test.ts -t "frameSysEx"` | ❌ W0 | ⬜ pending |
| 08-01-02 | 01 | 1 | WEBFEAT-02b | unit | `cd editor-tx && npx vitest run src/services/bleBridge.test.ts -t "reassemble"` | ❌ W0 | ⬜ pending |
| 08-02-01 | 02 | 2 | WEBFEAT-02c | unit | `cd editor-tx && npx vitest run src/services/bleBridge.test.ts -t "transport"` | ❌ W0 | ⬜ pending |
| 08-02-02 | 02 | 2 | WEBFEAT-02d | unit | `cd editor-tx && npx vitest run src/contexts/ConnectionContext.test.tsx` | ✅ | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `editor-tx/src/services/bleBridge.test.ts` — covers WEBFEAT-02a, 02b, 02c (BLE MIDI framing, reassembly, transport)
- Existing `ble.test.ts` covers packet parsing — no changes needed
- Existing `ConnectionContext.test.tsx` — extend for BLE transport path

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| BLE MIDI connection sends config dump | WEBFEAT-02 | Requires ESP32-S3 with BLE | Connect via BLE in Chrome, request config dump, verify config loads |
| BLE MIDI per-param edit reaches device | WEBFEAT-02 | Requires physical device | Connect BLE, edit a parameter, verify firmware receives SysEx |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 5s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
