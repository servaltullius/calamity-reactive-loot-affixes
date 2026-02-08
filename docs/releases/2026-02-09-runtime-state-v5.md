# CalamityAffixes Release Notes (2026-02-09)

## Prerequisites

- Required:
  - SKSE64 (runtime-matching build): https://skse.silverlock.org/
  - Address Library for SKSE Plugins: https://www.nexusmods.com/skyrimspecialedition/mods/32444
  - Prisma UI (tooltip/control panel framework): https://www.nexusmods.com/skyrimspecialedition/mods/148718
- Recommended:
  - SkyUI: https://www.nexusmods.com/skyrimspecialedition/mods/12604
  - Keyword Item Distributor (KID): https://www.nexusmods.com/skyrimspecialedition/mods/55728
  - Spell Perk Item Distributor (SPID): https://www.nexusmods.com/skyrimspecialedition/mods/36869
- Optional:
  - MCM Helper: https://www.nexusmods.com/skyrimspecialedition/mods/53000
  - Inventory Interface Information Injector (I4): https://www.nexusmods.com/skyrimspecialedition/mods/85702

## Summary

- Refactored runtime state tracking from instance-only keys to composite keys `(instanceKey, affixToken)` to prevent cross-affix state bleed.
- Added serialization v5 with explicit per-affix runtime-state record (`IRST`) and legacy compatibility for v1-v4 loads.
- Updated runtime state lifecycle cleanup so deleting a dropped instance removes all runtime states for that instance.
- Regenerated shipped plugin assets (`CalamityAffixes.esp`, `*_DISTR.ini`, `*_KID.ini`, Papyrus `.pex`, and `CalamityAffixes.dll`) and rebuilt MO2 package output.

## Player-Facing Impact

- Runeworded items can now coexist with inherited/supplemental affix state without mode/evolution state collisions.
- Affix tooltip runtime details (evolution/mode) are resolved against the correct affix token on the selected instance.

## Technical Notes

- New helper: `InstanceStateKey` and hash utility for `unordered_map` runtime-state storage.
- Runtime-state map is now keyed by `InstanceStateKey`, not raw instance key.
- Save/load path writes and reads separate instance runtime-state records while preserving old-save migration behavior.

## Verification

- `cmake --build skse/CalamityAffixes/build.linux-clangcl-rel -j`
- `ctest --test-dir skse/CalamityAffixes/build.linux-clangcl-rel --output-on-failure`
- `tools/build_mo2_zip.sh`
- Produced: `dist/CalamityAffixes_MO2_2026-02-09.zip`
