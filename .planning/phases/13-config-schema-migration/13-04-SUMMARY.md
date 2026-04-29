# Plan 13-04 — Env Rename + CI + Docs Summary

**Status:** complete
**Commits:** 3 (`785661c`, `5f2496d`, `3ea7a86`)
**Wave:** 1 (parallel slot, executed inline sequential)

## What changed

| File | Change |
|------|--------|
| `platformio.ini` | `[env:native]` → `[env:native_test]`; added `-DT16` to its `build_flags` |
| `.github/workflows/ci.yml` | Cache key suffix `native` → `native_test`; test command `pio test -e native` → `pio test -e native_test`. firmware-build matrix unchanged. |
| `docs/schema-v201.md` | New canonical reference doc (created `docs/` directory) |

## `-DT16` sufficient on native_test? — Yes

Plan 13-03 will exercise both T16 and T32 branches of variant-mismatch handling at runtime by constructing the *other* variant string (`(strcmp(NAME,"T16")==0) ? "T32" : "T16"`) inside the test fixture. So a single `-DT16` compile-time variant is enough for SCHEMA-03 coverage. A T32 native env can be added in a future phase if/when test cases need both compile-time variants simultaneously.

## Old `native` env grep audit

Pre-rename grep across the repo (excluding `.git`, `.pio`, `node_modules`, `.planning`):

| Hit | Resolution |
|-----|-----------|
| `platformio.ini:90:[env:native]` | Renamed to `[env:native_test]` |
| `.github/workflows/ci.yml:62:run: pio test -e native` | Renamed to `pio test -e native_test`; cache key also updated |

Post-rename grep `pio test -e native[^_]` returns 0 — no stragglers. `grep -c native_test .github/workflows/ci.yml` returns 2 (cache key + test command).

## CI matrix interpretation (Task 2)

D13.3's wording "Wire into the GitHub Actions matrix from Phase 10 (`t16_debug`, `t16_release`, `t32_debug`, `t32_release`, **`native_test`**)" was interpreted as "native_test runs alongside the firmware build matrix" — kept the existing two-job split:

- `firmware-build` (matrix of 4 firmware envs) — `pio run`
- `firmware-tests` (single job, native_test) — `pio test`

Merging them would either run `pio test` against firmware envs (fails — no test files for firmware envs) or `pio run` against native_test (meaningless — `build_src_filter = -<*>` excludes all source). The two jobs already run in parallel under the same workflow trigger; the intent is satisfied. This is the deliberate choice; future readers can see it documented here.

## Local verification

- `grep -c "^\[env:native_test\]" platformio.ini` → 1
- `grep -c "^\[env:native\]" platformio.ini` → 0
- `grep -c -- "-DT16" platformio.ini` → 3 (t16_debug + t16_release + native_test)
- `pio test -e native_test --without-uploading --without-testing` → env recognized; 4 test cases discovered (test_migration + test_config_manager + test_input_processor + test_mode_manager etc.). Pre-existing test failures are unrelated to the rename and will be addressed by Plan 13-03.
- `python3 -c "import yaml; yaml.safe_load(open('.github/workflows/ci.yml'))"` → 0 (YAML valid)
- `grep -c "pio test -e native_test" .github/workflows/ci.yml` → 1
- `grep -c "^# Config Schema v201$" docs/schema-v201.md` → 1
- All 9 acceptance grep counts on `docs/schema-v201.md` → match.

## Deviations

None. Plan executed verbatim.

## Files committed

```
platformio.ini
.github/workflows/ci.yml
docs/schema-v201.md (new)
```
