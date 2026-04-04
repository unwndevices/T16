# Roadmap: T16 Refactor

## Milestones

- ✅ **v1.0 T16 Refactor** — Phases 1-5 (shipped 2026-04-04)
- 🔧 **v1.0 Gap Closure** — Phases 6-9 (from milestone audit)

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

### v1.0 Gap Closure (Phases 6-9)

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
**Plans:** 2 plans
Plans:
- [x] 07-01-PLAN.md — Fix LedManager UpdateTransition bug, implement calibration/factory reset SysEx handlers
- [x] 07-02-PLAN.md — Wire web calibration/reset buttons, remove Monitor duplication and SYNC_CONFIRMED dead code

### Phase 8: BLE MIDI Bridging
**Goal:** Complete BLE MIDI data flow — implement firmware SysEx chunking over BLE MTU and wire web BLE connection to produce working MIDI I/O.
**Requirements:** WEBFEAT-02
**Gap Closure:** Closes BLE connection input/output null issue, "BLE MIDI Connection" flow
**Plans:** 2/2 plans complete
Plans:
- [x] 08-01-PLAN.md — BLE MIDI bridge: SysEx framing, reassembly, and transport abstraction (TDD)
- [x] 08-02-PLAN.md — Wire BLE transport into ConnectionContext, ConfigContext, and sysex.ts

### Phase 9: UI Wiring Gap Closure
**Goal:** Add missing UI surfaces for BLE connect, config import/export, and make calibration/factory reset transport-agnostic so all features are user-accessible.
**Requirements:** WEBFEAT-02, WEBFEAT-04
**Gap Closure:** Closes BLE connect button missing, import/export UI missing, calibration/reset BLE bypass
**Plans:** 1/2 plans executed
Plans:
- [x] 09-01-PLAN.md — Add BLE connect buttons to NavBar/EmptyState, fix transport-agnostic calibration/reset
- [x] 09-02-PLAN.md — Add config import/export buttons to SettingsTab

## Progress

| Phase | Milestone | Plans Complete | Status | Completed |
|-------|-----------|----------------|--------|-----------|
| 1. Protocol & Data Foundation | v1.0 | 5/5 | Complete | 2026-04-03 |
| 2. Firmware Service Extraction | v1.0 | 6/6 | Complete | 2026-04-03 |
| 3. Web Rewrite | v1.0 | 6/6 | Complete | 2026-04-03 |
| 4. Integration & CI | v1.0 | 4/4 | Complete | 2026-04-04 |
| 5. Feature Polish | v1.0 | 3/3 | Complete | 2026-04-04 |
| 6. CC Sync & Schema Fix | v1.0 gap | 0/2 | Planned | — |
| 7. Firmware Bug & Tech Debt | v1.0 gap | 0/2 | Planned | — |
| 8. BLE MIDI Bridging | v1.0 gap | 2/2 | Complete   | 2026-04-04 |
| 9. UI Wiring Gap Closure | v1.0 gap | 1/2 | In Progress|  |

---
*Roadmap last updated: 2026-04-04 after Phase 9 planning*
