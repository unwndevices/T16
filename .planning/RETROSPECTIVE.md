# Retrospective

## Milestone: v1.0 — T16 Refactor

**Shipped:** 2026-04-04
**Phases:** 5 | **Plans:** 24 | **Tasks:** 47

### What Was Built
- Structured SysEx protocol replacing magic-byte dispatch
- Service-based firmware architecture (AppEngine, ConfigManager, ModeManager, InputProcessor)
- Full TypeScript web rewrite with custom design system (Radix + CSS tokens)
- Context split (ConnectionContext + ConfigContext), feature-domain organization
- CI pipeline (GitHub Actions: firmware build/test/format + web build/test/lint)
- Auto-bootloader firmware update flow (SysEx → USB re-enumeration → esptool-js)
- Note grid visualizer with firmware-matching scale algorithm
- MIDI monitor with real-time CC/Note visualization
- PWA with service worker and BLE connection infrastructure

### What Worked
- Protocol-first approach: defining SysEx contract before touching either codebase prevented integration surprises
- Parallel wave execution: 3 independent plans per wave ran simultaneously, cutting execution time significantly
- TDD pattern in plans: writing test stubs first caught issues early (e.g., scale interval bug found in test writing)
- Reference repo alignment (eisei/DROP patterns): reduced decision fatigue, consistent architecture

### What Was Inefficient
- BLE MIDI bridging was planned without acknowledging the SysEx chunking blocker from Phase 1 research. Should have been deferred from the roadmap.
- CC per-parameter sync payload mismatch (4 vs 5 bytes) wasn't caught until integration audit. Protocol conformance tests should verify both ends.
- Schema `additionalProperties: false` + firmware's undocumented `pal` field created an export-reimport bug. Schema should be validated against actual firmware output.

### Patterns Established
- Service extraction pattern: constructor injection, Update() loop, no FreeRTOS tasks
- Web context pattern: createContext<T|null>(null) + useX() with throw guard
- Design system: Radix primitives + CSS modules + token variables
- Test pattern: Unity for firmware (include .cpp in test_main), Vitest + RTL for web
- CI pattern: 3 parallel jobs (firmware-build, firmware-format, web-build)

### Key Lessons
1. **Validate protocol round-trips end-to-end early.** The CC payload mismatch silently dropped edits — no error, no feedback. Integration tests for each SysEx domain would have caught this.
2. **Schema should be tested against real firmware output.** The `pal` field gap shows that schema and firmware can drift when maintained separately.
3. **BLE needs firmware support for SysEx chunking.** Web-side BLE connection is necessary but not sufficient — firmware must handle BLE MTU fragmentation.

---
*Retrospective written: 2026-04-04*
