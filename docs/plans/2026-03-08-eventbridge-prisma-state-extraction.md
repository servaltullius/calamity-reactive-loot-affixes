# 2026-03-08 EventBridge / PrismaTooltip State Extraction

## Goal

- Reduce the amount of mutable state defined inline in `EventBridge.h` and `PrismaTooltip.cpp`
- Keep existing behavior and public/private call sites stable
- Preserve runtime-gate coverage by teaching the tests about newly extracted state modules

## Plan

1. Extract `PrismaTooltip` shared constants, cache structs, refresh state, and telemetry counters into a dedicated include file.
2. Group `EventBridge` affix-runtime caches and instance-tracking storage into dedicated private state structs, while keeping the existing member names available through aliases.
3. Update runtime-gate policy checks so the extracted files are part of the contract.
4. Rebuild and rerun the generator tests plus SKSE runtime-gate tests.

## Notes

- `CombatRuntimeState`, `LootRuntimeState`, `RunewordRuntimeState`, and the coarse `std::recursive_mutex _stateMutex;` remain explicit in `EventBridge.h` because existing policy tests intentionally guard them.
- This phase is intentionally structural only. No gameplay logic or persistence format changes are included.
