#!/usr/bin/env python3
"""Let packaging reuse the committed .pex only while they match their sources.

Papyrus compilation needs PapyrusCompiler.exe, which ships with the Creation Kit
and cannot be installed on a GitHub runner. The compiled outputs are therefore
committed under Data/Scripts/ so a release can be packaged without a compiler.

That is only safe if the committed .pex are provably current. compile_papyrus.sh
states the rule this file exists to preserve:

    Existing staged PEX files are never accepted as a substitute for a failed
    or incomplete compiler invocation.

A blind fallback to the committed .pex would break it: edit a .psc, forget to
recompile, and the release ships a binary that no longer matches its source with
nothing to say so. This pin records the hash of every in-repo input that affects
compilation, plus the hash of each output. Packaging may reuse the committed
.pex only while all of them still match.

Inputs that affect the output:

  - the three compiled sources under Data/Scripts/Source/
  - every stub and the flags file under tools/papyrus-stubs/, which are on the
    compiler's import path, so editing one changes the output

Why --update recompiles instead of just re-hashing
--------------------------------------------------
PEX output is not reproducible. Compiling the same source twice produces files
of identical size holding an identical set of strings in a *different order*
(the compiler emits its string table in hash-map order), and the whole body
shifts because every operand references strings by index. The header also
embeds a compile timestamp. Two compiles of a byte-identical source differ in
roughly half their bytes.

So this pin cannot be self-verifying: there is no way to recompile and compare
against the recorded output hash. The recorded hash pins *an artifact*, not a
derivation. Which leaves one gap -- a pin refreshed without recompiling would
happily bless a stale .pex -- and --update closes it by running
compile_papyrus.sh itself. The outputs it hashes are then fresh by
construction, not by trust.

Known gap: the compiler also imports vanilla sources from the Creation Kit's
Scripts.zip, which is not in this repository and cannot be hashed here. A
Creation Kit update could change the output without changing anything this pin
can see. Recompiling resolves it; the recorded output hashes make it visible.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
PIN_FILE = REPO_ROOT / "tools" / "papyrus.pin.json"
COMPILE_SCRIPT = REPO_ROOT / "tools" / "compile_papyrus.sh"

# Mirrors the `targets` / `expected_outputs` pair in tools/compile_papyrus.sh.
TARGETS = [
    (
        "Data/Scripts/Source/CalamityAffixes_ModeControl.psc",
        "Data/Scripts/CalamityAffixes_ModeControl.pex",
    ),
    (
        "Data/Scripts/Source/CalamityAffixes_ModEventEmitter.psc",
        "Data/Scripts/CalamityAffixes_ModEventEmitter.pex",
    ),
    (
        "Data/Scripts/Source/CalamityAffixes_MCMConfig.psc",
        "Data/Scripts/CalamityAffixes_MCMConfig.pex",
    ),
]
STUB_DIR = "tools/papyrus-stubs"
PEX_DIR = "Data/Scripts"

REMEDY = (
    "Recompile where PapyrusCompiler.exe is available and refresh the pin:\n"
    "  python3 tools/verify_papyrus_pin.py --update\n"
    "then commit the regenerated .pex together with the updated pin."
)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1 << 16), b""):
            digest.update(chunk)
    return digest.hexdigest()


def shared_input_paths(root: Path = REPO_ROOT) -> list[str]:
    """Everything on the compiler's import path that lives in this repo."""
    return sorted(
        f"{STUB_DIR}/{p.name}"
        for p in (root / STUB_DIR).iterdir()
        if p.is_file() and p.suffix in {".psc", ".flg"}
    )


def build_pin(root: Path = REPO_ROOT) -> dict:
    return {
        "description": (
            "Hashes proving Data/Scripts/*.pex were compiled from the committed "
            "sources. Regenerate with tools/verify_papyrus_pin.py --update, which "
            "recompiles first. Do not hand-edit."
        ),
        "targets": [
            {
                "source": src,
                "output": out,
                "source_sha256": sha256(root / src),
                "output_sha256": sha256(root / out),
            }
            for src, out in TARGETS
        ],
        "shared_inputs": [
            {"path": rel, "sha256": sha256(root / rel)}
            for rel in shared_input_paths(root)
        ],
    }


def collect_mismatches(pin: dict, root: Path = REPO_ROOT) -> list[str]:
    """Return one human-readable line per problem; empty means the pin holds."""
    problems: list[str] = []

    for entry in pin.get("targets", []):
        for kind in ("source", "output"):
            rel = entry[kind]
            path = root / rel
            if not path.is_file():
                problems.append(f"missing {kind}: {rel}")
                continue
            actual = sha256(path)
            if actual != entry[f"{kind}_sha256"]:
                problems.append(
                    f"{kind} changed since the pin was recorded: {rel}\n"
                    f"      recorded {entry[f'{kind}_sha256']}\n"
                    f"      actual   {actual}"
                )

    recorded_shared = {e["path"]: e["sha256"] for e in pin.get("shared_inputs", [])}
    for rel, expected in recorded_shared.items():
        path = root / rel
        if not path.is_file():
            problems.append(f"missing import-path input: {rel}")
        elif sha256(path) != expected:
            problems.append(f"import-path input changed since the pin was recorded: {rel}")

    # A stub added after the pin was written is also on the import path and can
    # change the output, so an unrecorded one is a mismatch, not a nicety.
    for rel in shared_input_paths(root):
        if rel not in recorded_shared:
            problems.append(f"import-path input not recorded in the pin: {rel}")

    # compile_papyrus.sh deletes legacy outputs before writing the current ones.
    # Packaging copies Data/ wholesale, so a leftover .pex would still be shipped
    # -- and on the CI path no compiler runs to clear it. Requiring the set to be
    # exactly the pinned outputs is what lets the fallback skip that cleanup.
    pinned_outputs = {Path(e["output"]).name for e in pin.get("targets", [])}
    present = {p.name for p in (root / PEX_DIR).glob("*.pex")}
    for extra in sorted(present - pinned_outputs):
        problems.append(f"unpinned .pex present in {PEX_DIR}/: {extra}")

    return problems


def update(root: Path = REPO_ROOT) -> int:
    # Recompile first. Hashing whatever happens to be on disk would let a pin
    # refreshed without a compile declare a stale .pex current -- the exact
    # failure this pin exists to prevent.
    print("Recompiling Papyrus scripts before recording the pin...")
    result = subprocess.run(["bash", str(COMPILE_SCRIPT)], cwd=root)
    if result.returncode != 0:
        print(
            "\nERROR: Papyrus compile failed; the pin was NOT updated.\n"
            "A pin recorded over a failed compile would certify a stale .pex.",
            file=sys.stderr,
        )
        return 1

    pin = build_pin(root)
    PIN_FILE.write_text(json.dumps(pin, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(f"papyrus pin updated: {PIN_FILE.relative_to(root)}")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("--quiet", action="store_true", help="print nothing on success")
    parser.add_argument(
        "--update",
        action="store_true",
        help="recompile the Papyrus scripts and record the resulting hashes",
    )
    args = parser.parse_args()

    if args.update:
        return update()

    if not PIN_FILE.is_file():
        print(f"ERROR: pin file not found: {PIN_FILE}", file=sys.stderr)
        return 1

    pin = json.loads(PIN_FILE.read_text(encoding="utf-8"))
    problems = collect_mismatches(pin)
    if problems:
        print("ERROR: the committed .pex no longer match their recorded sources:", file=sys.stderr)
        for problem in problems:
            print(f"  - {problem}", file=sys.stderr)
        print(f"\n{REMEDY}", file=sys.stderr)
        return 1

    if not args.quiet:
        targets = len(pin.get("targets", []))
        shared = len(pin.get("shared_inputs", []))
        print(f"papyrus pin OK: {targets} compiled scripts, {shared} import-path inputs")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
