# Phase 12 — T32 Hardware Bring-Up: Context

**Phase goal:** A physical T32 unit boots, scans both muxes, applies the validated key permutation, and persists calibration.
**Requirements:** T32-01, T32-02, T32-03, T32-04
**Depends on:** Phase 11 (HAL with `MultiplexerConfig` array consumed by `Adc::InitMux`)

---

## Locked Decisions

### D12.1 — Port strategy: reference `origin/3dot0`, clean-rewrite on v1.0 architecture
- Read 3dot0's `Adc::Update` and dual-mux scan logic as a **behavioral reference**.
- Rewrite the implementation on top of the v1.0 service architecture established in Phases 10–11 (`CurrentVariant::kMuxes`, signal-based key state changes, namespaced pinout).
- Do **not** cherry-pick 3dot0 commits — it predates the v1.0 refactor and would drag in stale patterns.
- Source for validation: 3dot0 commit hashes / file references should be cited in the plan and PR description so the lineage is traceable.

### D12.2 — Calibration storage: per-variant filenames
- T16 firmware reads/writes `/calibration_t16.json`.
- T32 firmware reads/writes `/calibration_t32.json`.
- A variant swap (re-flashing the same physical chip with the other variant binary) never reads stale data — the file simply doesn't exist and calibration runs.
- Migration of the legacy `/calibration_data.json`: on first T16 boot under the new naming, if `/calibration_t16.json` is missing and `/calibration_data.json` exists, rename it. (Researcher: confirm LittleFS supports rename or implement read-then-write-then-delete.)
- Avoids the deferred "calibration variant tagging on disk" work explicitly listed in REQUIREMENTS.md Future.

### D12.3 — T32 key permutation lives in `variant_t32.hpp` `MultiplexerConfig::keyMapping[]`
- The validated 32-key permutation from `origin/3dot0` decomposes into two 16-entry `keyMapping` arrays — one per mux entry inside `inline constexpr MultiplexerConfig kMuxes[]`.
- Permutation sits alongside `commonPin`, `enablePin`, `selectPins` because it is hardware-bound data.
- `Adc` reads `kMuxes[muxIdx].keyMapping[ch]` to translate physical scan position → logical key index.
- No standalone `t32_keymap.hpp`; no `#if defined(T32)` block inside `Adc.cpp`.

### D12.4 — T32-01 success gate: scan-time measurement
- Instrument `Adc::Update` (or the equivalent service entry point) to log loop time during scan.
- Acceptance: T32 scan loop time stays within the existing T16 budget (no per-mux inflation; both muxes batched per channel).
- No oscilloscope evidence required for the success criterion. If timing regresses unexpectedly, escalate to scope capture as a debug aid — not a Phase-12 gate.
- Planner: include a logged-output artifact (paste of scan-time numbers) in the verification step.

---

## Implementation Notes for Researcher / Planner

- **Scan correctness (T32-01):** the loop must be `for (ch = 0..15) { setSelectPins(ch); for (mux : kMuxes) read(mux.commonPin); }` — select pins set once per channel, both muxes read before incrementing. Matches 3dot0 reference.
- **Shared select pins (T32-02):** both muxes share `S0..S3`. `Adc::InitMux` must not configure select pins twice; the first mux configures them and subsequent muxes skip via `useSharedSelect`.
- **Calibration (T32-04):** the existing `CalibrationRoutine` in `main.cpp` should generalize to `kConfig.TOTAL_KEYS` iterations via the Phase 11 HAL — verify this still holds on physical T32.
- **Regression risk:** changes to `Adc::Update` and `Adc::InitMux` directly affect T16 (single-mux). Success criterion #5 (T16 still works) is non-negotiable; plan a real-hardware T16 smoke test as the last verification step.
- **Calibration filename migration (D12.2):** decide whether the rename happens in `setup()` once, in `DataManager::Load`, or in a dedicated migration helper. Researcher to investigate LittleFS rename support on the ESP32 Arduino core.

## Out of Scope for Phase 12

- Config schema variant discriminator (`"variant"` field) → Phase 13
- Cross-variant config load semantics → Phase 13
- Editor-tx detecting/displaying T32 → Phase 14
- Any T32-specific feature redesign (Koala mode rework, alternative slider) — REQUIREMENTS.md defers these

## Open Items for Researcher

1. Confirm `origin/3dot0` is still accessible and identify the canonical commits/files for scan logic + permutation array.
2. LittleFS rename API on the current ESP32 Arduino core (`LittleFS.rename` vs read+write+delete).
3. Concrete scan-time budget number on T16 today (microseconds per `Adc::Update`) so the T32 acceptance threshold is not arbitrary.
4. Whether `MultiplexerConfig::keyMapping` should be `std::array<uint8_t, 16>` (per-mux fixed) or sized by template parameter to leave room for future variants.
