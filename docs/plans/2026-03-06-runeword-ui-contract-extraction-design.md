# Runeword UI Contract Extraction

Date: 2026-03-06
Status: implemented

## Context

`EventBridge.h` owned both runtime behavior and Prisma/runeword UI DTO definitions. That made UI contract changes look like `EventBridge` internal changes and kept runtime gate tests coupled to the monolithic header.

## Options Considered

1. Keep DTOs nested in `EventBridge`
   - Lowest immediate cost
   - Keeps UI contract coupled to runtime singleton

2. Extract DTO definitions into a shared contracts header and keep compatibility aliases in `EventBridge`
   - Smallest safe step
   - Narrows the boundary without changing runtime behavior

3. Move runeword UI flows into a full `RunewordService`
   - Stronger long-term separation
   - Too large for a first refactor because runeword state, inventory lookups, and serialization are still shared

## Decision

Choose option 2.

Create `RunewordUiContracts.h` for:

- `RunewordBaseInventoryEntry`
- `RunewordRecipeEntry`
- `RunewordRuneRequirement`
- `RunewordPanelState`
- `OperationResult`

Keep `EventBridge` API stable by including the contracts header and exposing type aliases for existing member signatures.

## Why This First

- No save-format changes
- No hook-path changes
- Minimal Prisma/UI ripple
- Gives runtime gate tests an explicit contract file to assert against

## Validation

- Add a failing runtime gate requiring `RunewordUiContracts.h`
- Build SKSE DLL
- Run runtime gate checks
- Run full `ctest`
