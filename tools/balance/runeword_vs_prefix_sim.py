#!/usr/bin/env python3
"""Runeword vs proc prefix (docs/plans/2026-10-09-runeword-prefix-slot.md): what does each give per minute of melee combat?

Reads affixes/affixes.json. For every lootable prefix and every runeword it
estimates procs/min from trigger, proc chance, lucky-hit gate and ICD, then
the payoff per role:
  damage   hostile Health effect on the target: damage/min (flat + % of hit)
  sustain  self Health: healing/min
  buff     self non-Health: uptime fraction
  control  hostile non-Health: uptime fraction on the current target
  special  engine actions (CastOnCrit, traps, ConvertDamage...): procs/min only
plus whether it carries an always-on passive (the runeword "bundle").

Combat model (per minute, melee, one target at a time) is a parameter set,
not a claim about real play; run the variants and read the ordering.
"""
from __future__ import annotations

import argparse
import json
import random
import statistics as st
from collections import Counter, defaultdict
from pathlib import Path

MODELS = {
    # name: hits dealt/min, hits taken/min, kills/min, crits/min, avg physical hit
    "melee-normal": dict(hit=40, taken=20, kill=2, crit=6, phys=35, lowhp=0.5),
    "melee-boss": dict(hit=40, taken=25, kill=0.2, crit=6, phys=39, lowhp=1.0),
    "archer": dict(hit=15, taken=8, kill=2, crit=2, phys=45, lowhp=0.3),
}


def procs_per_min(rt: dict, m: dict) -> float:
    trig = rt.get("trigger")
    events = {"Hit": m["hit"], "IncomingHit": m["taken"], "Kill": m["kill"], "LowHealth": m["lowhp"]}.get(trig, 0)
    p = (rt.get("procChancePercent") or 100.0) / 100.0
    if rt.get("luckyHitChancePercent"):
        p *= rt["luckyHitChancePercent"] / 100.0 * (rt.get("luckyHitProcCoefficient") or 1.0)
    rate = events * p  # per minute
    if rate <= 0:
        return 0.0
    icd = (rt.get("icdSeconds") or 0.0) / 60.0
    return 1.0 / (icd + 1.0 / rate)


def spells_of(x: dict) -> list[dict]:
    rec = x.get("records") or {}
    return ([rec["spell"]] if "spell" in rec else []) + list(rec.get("spells", []))


def effects_of(x: dict) -> dict[str, dict]:
    rec = x.get("records") or {}
    mg = ([rec["magicEffect"]] if "magicEffect" in rec else []) + list(rec.get("magicEffects", []))
    return {e["editorId"]: e for e in mg}


SPELLS: dict[str, dict] = {}
EFFECTS: dict[str, dict] = {}


def index_records(affixes: list[dict]) -> None:
    # Prefixes reuse spells defined on other affixes (traps, chaos curses), so
    # resolve against every record in the file, not just the affix's own.
    for x in affixes:
        SPELLS.update({s["editorId"]: s for s in spells_of(x)})
        EFFECTS.update(effects_of(x))


def classify(x: dict, m: dict) -> dict:
    rt = x.get("runtime", {})
    ac = rt.get("action", {})
    procs = procs_per_min(rt, m)
    effs = EFFECTS
    spells = SPELLS
    main = spells.get(ac.get("spellEditorId", ""))
    passive = spells.get(ac.get("passiveSpellEditorId", ""))
    out = dict(id=x["id"], name=x.get("nameKo", ""), trig=rt.get("trigger"), procs=procs,
               icd=rt.get("icdSeconds") or 0, passive=passive is not None, role="special", value=0.0)
    if ac.get("spellForm"):
        out["role"] = "summon"
        return out
    if ac.get("type") == "CastSpellAdaptiveElement" and main is None:
        main = spells.get((ac.get("spells") or {}).get("Fire", "")) if isinstance(ac.get("spells"), dict) else None
    if ac.get("type") not in ("CastSpell", "CastSpellAdaptiveElement") or main is None:
        out["role"] = {"DebugNotify": "passive-only"}.get(ac.get("type"), f"special:{ac.get('type')}")
        return out
    eff = main.get("effect") or main["effects"][0]
    mgef = effs.get(eff["magicEffectEditorId"], {})
    hostile = bool(mgef.get("hostile"))
    av = mgef.get("actorValue")
    mag = float(eff.get("magnitude") or 0)
    dur = float(eff.get("duration") or 0)
    sc = ac.get("magnitudeScaling") or {}
    if sc:
        src = sc.get("source", "")
        scaled = sc.get("mult", 0) * m["phys"] + sc.get("add", 0)
        if src.startswith("Hit"):
            mag = max(mag, scaled) if sc.get("spellBaseAsMin") else scaled
    if "magnitudeOverride" in ac:
        mag = float(ac["magnitudeOverride"])
    if av == "Health":
        per = mag * max(dur, 1.0) if dur else mag  # DoT: magnitude per second
        out["role"] = "damage" if hostile else "sustain"
        out["value"] = procs * per
    else:
        out["role"] = "control" if hostile else "buff"
        out["value"] = min(1.0, procs * dur / 60.0) if dur else 0.0
    return out


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--repo", default=".")
    ap.add_argument("--top", type=int, default=8)
    ap.add_argument("--trials", type=int, default=200)
    args = ap.parse_args()
    affixes = json.loads((Path(args.repo) / "affixes" / "affixes.json").read_text(encoding="utf-8"))["keywords"]["affixes"]
    rws = [x for x in affixes if x["id"].startswith("runeword_")]
    pfs = [x for x in affixes if x.get("slot") == "prefix" and not x["id"].startswith(("runeword_", "internal_"))
           and (x.get("runtime", {}).get("lootWeight") or 0) > 0]
    index_records(affixes)
    print(f"prefixes (lootable) {len(pfs)}, runewords {len(rws)}")
    for name, m in MODELS.items():
        print(f"\n=== model {name}: {m}")
        rows = {"PF": [classify(x, m) for x in pfs], "RW": [classify(x, m) for x in rws]}
        for kind, rs in rows.items():
            c = Counter(r["role"] for r in rs)
            print(f"  {kind} roles: {dict(c)}  passive bundle: {sum(r['passive'] for r in rs)}")
        for role, unit in (("damage", "dmg/min"), ("sustain", "heal/min"), ("buff", "uptime"), ("control", "uptime")):
            print(f"  -- {role} ({unit})")
            for kind in ("PF", "RW"):
                vals = sorted((r for r in rows[kind] if r["role"] == role), key=lambda r: -r["value"])
                if not vals:
                    print(f"     {kind}: none")
                    continue
                v = [r["value"] for r in vals]
                print(f"     {kind}: n={len(v)} median={st.median(v):.2f} p90={v[max(0, len(v) // 10 - 1)]:.2f} max={v[0]:.2f}")
                for r in vals[: args.top if name == "melee-normal" else 3]:
                    tag = " +passive" if r["passive"] else ""
                    print(f"        {r['value']:8.2f}  {r['procs']:5.1f}/min  icd {r['icd']:>4}  {r['id']}{tag}  {r['name'][:46]}")
    acquisition_cost(Path(args.repo), pfs, args.trials)


def acquisition_cost(repo: Path, pfs: list[dict], trials: int) -> None:
    """How hard is each side to get? Fragments are a targeted path, prefixes a gamble."""
    contract = json.loads((repo / "affixes" / "runeword.contract.json").read_text(encoding="utf-8"))
    weights = {r["rune"]: r["weight"] for r in contract["runewordRuneWeights"]}
    runes, wts = list(weights), list(weights.values())
    rng = random.Random(1)

    def drops(seq: list[str]) -> float:
        need = Counter(seq)
        total = 0
        for _ in range(trials):
            have: Counter = Counter()
            while any(have[r] < k for r, k in need.items()):
                total += 1
                have[rng.choices(runes, wts)[0]] += 1
        return total / trials

    rows = sorted((drops(rw["runes"]), rw["name"]) for rw in contract["runewordCatalog"])
    v = [r[0] for r in rows]
    print("\n=== acquisition")
    print(f"  fragment drops to finish one runeword from zero: min {v[0]:.0f} ({rows[0][1]}), "
          f"median {st.median(v):.0f}, p90 {v[int(len(v) * 0.9)]:.0f}, max {v[-1]:.0f} ({rows[-1][1]})")
    pw = [x["runtime"]["lootWeight"] for x in pfs]
    p = sorted(w / sum(pw) for w in pw)
    med = st.median(p)
    print(f"  one specific prefix per roll: median {med:.4f} -> {1 / med:.0f} rolls expected; "
          f"within 6 selected reforges {1 - (1 - med) ** 6:.0%}")


if __name__ == "__main__":
    main()
