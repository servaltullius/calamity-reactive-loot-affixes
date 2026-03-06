#!/usr/bin/env python3
from __future__ import annotations

import json
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path


def _repo_root() -> Path:
    return Path(__file__).resolve().parents[1]


def _repo_run_root(repo_root: Path) -> Path:
    safe_link = repo_root.parent / "calamity"
    if " " in str(repo_root) and safe_link.is_dir() and safe_link.resolve() == repo_root.resolve():
        return safe_link
    return repo_root


def _normalize_json(path: Path) -> object:
    return json.loads(path.read_text(encoding="utf-8"))


def main() -> int:
    repo_root = _repo_root()
    repo_run_root = _repo_run_root(repo_root)
    expected_contract = repo_root / "Data" / "SKSE" / "Plugins" / "CalamityAffixes" / "runtime_contract.json"
    spec_path = repo_root / "affixes" / "affixes.json"

    if not expected_contract.is_file():
        print(f"runtime_contract sync: missing checked-in contract: {expected_contract}", file=sys.stderr)
        return 1
    if not spec_path.is_file():
        print(f"runtime_contract sync: missing spec: {spec_path}", file=sys.stderr)
        return 1

    temp_parent = repo_run_root / ".tmp"
    temp_parent.mkdir(parents=True, exist_ok=True)

    with tempfile.TemporaryDirectory(prefix="caff-runtime-contract-", dir=temp_parent) as temp_dir:
        temp_root = Path(temp_dir)
        generated_data_dir = temp_root / "Data"
        cmd = [
            "dotnet",
            "run",
            "--project",
            "tools/CalamityAffixes.Generator",
            "--",
            "--spec",
            "affixes/affixes.json",
            "--data",
            str(generated_data_dir),
        ]
        result = subprocess.run(
            cmd,
            cwd=repo_run_root,
            text=True,
            capture_output=True,
            check=False,
        )
        if result.returncode != 0:
            sys.stderr.write(result.stdout)
            sys.stderr.write(result.stderr)
            return result.returncode

        generated_contract = generated_data_dir / "SKSE" / "Plugins" / "CalamityAffixes" / "runtime_contract.json"
        if not generated_contract.is_file():
            print(f"runtime_contract sync: generator did not produce contract: {generated_contract}", file=sys.stderr)
            return 1

        expected = _normalize_json(expected_contract)
        actual = _normalize_json(generated_contract)
        if expected != actual:
            print(
                "runtime_contract sync: checked-in contract is stale. "
                "Regenerate Data/SKSE/Plugins/CalamityAffixes/runtime_contract.json via generator.",
                file=sys.stderr,
            )
            return 1

    print("runtime_contract sync: OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
