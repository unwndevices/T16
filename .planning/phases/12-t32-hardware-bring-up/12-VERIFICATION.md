---
status: validation_deferred
phase: 12-t32-hardware-bring-up
plans_complete: 4/4
software_gates: passed
hardware_gates: deferred (milestone v1.1 batch)
verified: 2026-04-29
---

# Phase 12 — T32 Hardware Bring-Up: Verification

All four plans (12.01–12.04) are code-complete. Software gates pass; hardware-bound gates intentionally deferred to milestone v1.1 batch per orchestrator convention.

Actual PlatformIO env names: `t16_debug`, `t16_release`, `t32_debug`, `t32_release` (the original plan text referenced legacy `esp32s3` / `release` aliases).

---

## Software gates (binding for plan completion)

| Plan | Gate | Status |
|---|---|---|
| 12.01 | All four envs (`t16_debug`, `t16_release`, `t32_debug`, `t32_release`) build clean | PASSED |
| 12.01 | `grep '"/calibration_data.json"'` returns 0 matches at LittleFS call sites in `src/` (1 residue: the `kLegacyCalibrationPath` constant in variant.hpp — intentional, used by migration) | PASSED |
| 12.02 | `static_assert` in `variant_t32.hpp` confirms permutation = inversion of `origin/3dot0:src/main.cpp:10`. Caught and corrected a typo in the plan's hand-derived expected values (mux 0[12..13] and mux 1[12..13]) — see 12-02-SUMMARY.md | PASSED |
| 12.03 | `Adc::ReadValues` uses `_muxes[m].keyMapping[iterator]` and wraps iterator at 16 (not `_channels.size()`) | PASSED |
| 12.03 | All four envs build clean | PASSED |
| 12.04 | `CalibrationData::kSize == TOTAL_KEYS` (T16=16, T32=32) | PASSED |
| 12.04 | `SCAN_TIMING_LOG` flag compiles and emits per-pass µs over Serial (gated, zero overhead in default builds) | PASSED |

---

## Validation Deferred → Milestone v1.1 Batch

Per orchestrator-stated convention for Milestone v1.1: **hardware verification of Phase 12 is intentionally deferred to a single end-of-milestone batch.** The user has T32 hardware on hand but will run hardware tests for all v1.1 phases together. Plans 12.01.3, 12.02.3, 12.03.5, and 12.04.5 are tagged `validation_deferred: true` (`type: checkpoint:human-verify`) and DO NOT block plan completion.

The following items must be executed and recorded here before the milestone is closed:

### From Plan 12.01 (`task 12.01.3`)
- [ ] T16 unit with existing `/calibration_data.json` boots, migrates to `/calibration_t16.json`, retains thresholds. Serial log paste:
  ```
  (paste here)
  ```
- [ ] Fresh T32 boots without `/calibration_t32.json`, runs calibration, persists. Serial log paste:
  ```
  (paste here)
  ```

### From Plan 12.02 (`task 12.02.3`)
- [ ] T32 raster press of all 32 keys yields `key_index = 0..31` sequence. Serial log paste:
  ```
  (paste here)
  ```
- [ ] Spot-check (4 corners + 4 edges): every press lands on the expected logical index. Notes:
  ```
  (paste here)
  ```

### From Plan 12.03 (`task 12.03.5`)
- [ ] **A.** `pinout::t32::COM2` matches T32 schematic. Schematic GPIO confirmed: `____`. (If different from placeholder `17`, file the source-edit alongside this entry.)
- [ ] **B.** Oscilloscope/logic-analyzer capture of S0..S3 + COM/COM2 — saved as `12-VERIFICATION/scope-s0s3.png`. (Optional debug aid — NOT a binding gate per CONTEXT D12.4.)
- [ ] **C.** Scan-time measurement (T32-01 binding):
  - T16 baseline median µs/pass: `____`
  - T32 measured median µs/pass: `____`
  - Ratio: `____` (acceptance: ≤ 2.0)
- [ ] **D.** All-32-keys-distinct: each press changes exactly one logical-key slot. Notes:
  ```
  (paste here)
  ```

### From Plan 12.04 (`task 12.04.5`)
- [ ] **A.** T16 regression smoke — all 16 keys + slider + persistence. Notes:
  ```
  (paste here)
  ```
- [ ] **B.** T32 dual-mux scan + permutation — full procedure. Notes:
  ```
  (paste here)
  ```
- [ ] **C.** Scan-time measurement (duplicates 12.03.5.C — record once, link from both entries).
- [ ] **D.** Calibration persistence cross-check — `/calibration_t32.json` contains 32 entries each. Notes:
  ```
  (paste here)
  ```
- [ ] **E.** Schematic GPIO confirmation (duplicates 12.03.5.A).

---

## Phase-12 success criteria (from ROADMAP)

| # | Criterion | Gating plan / task |
|---|---|---|
| 1 | Multi-mux ADC scan sets S0..S3 once per channel; reads each commonPin before incrementing — verified by oscilloscope or scan-time measurement | Plan 12.03 (scan logic) + 12.03.5.C / 12.04.5.C (scan-time, BINDING) |
| 2 | T32 dual-mux init matches `origin/3dot0` (shared select, separate common); all 32 keys produce distinct readings | Plan 12.03 + 12.03.5.D (deferred) |
| 3 | T32 key permutation decomposes into two 16-entry `keyMapping[]` arrays; physical positions match logical indices | Plan 12.02 (static_assert, BINDING) + 12.02.3 (deferred press test) |
| 4 | Calibration completes on T32; calibration file survives power cycle | Plan 12.01 + 12.04 + 12.04.5.D (deferred) |
| 5 | T16 hardware remains functional (regression) | Plan 12.04.5.A (deferred) |

Criteria 1, 3, 4 (calibration code path) have BINDING software gates. Criteria 2, 4 (persistence on real hardware), and 5 are deferred.
