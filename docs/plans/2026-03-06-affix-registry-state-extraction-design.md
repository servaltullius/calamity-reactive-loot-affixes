# Affix Registry State Extraction

Date: 2026-03-06
Status: implemented

## Context

`EventBridge` kept config-time affix lookup maps, trigger buckets, and loot-pool index vectors as direct fields. They are not save state and are rebuilt during config load, but they still looked like core runtime state inside the monolithic header.

## Decision

Extract those config-derived containers into `AffixRegistryState` and let `EventBridge` own them through a single `_affixRegistry` member.

Included in this extraction:

- affix id/token/label lookup containers
- trigger bucket index vectors
- loot pool index vectors

Excluded on purpose:

- active trigger caches
- shuffle bag runtime cursors/order
- serialization state
- combat/runtime mutable state

## Why This Scope

- Smaller than a full config loader/service extraction
- Makes config-derived state easier to identify in the header
- Leaves runtime behavior unchanged
- Reduces the next extraction step for config/indexing code

## Validation

- Add a failing runtime gate requiring `AffixRegistryState.h`
- Build SKSE DLL
- Run runtime gate checks
- Run full `ctest`
