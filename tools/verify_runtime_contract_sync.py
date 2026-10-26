#!/usr/bin/env python3
from __future__ import annotations

import json
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Callable

from repo_paths import repo_run_root


def _repo_root() -> Path:
    return Path(__file__).resolve().parents[1]


def _normalize_json(path: Path) -> object:
    return json.loads(path.read_text(encoding="utf-8"))


def _normalize_text(path: Path) -> str:
    # A Windows dotnet writes CRLF; the checked-in files are LF.
    return path.read_text(encoding="utf-8").replace("\r\n", "\n")


def _read_bytes(path: Path) -> bytes:
    return path.read_bytes()


# Every file GeneratorRunner writes into --data, compared against the checked-in
# Data/ copy. runtime_contract.json stays first so its stale message is unchanged.
# The ESP is compared byte for byte: Mutagen output is a pure function of the spec
# and the generator code, so any difference means Data/ was not regenerated
# (generator/Mutagen change, or a partial commit that left the ESP unstaged).
_GENERATED_OUTPUTS: tuple[tuple[str, Callable[[Path], object]], ...] = (
    ("SKSE/Plugins/CalamityAffixes/runtime_contract.json", _normalize_json),
    ("SKSE/Plugins/CalamityAffixes/affixes.json", _normalize_json),
    ("SKSE/Plugins/InventoryInjector/CalamityAffixes.json", _normalize_json),
    ("CalamityAffixes_KID.ini", _normalize_text),
    ("CalamityAffixes_DISTR.ini", _normalize_text),
    ("CalamityAffixes.esp", _read_bytes),
)
_CONTRACT_RELATIVE = _GENERATED_OUTPUTS[0][0]


def main() -> int:
    repo_root = _repo_root()
    dotnet_cmd = os.environ.get("CAFF_DOTNET", "dotnet")
    expected_contract = repo_root / "Data" / "SKSE" / "Plugins" / "CalamityAffixes" / "runtime_contract.json"
    spec_path = repo_root / "affixes" / "affixes.json"

    if not expected_contract.is_file():
        print(f"runtime_contract sync: missing checked-in contract: {expected_contract}", file=sys.stderr)
        return 1
    if not spec_path.is_file():
        print(f"runtime_contract sync: missing spec: {spec_path}", file=sys.stderr)
        return 1

    with repo_run_root(repo_root) as run_root:
        temp_parent = run_root / ".tmp"
        temp_parent.mkdir(parents=True, exist_ok=True)

        with tempfile.TemporaryDirectory(prefix="caff-runtime-contract-", dir=temp_parent) as temp_dir:
            temp_root = Path(temp_dir)
            generated_data_dir = temp_root / "Data"
            generated_data_arg = generated_data_dir.relative_to(run_root)
            cmd = [
                dotnet_cmd,
                "run",
                "--project",
                "tools/CalamityAffixes.Generator",
                "--",
                "--spec",
                "affixes/affixes.json",
                "--data",
                generated_data_arg.as_posix(),
            ]
            result = subprocess.run(
                cmd,
                cwd=run_root,
                text=True,
                capture_output=True,
                check=False,
            )
            if result.returncode != 0:
                sys.stderr.write(result.stdout)
                sys.stderr.write(result.stderr)
                return result.returncode

            generated_contract = generated_data_dir / _CONTRACT_RELATIVE
            if not generated_contract.is_file():
                print(f"runtime_contract sync: generator did not produce contract: {generated_contract}", file=sys.stderr)
                return 1

            stale: list[str] = []
            for relative, normalize in _GENERATED_OUTPUTS:
                checked_in = repo_root / "Data" / relative
                generated = generated_data_dir / relative
                if not generated.is_file():
                    print(f"generated outputs sync: generator did not produce {relative}", file=sys.stderr)
                    return 1
                if not checked_in.is_file():
                    stale.append(relative)
                    continue
                if normalize(checked_in) != normalize(generated):
                    stale.append(relative)

            if _CONTRACT_RELATIVE in stale:
                print(
                    "runtime_contract sync: checked-in contract is stale. "
                    "Regenerate Data/SKSE/Plugins/CalamityAffixes/runtime_contract.json via generator.",
                    file=sys.stderr,
                )
            if stale:
                for relative in stale:
                    print(f"generated outputs sync: checked-in Data/{relative} is stale or missing", file=sys.stderr)
                print(
                    "generated outputs sync: run the generator "
                    "(dotnet run --project tools/CalamityAffixes.Generator -- --spec affixes/affixes.json --data Data) "
                    "and commit every file it writes.",
                    file=sys.stderr,
                )
                return 1

    print("runtime_contract sync: OK")
    print(f"generated outputs sync: OK ({len(_GENERATED_OUTPUTS)} files)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
