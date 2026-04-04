# Phase 9: UI Wiring Gap Closure - Context

**Gathered:** 2026-04-04
**Status:** Ready for planning
**Mode:** Auto-generated (discuss skipped per user request)

<domain>
## Phase Boundary

Add missing UI surfaces for BLE connect, config import/export, and make calibration/factory reset transport-agnostic so all features are user-accessible.

</domain>

<decisions>
## Implementation Decisions

### Claude's Discretion
All implementation choices are at Claude's discretion — discuss phase was skipped per user request. Use ROADMAP phase goal, success criteria, and codebase conventions to guide decisions.

Key gaps from milestone audit:
1. No BLE connect button — connectBLE() exists in ConnectionContext but no UI calls it
2. No import/export UI — importConfig/exportConfig functions exist in ConfigContext but no buttons
3. Calibration/factory reset bypass BLE — Dashboard SettingsTab uses `output` instead of `transport ?? output`

</decisions>

<code_context>
## Existing Code Insights

Codebase context will be gathered during plan-phase research.

</code_context>

<specifics>
## Specific Ideas

No specific requirements — discuss phase skipped. Refer to ROADMAP phase description and milestone audit gaps.

</specifics>

<deferred>
## Deferred Ideas

None — discuss phase skipped.

</deferred>
