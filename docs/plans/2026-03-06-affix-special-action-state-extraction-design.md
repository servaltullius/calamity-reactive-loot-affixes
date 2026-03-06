# Affix Special Action State Extraction

Date: 2026-03-06
Status: implemented

## Context

`EventBridge` still stored config-derived special-action index vectors as direct fields:

- cast on crit
- convert damage
- mind over matter
- archmage
- corpse explosion
- summon corpse explosion

Those buckets are rebuilt during config load and reset during config reload, but they still looked like core monolithic runtime state in the main header.

## Decision

Extract those vectors into `AffixSpecialActionState` and let `EventBridge` own them through a single `_affixSpecialActions` member.

## Why This Scope

- Keeps behavior unchanged
- Matches the earlier `AffixRegistryState` extraction style
- Makes config-derived special-action buckets easier to identify
- Reduces the next refactor step for trigger/action dispatch boundaries

## Validation

- Add a failing runtime gate requiring `AffixSpecialActionState.h`
- Build SKSE DLL
- Run runtime gate checks
- Run full `ctest`
