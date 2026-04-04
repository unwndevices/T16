---
phase: 6
slug: cc-sync-schema-fix
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-04-04
---

# Phase 6 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | vitest (editor-tx) |
| **Config file** | editor-tx/vite.config.js |
| **Quick run command** | `cd editor-tx && npx vitest run --reporter=verbose` |
| **Full suite command** | `cd editor-tx && npx vitest run --reporter=verbose` |
| **Estimated runtime** | ~5 seconds |

---

## Sampling Rate

- **After every task commit:** Run `cd editor-tx && npx vitest run --reporter=verbose`
- **After every plan wave:** Run `cd editor-tx && npx vitest run --reporter=verbose`
- **Before `/gsd:verify-work`:** Full suite must be green
- **Max feedback latency:** 5 seconds

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------|-------------------|-------------|--------|
| 06-01-01 | 01 | 1 | PROTO-02 | unit | `grep "sendCCParamUpdate" editor-tx/src/services/protocol.js` | ❌ W0 | ⬜ pending |
| 06-01-02 | 01 | 1 | PROTO-02 | unit | `grep "domain.*bank.*ccIndex.*channel.*id" editor-tx/src/services/protocol.js` | ❌ W0 | ⬜ pending |
| 06-02-01 | 02 | 1 | WEBFEAT-04 | unit | `grep '"pal"' editor-tx/src/schema/` | ❌ W0 | ⬜ pending |
| 06-02-02 | 02 | 1 | WEBFEAT-04 | unit | `grep "pal" editor-tx/src/services/config.js` | ❌ W0 | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- Existing infrastructure covers all phase requirements — fixes are to existing protocol and schema code.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| CC edit reaches firmware | PROTO-02 | Requires physical device | Edit CC channel in web editor, verify firmware receives correct SysEx |
| Config round-trip with pal | WEBFEAT-04 | Requires device config dump | Export config from device, reimport, verify pal field preserved |

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references
- [ ] No watch-mode flags
- [ ] Feedback latency < 5s
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
