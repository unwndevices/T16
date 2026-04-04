# Roadmap: T16 Refactor

## Milestones

- ✅ **v1.0 T16 Refactor** — Phases 1-5 (shipped 2026-04-04)
- 🔧 **v1.0 Gap Closure** — Phases 6-8 (from milestone audit)

## Phases

<details>
<summary>✅ v1.0 T16 Refactor (Phases 1-5) — SHIPPED 2026-04-04</summary>

- [x] Phase 1: Protocol & Data Foundation (5/5 plans) — completed 2026-04-03
- [x] Phase 2: Firmware Service Extraction (6/6 plans) — completed 2026-04-03
- [x] Phase 3: Web Rewrite (6/6 plans) — completed 2026-04-03
- [x] Phase 4: Integration & CI (4/4 plans) — completed 2026-04-04
- [x] Phase 5: Feature Polish (3/3 plans) — completed 2026-04-04

Full details: `.planning/milestones/v1.0-ROADMAP.md`

</details>

### v1.0 Gap Closure (Phases 6-8)

### Phase 6: CC Sync & Schema Fix
**Goal:** Fix CC per-parameter sync payload mismatch and add missing 'pal' field to schema so CC edits and config export-reimport work correctly.
**Requirements:** PROTO-02, WEBFEAT-04
**Gap Closure:** Closes CC domain payload mismatch, schema 'pal' field mismatch, "Config Edit (CC domain)" flow, "Config Import/Export" flow
**Plans:** 2 plans
Plans:
- [x] 06-01-PLAN.md — Add pal field to schema, types, defaults, and migration
- [ ] 06-02-PLAN.md — Fix CC per-parameter sync with dedicated 5-byte SysEx path

### Phase 7: Firmware Bug Confirmation & Tech Debt
**Goal:** Verify/complete LedManager pattern leak fix, implement calibration/factory reset SysEx commands, clean up code duplication and dead code.
**Requirements:** FWBUG-01
**Gap Closure:** Closes FWBUG-01 partial, calibration/factory reset stubs, getNoteNameWithOctave duplication, SYNC_CONFIRMED dead code

### Phase 8: BLE MIDI Bridging
**Goal:** Complete BLE MIDI data flow — implement firmware SysEx chunking over BLE MTU and wire web BLE connection to produce working MIDI I/O.
**Requirements:** WEBFEAT-02
**Gap Closure:** Closes BLE connection input/output null issue, "BLE MIDI Connection" flow

## Progress

| Phase | Milestone | Plans Complete | Status | Completed |
|-------|-----------|----------------|--------|-----------|
| 1. Protocol & Data Foundation | v1.0 | 5/5 | Complete | 2026-04-03 |
| 2. Firmware Service Extraction | v1.0 | 6/6 | Complete | 2026-04-03 |
| 3. Web Rewrite | v1.0 | 6/6 | Complete | 2026-04-03 |
| 4. Integration & CI | v1.0 | 4/4 | Complete | 2026-04-04 |
| 5. Feature Polish | v1.0 | 3/3 | Complete | 2026-04-04 |
| 6. CC Sync & Schema Fix | v1.0 gap | 0/2 | Planned | — |
| 7. Firmware Bug & Tech Debt | v1.0 gap | 0/0 | Pending | — |
| 8. BLE MIDI Bridging | v1.0 gap | 0/0 | Pending | — |

---
*Roadmap last updated: 2026-04-04 after Phase 6 planning*
