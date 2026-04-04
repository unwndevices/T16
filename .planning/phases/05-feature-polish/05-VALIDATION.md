---
phase: 5
slug: feature-polish
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-04-04
---

# Phase 5 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Vitest 3.x + React Testing Library |
| **Config file** | `editor-tx/vitest.config.ts` |
| **Quick run command** | `cd editor-tx && npx vitest run` |
| **Full suite command** | `cd editor-tx && npx vitest run && npx tsc --noEmit` |
| **Estimated runtime** | ~15 seconds |

---

## Sampling Rate

- **After every task commit:** Run `cd editor-tx && npx vitest run`
- **After every plan wave:** Run full suite command
- **Before `/gsd:verify-work`:** Full suite must be green
- **Max feedback latency:** 15 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| TBD | TBD | TBD | WEBFEAT-01 | component | `npx vitest run src/components/NoteGrid` | ❌ W0 | ⬜ pending |
| TBD | TBD | TBD | WEBFEAT-02 | integration | `npx vitest run` | ❌ W0 | ⬜ pending |
| TBD | TBD | TBD | WEBFEAT-05 | component | `npx vitest run src/pages/Monitor` | ❌ W0 | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] NoteGrid component test stubs (`editor-tx/src/components/NoteGrid/NoteGrid.test.tsx`)
- [ ] Monitor page test stubs (`editor-tx/src/pages/Monitor/Monitor.test.tsx`)
- [ ] Scale computation utility tests (`editor-tx/src/utils/scales.test.ts`)

*Existing test infrastructure (Vitest + RTL) covers framework setup.*

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Note grid colors match scale degrees | WEBFEAT-01 | Visual verification of HSL colors | Open Dashboard, select different scales, verify colors change per degree |
| PWA installable on mobile | WEBFEAT-02 | Requires mobile Chrome | Open on Android Chrome, verify "Add to Home Screen" prompt appears |
| BLE MIDI connection on mobile | WEBFEAT-02 | Requires T16 + mobile device | Install PWA, connect via BLE, verify config sync works |
| MIDI monitor real-time display | WEBFEAT-05 | Requires live MIDI input | Connect T16, play keys/CCs, verify messages appear in real-time |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 15s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
