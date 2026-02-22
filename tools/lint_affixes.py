#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any, Dict, Iterable, List, Optional, Set, Tuple


def _read_json(path: Path) -> Any:
    try:
        with path.open("r", encoding="utf-8") as f:
            return json.load(f)
    except FileNotFoundError:
        raise RuntimeError(f"File not found: {path}")
    except json.JSONDecodeError as e:
        raise RuntimeError(f"Invalid JSON ({path}:{e.lineno}:{e.colno}): {e.msg}")


def _as_dict(value: Any) -> Optional[Dict[str, Any]]:
    return value if isinstance(value, dict) else None


def _as_list(value: Any) -> Optional[List[Any]]:
    return value if isinstance(value, list) else None


def _is_number(value: Any) -> bool:
    return isinstance(value, (int, float)) and not isinstance(value, bool)


def _extract_spell_editor_id(value: Any) -> Optional[str]:
    if isinstance(value, str):
        return value
    if isinstance(value, dict):
        editor_id = value.get("spellEditorId")
        if isinstance(editor_id, str) and editor_id.strip():
            return editor_id.strip()
    return None


def _collect_spell_refs_from_action(
    action: Dict[str, Any], *, affix_id: str, out: List[Tuple[str, str]]
) -> None:
    action_type = action.get("type")
    if not isinstance(action_type, str):
        return

    def add(ref: Optional[str], ctx: str) -> None:
        if ref:
            out.append((ref, ctx))

    if action_type in {"CastSpell", "CastOnCrit", "ConvertDamage", "Archmage", "CorpseExplosion", "SummonCorpseExplosion", "SpawnTrap"}:
        add(_extract_spell_editor_id(action.get("spellEditorId")), f"{affix_id}:action.spellEditorId")

    if action_type == "CastSpellAdaptiveElement":
        spells = _as_dict(action.get("spells")) or {}
        for element in ("Fire", "Frost", "Shock"):
            add(_extract_spell_editor_id(spells.get(element)), f"{affix_id}:action.spells.{element}")

    if action_type == "SpawnTrap":
        extra_spells = _as_list(action.get("extraSpells")) or []
        for idx, entry in enumerate(extra_spells):
            add(_extract_spell_editor_id(entry), f"{affix_id}:action.extraSpells[{idx}]")

    if action_type in {"CastSpell", "CastSpellAdaptiveElement"}:
        mode_cycle = _as_dict(action.get("modeCycle")) or {}
        mode_spells = _as_list(mode_cycle.get("spells")) or []
        for idx, entry in enumerate(mode_spells):
            add(_extract_spell_editor_id(entry), f"{affix_id}:action.modeCycle.spells[{idx}]")


def _load_validation_contract() -> Tuple[Set[str], Set[str]]:
    contract_path = Path(__file__).resolve().parent / "affix_validation_contract.json"
    contract = _as_dict(_read_json(contract_path))
    if not contract:
        raise RuntimeError(f"Invalid validation contract (expected object): {contract_path}")

    def read_set(key: str) -> Set[str]:
        raw = _as_list(contract.get(key))
        if raw is None:
            raise RuntimeError(f"Validation contract missing array: {contract_path}:{key}")
        values: Set[str] = set()
        for idx, item in enumerate(raw):
            if not isinstance(item, str) or not item.strip():
                raise RuntimeError(f"Validation contract invalid value: {contract_path}:{key}[{idx}]")
            values.add(item.strip())
        if not values:
            raise RuntimeError(f"Validation contract array must not be empty: {contract_path}:{key}")
        return values

    return read_set("supportedTriggers"), read_set("supportedActionTypes")


def _format_json_path(path_tokens: Iterable[Any]) -> str:
    path = "$"
    for token in path_tokens:
        if isinstance(token, int):
            path += f"[{token}]"
        else:
            escaped = str(token).replace('"', '\\"')
            path += f'.{escaped}'
    return path


def _validate_schema(
    *,
    instance: Dict[str, Any],
    schema: Dict[str, Any],
    label: str,
    errors: List[str],
    max_errors: int = 20,
) -> None:
    try:
        from jsonschema import Draft202012Validator  # type: ignore
    except ImportError:
        errors.append(
            "Schema validation requires Python package 'jsonschema'. "
            "Install: python3 -m pip install --user jsonschema"
        )
        return

    validator = Draft202012Validator(schema)
    violation_count = 0
    for violation in sorted(
        validator.iter_errors(instance),
        key=lambda item: (len(list(item.absolute_path)), _format_json_path(item.absolute_path), item.message),
    ):
        errors.append(
            f"{label}: schema violation at {_format_json_path(violation.absolute_path)}: {violation.message}"
        )
        violation_count += 1
        if violation_count >= max_errors:
            errors.append(f"{label}: schema validation truncated after {max_errors} error(s).")
            break


def _lint_spec(
    spec: Dict[str, Any],
    *,
    errors: List[str],
    warnings: List[str],
    supported_triggers: Set[str],
    supported_action_types: Set[str],
) -> None:
    loot = _as_dict(spec.get("loot")) or {}
    shared_pool = loot.get("sharedPool")
    if shared_pool is not None and not isinstance(shared_pool, bool):
        errors.append("loot.sharedPool must be a boolean.")

    trap_global_max = loot.get("trapGlobalMaxActive")
    if trap_global_max is not None:
        if not isinstance(trap_global_max, int) or isinstance(trap_global_max, bool):
            errors.append("loot.trapGlobalMaxActive must be an integer (0 = unlimited).")
        elif trap_global_max < 0:
            errors.append("loot.trapGlobalMaxActive must be >= 0.")
        elif trap_global_max == 0:
            warnings.append("loot.trapGlobalMaxActive is 0 (unlimited). This can degrade performance with trap-heavy pools.")
        elif trap_global_max > 4096:
            warnings.append(f"loot.trapGlobalMaxActive is very high ({trap_global_max}). Consider keeping it <= 256.")

    cleanup_legacy = loot.get("cleanupInvalidLegacyAffixes")
    if cleanup_legacy is not None and not isinstance(cleanup_legacy, bool):
        errors.append("loot.cleanupInvalidLegacyAffixes must be a boolean.")

    deny_markers = loot.get("armorEditorIdDenyContains")
    if deny_markers is not None:
        if not isinstance(deny_markers, list):
            errors.append("loot.armorEditorIdDenyContains must be an array of strings.")
        else:
            for idx, marker in enumerate(deny_markers):
                if not isinstance(marker, str) or not marker.strip():
                    errors.append(f"loot.armorEditorIdDenyContains[{idx}] must be a non-empty string.")

    keywords = _as_dict(spec.get("keywords"))
    if not keywords:
        errors.append("Missing object: keywords")
        return

    affixes = _as_list(keywords.get("affixes"))
    if affixes is None:
        errors.append("Missing array: keywords.affixes")
        return

    spid_rules = _as_list(keywords.get("spidRules"))
    if spid_rules is not None:
        for idx, raw_rule in enumerate(spid_rules):
            rule = _as_dict(raw_rule)
            if not rule:
                errors.append(f"keywords.spidRules[{idx}] must be an object.")
                continue

            line = rule.get("line")
            if not isinstance(line, str):
                continue

            if line.lstrip().lower().startswith("deathitem"):
                errors.append(
                    f"keywords.spidRules[{idx}].line uses DeathItem distribution. "
                    "Hybrid policy requires Perk distribution + AddLeveledListOnDeath."
                )

    seen_ids: Dict[str, int] = {}
    seen_editor_ids: Dict[str, int] = {}
    generated_spells: Set[str] = set()
    spell_refs: List[Tuple[str, str]] = []

    for idx, raw in enumerate(affixes):
        affix = _as_dict(raw)
        if not affix:
            errors.append(f"keywords.affixes[{idx}] must be an object.")
            continue

        affix_id = affix.get("id")
        editor_id = affix.get("editorId")

        if not isinstance(affix_id, str) or not affix_id.strip():
            errors.append(f"keywords.affixes[{idx}].id must be a non-empty string.")
            affix_id = f"<index:{idx}>"
        if not isinstance(editor_id, str) or not editor_id.strip():
            errors.append(f"keywords.affixes[{idx}].editorId must be a non-empty string.")
            editor_id = f"<index:{idx}>"

        id_key = str(affix_id).strip().lower()
        if id_key in seen_ids:
            errors.append(
                f"Duplicate affix id (case-insensitive): '{affix_id}' at keywords.affixes[{seen_ids[id_key]}] and keywords.affixes[{idx}]."
            )
        else:
            seen_ids[id_key] = idx

        editor_key = str(editor_id).strip().lower()
        if editor_key in seen_editor_ids:
            errors.append(
                f"Duplicate affix editorId (case-insensitive): '{editor_id}' at keywords.affixes[{seen_editor_ids[editor_key]}] and keywords.affixes[{idx}]."
            )
        else:
            seen_editor_ids[editor_key] = idx

        records = _as_dict(affix.get("records"))
        if records:
            spell = _as_dict(records.get("spell"))
            if spell:
                spell_edid = spell.get("editorId")
                if isinstance(spell_edid, str) and spell_edid.strip():
                    generated_spells.add(spell_edid.strip())

            spells = _as_list(records.get("spells")) or []
            for s_idx, raw_spell in enumerate(spells):
                spell_entry = _as_dict(raw_spell)
                if not spell_entry:
                    errors.append(f"{affix_id}: records.spells[{s_idx}] must be an object.")
                    continue

                spell_edid = spell_entry.get("editorId")
                if isinstance(spell_edid, str) and spell_edid.strip():
                    generated_spells.add(spell_edid.strip())

        runtime = _as_dict(affix.get("runtime"))
        if not runtime:
            errors.append(f"{affix_id}: missing runtime object.")
            continue

        trigger = runtime.get("trigger")
        if not isinstance(trigger, str) or trigger not in supported_triggers:
            errors.append(f"{affix_id}: runtime.trigger must be one of {sorted(supported_triggers)}.")

        chance = runtime.get("procChancePercent")
        if chance is not None:
            if not _is_number(chance):
                errors.append(f"{affix_id}: runtime.procChancePercent must be a number (0-100).")
            else:
                if chance < 0.0 or chance > 100.0:
                    errors.append(f"{affix_id}: runtime.procChancePercent out of range: {chance} (expected 0-100).")

        for key in ("icdSeconds", "perTargetICDSeconds"):
            v = runtime.get(key)
            if v is None:
                continue
            if not _is_number(v):
                errors.append(f"{affix_id}: runtime.{key} must be a number (seconds).")
            elif v < 0.0:
                errors.append(f"{affix_id}: runtime.{key} must be >= 0.")

        conditions = _as_dict(runtime.get("conditions")) or {}

        for key in ("requireRecentlyHitSeconds", "requireRecentlyKillSeconds", "requireNotHitRecentlySeconds"):
            v = runtime.get(key)
            if v is None:
                v = conditions.get(key)
            if v is None:
                continue
            if not _is_number(v):
                errors.append(f"{affix_id}: runtime.{key} must be a number (seconds).")
            elif v < 0.0:
                errors.append(f"{affix_id}: runtime.{key} must be >= 0.")
            elif v > 600.0:
                warnings.append(f"{affix_id}: runtime.{key} is very high ({v}s). Consider <= 60s for combat readability.")

        lucky_hit_chance = runtime.get("luckyHitChancePercent")
        if lucky_hit_chance is None:
            lucky_hit_chance = conditions.get("luckyHitChancePercent")
        if lucky_hit_chance is not None:
            if not _is_number(lucky_hit_chance):
                errors.append(f"{affix_id}: runtime.luckyHitChancePercent must be a number (0-100).")
            elif lucky_hit_chance < 0.0 or lucky_hit_chance > 100.0:
                errors.append(f"{affix_id}: runtime.luckyHitChancePercent out of range: {lucky_hit_chance} (expected 0-100).")

        lucky_hit_coeff = runtime.get("luckyHitProcCoefficient")
        if lucky_hit_coeff is None:
            lucky_hit_coeff = conditions.get("luckyHitProcCoefficient")
        if lucky_hit_coeff is not None:
            if not _is_number(lucky_hit_coeff):
                errors.append(f"{affix_id}: runtime.luckyHitProcCoefficient must be a number (> 0).")
            elif lucky_hit_coeff <= 0.0:
                errors.append(f"{affix_id}: runtime.luckyHitProcCoefficient must be > 0.")
            elif lucky_hit_coeff > 5.0:
                warnings.append(f"{affix_id}: runtime.luckyHitProcCoefficient is very high ({lucky_hit_coeff}).")

        action = _as_dict(runtime.get("action"))
        if not action:
            errors.append(f"{affix_id}: runtime.action must be an object.")
            continue

        action_type = action.get("type")
        if not isinstance(action_type, str) or action_type not in supported_action_types:
            errors.append(f"{affix_id}: action.type must be one of {sorted(supported_action_types)}.")
            continue

        lucky_hit_configured = (
            (lucky_hit_chance is not None and _is_number(lucky_hit_chance) and lucky_hit_chance > 0.0) or
            (lucky_hit_coeff is not None and _is_number(lucky_hit_coeff) and lucky_hit_coeff > 0.0)
        )
        if lucky_hit_configured:
            lucky_supported = False
            if action_type in {"CastOnCrit", "ConvertDamage", "MindOverMatter", "Archmage"}:
                lucky_supported = True
            elif trigger in {"Hit", "IncomingHit"}:
                lucky_supported = True

            if not lucky_supported:
                warnings.append(
                    f"{affix_id}: luckyHit settings are ignored for this action/trigger combination "
                    f"(action.type={action_type}, trigger={trigger})."
                )

        if action_type in {"CastSpell", "CastSpellAdaptiveElement"}:
            evolution = action.get("evolution")
            if evolution is not None:
                evolution_obj = _as_dict(evolution)
                if not evolution_obj:
                    errors.append(f"{affix_id}: action.evolution must be an object.")
                else:
                    xp_per_proc = evolution_obj.get("xpPerProc")
                    if xp_per_proc is not None:
                        if not isinstance(xp_per_proc, int) or isinstance(xp_per_proc, bool) or xp_per_proc <= 0:
                            errors.append(f"{affix_id}: action.evolution.xpPerProc must be an integer > 0.")

                    thresholds = _as_list(evolution_obj.get("thresholds"))
                    if thresholds is None or len(thresholds) == 0:
                        errors.append(f"{affix_id}: action.evolution.thresholds must be a non-empty integer array.")
                    else:
                        for t_idx, value in enumerate(thresholds):
                            if not isinstance(value, int) or isinstance(value, bool) or value < 0:
                                errors.append(f"{affix_id}: action.evolution.thresholds[{t_idx}] must be an integer >= 0.")

                    multipliers = _as_list(evolution_obj.get("multipliers"))
                    if multipliers is None or len(multipliers) == 0:
                        errors.append(f"{affix_id}: action.evolution.multipliers must be a non-empty number array.")
                    else:
                        for m_idx, value in enumerate(multipliers):
                            if not _is_number(value) or value <= 0:
                                errors.append(f"{affix_id}: action.evolution.multipliers[{m_idx}] must be > 0.")

            mode_cycle = action.get("modeCycle")
            if mode_cycle is not None:
                mode_cycle_obj = _as_dict(mode_cycle)
                if not mode_cycle_obj:
                    errors.append(f"{affix_id}: action.modeCycle must be an object.")
                else:
                    spells = _as_list(mode_cycle_obj.get("spells"))
                    if spells is None or len(spells) == 0:
                        errors.append(f"{affix_id}: action.modeCycle.spells must be a non-empty array.")
                    else:
                        for s_idx, entry in enumerate(spells):
                            ref = _extract_spell_editor_id(entry)
                            if not ref:
                                errors.append(
                                    f"{affix_id}: action.modeCycle.spells[{s_idx}] must be a spellEditorId string or spell object."
                                )

                    labels = _as_list(mode_cycle_obj.get("labels"))
                    if labels is not None:
                        for l_idx, label in enumerate(labels):
                            if not isinstance(label, str):
                                errors.append(f"{affix_id}: action.modeCycle.labels[{l_idx}] must be a string.")

                    switch_every = mode_cycle_obj.get("switchEveryProcs")
                    if switch_every is not None:
                        if not isinstance(switch_every, int) or isinstance(switch_every, bool) or switch_every <= 0:
                            errors.append(f"{affix_id}: action.modeCycle.switchEveryProcs must be an integer > 0.")

                    manual_only = mode_cycle_obj.get("manualOnly")
                    if manual_only is not None and not isinstance(manual_only, bool):
                        errors.append(f"{affix_id}: action.modeCycle.manualOnly must be a boolean.")

        if action_type == "CastSpell":
            if not isinstance(action.get("spellEditorId"), str) and not isinstance(action.get("spellForm"), str):
                errors.append(f"{affix_id}: CastSpell requires spellEditorId or spellForm.")

        if action_type == "CastSpellAdaptiveElement":
            spells = _as_dict(action.get("spells"))
            if not spells:
                errors.append(f"{affix_id}: CastSpellAdaptiveElement requires action.spells object.")

            mode_cycle = _as_dict(action.get("modeCycle"))
            if mode_cycle is not None and mode_cycle.get("manualOnly") is not True:
                warnings.append(
                    f"{affix_id}: CastSpellAdaptiveElement modeCycle is coerced to manualOnly=true at runtime."
                )

        if action_type == "SpawnTrap":
            ttl = action.get("ttlSeconds")
            radius = action.get("radius")
            if not _is_number(ttl) or ttl <= 0.0:
                errors.append(f"{affix_id}: SpawnTrap requires ttlSeconds > 0.")
            if not _is_number(radius) or radius <= 0.0:
                errors.append(f"{affix_id}: SpawnTrap requires radius > 0.")
            if trigger == "DotApply" and action.get("requireWeaponHit", False):
                warnings.append(f"{affix_id}: DotApply + SpawnTrap has requireWeaponHit=true (will never fire).")

        if action_type == "MindOverMatter":
            damage_to_magicka_pct = action.get("damageToMagickaPct")
            if not _is_number(damage_to_magicka_pct):
                errors.append(f"{affix_id}: MindOverMatter requires action.damageToMagickaPct as a number.")
            elif damage_to_magicka_pct <= 0.0 or damage_to_magicka_pct > 100.0:
                errors.append(
                    f"{affix_id}: action.damageToMagickaPct out of range: {damage_to_magicka_pct} (expected >0 and <=100)."
                )

            max_redirect_per_hit = action.get("maxRedirectPerHit")
            if max_redirect_per_hit is not None:
                if not _is_number(max_redirect_per_hit):
                    errors.append(f"{affix_id}: action.maxRedirectPerHit must be a number when provided.")
                elif max_redirect_per_hit < 0.0:
                    errors.append(f"{affix_id}: action.maxRedirectPerHit must be >= 0.")

            if trigger != "IncomingHit":
                errors.append(
                    f"{affix_id}: MindOverMatter requires trigger=IncomingHit (current trigger={trigger})."
                )

        if str(affix_id).lower().startswith("internal_"):
            kid = _as_dict(affix.get("kid")) or {}
            kid_type = kid.get("type")
            if isinstance(kid_type, str) and kid_type != "None":
                warnings.append(f"{affix_id}: internal_ entry has kid.type={kid_type} (expected None).")

        _collect_spell_refs_from_action(action, affix_id=str(affix_id), out=spell_refs)

    # Validate spell editorId references that should exist in our generated plugin.
    for ref, ctx in spell_refs:
        if ref.startswith("CAFF_") and ref not in generated_spells:
            errors.append(
                f"Missing generated spell for reference '{ref}' ({ctx}). "
                "Define it under records.spell or records.spells[]."
            )


def _check_generated_sync(
    spec: Dict[str, Any],
    generated: Dict[str, Any],
    *,
    spec_path: Path,
    generated_path: Path,
    errors: List[str],
    warnings: List[str],
) -> None:
    # Keep this intentionally simple and robust:
    # - The generator re-serializes the spec into Data/SKSE/Plugins/CalamityAffixes/affixes.json.
    # - The serialized form may include defaulted fields (e.g., bools), so strict JSON equality
    #   against the author spec is not reliable.
    #
    # We treat the generated file as stale if it's older than the spec file, and also sanity-check
    # the affix id set to catch partial/corrupt outputs.
    try:
        spec_mtime = spec_path.stat().st_mtime_ns
        gen_mtime = generated_path.stat().st_mtime_ns
    except OSError as e:
        warnings.append(f"Unable to stat files for staleness check: {e}")
        spec_mtime = 0
        gen_mtime = 0

    if spec_mtime and gen_mtime and spec_mtime > gen_mtime:
        errors.append(
            "Generated runtime config is older than the spec (Data/SKSE/Plugins/CalamityAffixes/affixes.json looks stale). "
            "Re-run: dotnet run --project tools/CalamityAffixes.Generator -- --spec affixes/affixes.json --data Data"
        )

    spec_kw = _as_dict(spec.get("keywords")) or {}
    gen_kw = _as_dict(generated.get("keywords")) or {}
    spec_affixes = _as_list(spec_kw.get("affixes")) or []
    gen_affixes = _as_list(gen_kw.get("affixes")) or []

    def ids(arr: Iterable[Any]) -> Set[str]:
        out: Set[str] = set()
        for entry in arr:
            obj = _as_dict(entry)
            if not obj:
                continue
            v = obj.get("id")
            if isinstance(v, str) and v.strip():
                out.add(v.strip())
        return out

    spec_ids = ids(spec_affixes)
    gen_ids = ids(gen_affixes)

    missing = sorted(spec_ids - gen_ids)
    extra = sorted(gen_ids - spec_ids)
    if missing or extra:
        msg = "Generated runtime config affix id set mismatch."
        if missing:
            msg += f" Missing ids: {missing[:10]}{'...' if len(missing) > 10 else ''}."
        if extra:
            msg += f" Extra ids: {extra[:10]}{'...' if len(extra) > 10 else ''}."
        errors.append(
            msg
            + " Re-run: dotnet run --project tools/CalamityAffixes.Generator -- --spec affixes/affixes.json --data Data"
        )


def _collect_manifest_module_paths(manifest_path: Path) -> List[Path]:
    manifest = _read_json(manifest_path)
    if not isinstance(manifest, dict):
        raise RuntimeError(f"Modules manifest must be an object: {manifest_path}")

    root = manifest.get("root")
    keywords = _as_dict(manifest.get("keywords"))
    if not isinstance(root, str) or not root.strip():
        raise RuntimeError(f"Modules manifest requires a non-empty string 'root': {manifest_path}")
    if not keywords:
        raise RuntimeError(f"Modules manifest requires object 'keywords': {manifest_path}")

    def read_module_array(key: str) -> List[str]:
        raw = _as_list(keywords.get(key))
        if raw is None:
            raise RuntimeError(f"Modules manifest requires keywords.{key} array: {manifest_path}")
        out: List[str] = []
        for idx, value in enumerate(raw):
            if not isinstance(value, str) or not value.strip():
                raise RuntimeError(f"Modules manifest invalid path at keywords.{key}[{idx}]: {manifest_path}")
            out.append(value.strip())
        return out

    module_rel_paths: List[str] = [root.strip()]
    module_rel_paths.extend(read_module_array("tags"))
    module_rel_paths.extend(read_module_array("affixes"))
    module_rel_paths.extend(read_module_array("kidRules"))
    module_rel_paths.extend(read_module_array("spidRules"))

    resolved: List[Path] = []
    for rel in module_rel_paths:
        resolved.append((manifest_path.parent / rel).resolve())
    return resolved


def _check_module_manifest_sync(
    *,
    spec_path: Path,
    manifest_path: Path,
    errors: List[str],
    warnings: List[str],
) -> None:
    try:
        module_paths = _collect_manifest_module_paths(manifest_path)
    except RuntimeError as e:
        errors.append(str(e))
        return

    missing_paths = [p for p in module_paths if not p.exists()]
    if missing_paths:
        preview = ", ".join(str(p) for p in missing_paths[:3])
        suffix = "" if len(missing_paths) <= 3 else ", ..."
        errors.append(
            f"Modules manifest references missing files: {preview}{suffix}. "
            "Run: python3 tools/compose_affixes.py"
        )
        return

    try:
        spec_mtime = spec_path.stat().st_mtime_ns
    except OSError as e:
        warnings.append(f"Unable to stat spec file for modules staleness check: {e}")
        return

    latest_module_mtime = 0
    for module_path in module_paths:
        try:
            module_mtime = module_path.stat().st_mtime_ns
        except OSError as e:
            warnings.append(f"Unable to stat module file for staleness check: {module_path} ({e})")
            continue
        latest_module_mtime = max(latest_module_mtime, module_mtime)

    if latest_module_mtime and latest_module_mtime > spec_mtime:
        errors.append(
            f"Spec file is older than modular sources ({spec_path} is stale vs {manifest_path}). "
            "Run: python3 tools/compose_affixes.py"
        )


def main() -> int:
    parser = argparse.ArgumentParser(description="Lint Calamity affix spec for common authoring mistakes.")
    parser.add_argument("--spec", default="affixes/affixes.json", help="Path to affix spec JSON.")
    parser.add_argument("--schema", default="affixes/affixes.schema.json", help="Path to JSON schema for affix spec.")
    parser.add_argument(
        "--manifest",
        default=None,
        help="Optional modules manifest path (defaults to sibling affixes.modules.json when present).",
    )
    parser.add_argument(
        "--generated",
        default=None,
        help="Optional path to generated runtime config JSON to check for staleness (e.g. Data/SKSE/Plugins/CalamityAffixes/affixes.json).",
    )
    parser.add_argument("--strict", action="store_true", help="Treat warnings as errors.")
    args = parser.parse_args()

    spec_path = Path(args.spec)
    schema_path = Path(args.schema)
    errors: List[str] = []
    warnings: List[str] = []

    try:
        spec = _read_json(spec_path)
    except RuntimeError as e:
        print(f"[ERROR] {e}", file=sys.stderr)
        return 2

    try:
        schema = _read_json(schema_path)
    except RuntimeError as e:
        print(f"[ERROR] {e}", file=sys.stderr)
        return 2

    if args.manifest:
        manifest_path = Path(args.manifest)
        if not manifest_path.exists():
            errors.append(f"Modules manifest file not found: {manifest_path}")
        else:
            _check_module_manifest_sync(
                spec_path=spec_path,
                manifest_path=manifest_path,
                errors=errors,
                warnings=warnings,
            )
    else:
        inferred_manifest_path = spec_path.with_name("affixes.modules.json")
        if inferred_manifest_path.exists():
            _check_module_manifest_sync(
                spec_path=spec_path,
                manifest_path=inferred_manifest_path,
                errors=errors,
                warnings=warnings,
            )

    try:
        supported_triggers, supported_action_types = _load_validation_contract()
    except RuntimeError as e:
        print(f"[ERROR] {e}", file=sys.stderr)
        return 2

    if not isinstance(spec, dict):
        print("[ERROR] Spec JSON must be an object at top-level.", file=sys.stderr)
        return 2
    if not isinstance(schema, dict):
        print("[ERROR] Schema JSON must be an object at top-level.", file=sys.stderr)
        return 2

    _validate_schema(
        instance=spec,
        schema=schema,
        label=f"spec({spec_path})",
        errors=errors,
    )

    _lint_spec(
        spec,
        errors=errors,
        warnings=warnings,
        supported_triggers=supported_triggers,
        supported_action_types=supported_action_types,
    )

    if args.generated:
        gen_path = Path(args.generated)
        try:
            gen = _read_json(gen_path)
        except RuntimeError as e:
            errors.append(str(e))
        else:
            if isinstance(gen, dict):
                _validate_schema(
                    instance=gen,
                    schema=schema,
                    label=f"generated({gen_path})",
                    errors=errors,
                )
                _check_generated_sync(
                    spec,
                    gen,
                    spec_path=spec_path,
                    generated_path=gen_path,
                    errors=errors,
                    warnings=warnings,
                )
            else:
                errors.append(f"Generated runtime config JSON must be an object: {gen_path}")

    for w in warnings:
        print(f"[WARN] {w}", file=sys.stderr)
    for e in errors:
        print(f"[ERROR] {e}", file=sys.stderr)

    if errors or (args.strict and warnings):
        print(f"Lint FAILED: {len(errors)} error(s), {len(warnings)} warning(s).", file=sys.stderr)
        return 1

    print(f"Lint OK: {len(warnings)} warning(s).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
