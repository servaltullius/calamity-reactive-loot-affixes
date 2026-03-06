# Config Load Pipeline Extraction

Date: 2026-03-06
Status: implemented

## Context

`EventBridge::LoadConfig()` still directly performed the affix config load pipeline after basic JSON validation:

- parse configured affixes
- index configured affixes
- synthesize runeword runtime affixes
- rebuild shared loot pools
- resize active counts

The individual steps already lived in separate helpers, but the orchestration still sat inline inside `LoadConfig()`, which made the config-load boundary harder to identify.

## Decision

Extract that orchestration into `BuildConfigDerivedAffixState(const nlohmann::json&, RE::TESDataHandler*)` implemented in its own `EventBridge.Config.LoadPipeline.cpp` source file.

## Why This Scope

- Keeps behavior unchanged
- Shrinks `LoadConfig()` into lifecycle orchestration
- Creates an obvious next seam for a future config loader or affix batch builder
- Avoids forcing `AffixRuntime` out of the private `EventBridge` type boundary yet

## Validation

- Add a failing runtime gate requiring the dedicated load-pipeline source and delegation
- Build SKSE DLL
- Run runtime gate checks
- Run full `ctest`
