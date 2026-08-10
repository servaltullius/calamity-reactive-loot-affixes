#!/usr/bin/env python3
from __future__ import annotations

import argparse
import getpass
import os
import re
import sys
from collections import deque
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class PatternGroup:
    name: str
    desc: str
    any_of: list[re.Pattern[str]]


def _is_wsl() -> bool:
    try:
        return "microsoft" in Path("/proc/version").read_text(encoding="utf-8", errors="ignore").lower()
    except Exception:
        return False


def _candidate_log_paths() -> list[Path]:
    # Explicit env var wins.
    env = (
        os.environ.get("CALAMITYAFFIXES_LOG")
        or os.environ.get("SKSE_CALAMITY_LOG")
        or os.environ.get("SKSE_LOG_PATH")
        or ""
    ).strip()
    out: list[Path] = []
    if env:
        out.append(Path(env))

    # Native Linux path (rare, but cheap to check).
    out.append(Path.home() / "Documents" / "My Games" / "Skyrim Special Edition" / "SKSE" / "CalamityAffixes.log")

    # WSL: default Windows documents path.
    if Path("/mnt/c/Users").is_dir():
        user = getpass.getuser()
        # Common cases: WSL username == Windows username, or repo owner uses kdw73.
        for win_user in [user, "kdw73"]:
            out.append(
                Path("/mnt/c/Users")
                / win_user
                / "Documents"
                / "My Games"
                / "Skyrim Special Edition"
                / "SKSE"
                / "CalamityAffixes.log"
            )

    # De-dup while preserving order.
    seen: set[str] = set()
    uniq: list[Path] = []
    for p in out:
        ps = str(p)
        if ps in seen:
            continue
        seen.add(ps)
        uniq.append(p)
    return uniq


def _pick_log_path(explicit: str | None) -> Path | None:
    if explicit:
        return Path(explicit)
    for p in _candidate_log_paths():
        if p.is_file():
            return p
    return None


def _tail_lines(path: Path, max_lines: int) -> list[str]:
    # Keep only the tail to avoid giant logs slowing the check.
    if max_lines <= 0:
        max_lines = 5000
    dq: deque[str] = deque(maxlen=max_lines)
    with path.open("r", encoding="utf-8", errors="ignore") as f:
        for line in f:
            dq.append(line.rstrip("\n"))
    return list(dq)


def _find_matches(lines: list[str], pat: re.Pattern[str], limit: int = 5) -> list[str]:
    hits: list[str] = []
    for ln in lines:
        if pat.search(ln):
            hits.append(ln)
            if len(hits) >= limit:
                break
    return hits


def main(argv: list[str]) -> int:
    ap = argparse.ArgumentParser(
        description="In-game smoke helper: validate CalamityAffixes SKSE log has expected startup signatures.",
    )
    ap.add_argument(
        "--log",
        help="Path to CalamityAffixes.log. If omitted, tries common locations (env CALAMITYAFFIXES_LOG, WSL Windows Documents).",
    )
    ap.add_argument("--tail-lines", type=int, default=5000, help="Scan only the last N lines (default: 5000).")
    ap.add_argument("--strict", action="store_true", help="Treat warnings as failures (exit 1).")
    args = ap.parse_args(argv)

    log_path = _pick_log_path(args.log)
    if log_path is None:
        print("[logcheck] FAIL: log file not found.")
        print("[logcheck] Provide `--log <path>` or set env `CALAMITYAFFIXES_LOG`.")
        if _is_wsl():
            print(
                "[logcheck] WSL hint: typical path is "
                "`/mnt/c/Users/<You>/Documents/My Games/Skyrim Special Edition/SKSE/CalamityAffixes.log`."
            )
        return 1

    if not log_path.is_file():
        print(f"[logcheck] FAIL: not a file: {log_path}")
        return 1

    lines = _tail_lines(log_path, int(args.tail_lines))
    scanned = len(lines)
    blob = "\n".join(lines)

    required = [
        PatternGroup(
            name="plugin_loaded",
            desc="Plugin loaded and waiting for DataLoaded",
            any_of=[re.compile(r"CalamityAffixes: plugin loaded, waiting for kDataLoaded\.", re.IGNORECASE)],
        ),
        PatternGroup(
            name="runtime_config_loaded",
            desc="Runtime config loaded (affixes count etc.)",
            any_of=[re.compile(r"CalamityAffixes: runtime config loaded \(affixes=", re.IGNORECASE)],
        ),
        PatternGroup(
            name="hit_pipeline",
            desc="Hit pipeline active (HandleHealthDamage hook or TESHit fallback)",
            any_of=[
                re.compile(r"installed HandleHealthDamage vfunc hooks", re.IGNORECASE),
                re.compile(r"HandleHealthDamage callback is active", re.IGNORECASE),
                re.compile(r"TESHitEvent seen", re.IGNORECASE),
            ],
        ),
        PatternGroup(
            name="prisma_overlay",
            desc="Prisma overlay enabled and PrismaUI API acquired",
            any_of=[
                re.compile(r"Prisma tooltip/control overlay enabled", re.IGNORECASE),
                re.compile(r"PrismaUI API acquired\.", re.IGNORECASE),
            ],
        ),
        PatternGroup(
            name="trap_system",
            desc="TrapSystem enabled",
            any_of=[re.compile(r"TrapSystem enabled", re.IGNORECASE)],
        ),
    ]

    # Fail-fast conditions (specific + generic).
    error_pats: list[tuple[str, re.Pattern[str]]] = [
        ("runtime_config_missing", re.compile(r"CalamityAffixes: runtime config not found:", re.IGNORECASE)),
        ("runtime_config_parse_fail", re.compile(r"CalamityAffixes: failed to parse runtime config", re.IGNORECASE)),
        ("no_affixes_loaded", re.compile(r"CalamityAffixes: no affixes loaded\.", re.IGNORECASE)),
        ("prismaui_missing", re.compile(r"CalamityAffixes: PrismaUI API not available", re.IGNORECASE)),
        ("dotapply_auto_disabled", re.compile(r"DotApply safety auto-disabled", re.IGNORECASE)),
        ("generic_error", re.compile(r"\[error\]\s+CalamityAffixes:", re.IGNORECASE)),
    ]
    warn_pats: list[tuple[str, re.Pattern[str]]] = [
        ("generic_warn", re.compile(r"\[warn\]\s+CalamityAffixes:", re.IGNORECASE)),
        ("dotapply_safety_warning", re.compile(r"DotApply safety warning", re.IGNORECASE)),
    ]

    # Observational groups: engine-path telemetry emitted by debug logging.
    # These confirm that the proc/feedback pipeline RAN — they must never gate
    # the result: a session without combat legitimately produces none of them,
    # and none of them prove that a visual rendered on screen or that a sound
    # was audible. Eyes-on gameplay is the only visual/audio verdict
    # (see docs/QA_SMOKE_CHECKLIST.md).
    observational = [
        PatternGroup(
            name="proc_dispatch",
            desc="Affix proc rolled and dispatched",
            any_of=[re.compile(r"CalamityAffixes: proc \(affixId=", re.IGNORECASE)],
        ),
        PatternGroup(
            name="cast_spell_immediate",
            desc="CastSpell lane executed",
            any_of=[re.compile(r"CalamityAffixes: CastSpellImmediate \(affix=", re.IGNORECASE)],
        ),
        PatternGroup(
            name="magic_effect_apply",
            desc="Engine apply event observed for a CalamityAffixes magic effect",
            any_of=[re.compile(r"CalamityAffixes: magic effect apply observed \(mgef=", re.IGNORECASE)],
        ),
        PatternGroup(
            name="feedback_art_accepted",
            desc="Feedback art instantiation accepted by the engine",
            any_of=[re.compile(r"CalamityAffixes: action feedback art \(.*instantiated=true", re.IGNORECASE)],
        ),
        PatternGroup(
            name="feedback_sound_accepted",
            desc="Feedback sound handle accepted by the engine",
            any_of=[re.compile(r"CalamityAffixes: action feedback sound \(.*played=true", re.IGNORECASE)],
        ),
        PatternGroup(
            name="trap_marker_spawned",
            desc="Trap ground marker accepted by the temp-effect path",
            any_of=[re.compile(r"CalamityAffixes: trap marker spawn \(.*spawned=true", re.IGNORECASE)],
        ),
        PatternGroup(
            name="trap_marker_probe",
            desc="Debug probe spawn at player feet accepted by the temp-effect path",
            any_of=[re.compile(r"CalamityAffixes: trap marker probe \(.*spawned=true", re.IGNORECASE)],
        ),
    ]
    # Engine-path refusals worth surfacing. Still observational — they do not
    # change the exit code — but a nonzero count is a real engine-side rejection
    # that deserves eyes during the in-game session.
    observational_reject_pats: list[tuple[str, re.Pattern[str]]] = [
        ("feedback_art_rejected", re.compile(r"action feedback art \(.*instantiated=false", re.IGNORECASE)),
        ("feedback_art_skipped_no3d", re.compile(r"action feedback art skipped \(", re.IGNORECASE)),
        ("feedback_sound_rejected", re.compile(r"action feedback sound \(.*(?:built=false|played=false)", re.IGNORECASE)),
        ("trap_marker_rejected", re.compile(r"trap (?:marker|cue) spawn \(.*spawned=false", re.IGNORECASE)),
        ("trap_marker_probe_rejected", re.compile(r"trap marker probe \(.*spawned=false", re.IGNORECASE)),
    ]

    missing: list[str] = []
    ok: list[str] = []
    for g in required:
        hit = any(p.search(blob) for p in g.any_of)
        (ok if hit else missing).append(g.name)

    errors: list[tuple[str, list[str]]] = []
    for name, pat in error_pats:
        hits = _find_matches(lines, pat, limit=5)
        if hits:
            errors.append((name, hits))

    warns: list[tuple[str, list[str]]] = []
    for name, pat in warn_pats:
        hits = _find_matches(lines, pat, limit=5)
        if hits:
            warns.append((name, hits))

    print("[logcheck] CalamityAffixes SKSE log smoke check")
    print(f"- file: {log_path}")
    print(f"- scanned_tail_lines: {scanned}")
    print(f"- required_ok: {len(ok)} / {len(required)}")
    if missing:
        print(f"- missing: {', '.join(missing)}")

    if errors:
        print(f"- errors: {len(errors)}")
        for name, samples in errors:
            print(f"  - {name}: {len(samples)} sample(s)")
            for s in samples:
                print(f"    - {s}")
    else:
        print("- errors: 0")

    if warns:
        print(f"- warnings: {len(warns)}")
        for name, samples in warns:
            print(f"  - {name}: {len(samples)} sample(s)")
            for s in samples:
                print(f"    - {s}")
    else:
        print("- warnings: 0")

    observed: list[tuple[str, int]] = []
    not_observed: list[str] = []
    for g in observational:
        count = sum(1 for line in lines if any(p.search(line) for p in g.any_of))
        if count:
            observed.append((g.name, count))
        else:
            not_observed.append(g.name)

    print("- feedback pipeline observation (engine-path only; NOT an on-screen visual or audible-audio verdict):")
    for name, count in observed:
        print(f"  - {name}: OBSERVED ({count})")
    for name in not_observed:
        print(f"  - {name}: NOT OBSERVED")
    if not observed:
        print("  - note: no proc activity in scanned tail (no combat, or debugVerbose disabled)")

    reject_hits = []
    for name, pat in observational_reject_pats:
        hits = _find_matches(lines, pat, limit=3)
        if hits:
            reject_hits.append((name, hits))
    if reject_hits:
        print("- feedback pipeline engine-path rejections (observational, non-fatal):")
        for name, samples in reject_hits:
            print(f"  - {name}: {len(samples)} sample(s)")
            for s in samples:
                print(f"    - {s}")

    if errors or missing:
        print("[logcheck] RESULT: FAIL")
        return 1
    if warns and args.strict:
        print("[logcheck] RESULT: FAIL (strict warnings)")
        return 1
    if warns:
        print("[logcheck] RESULT: WARN")
        return 0

    print("[logcheck] RESULT: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
