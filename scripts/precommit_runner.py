#!/usr/bin/env python3
from __future__ import annotations

import subprocess
import runpy
import sys
from pathlib import Path

try:
    from vibe_bootstrap import install_bundled_brain
except ModuleNotFoundError:
    from scripts.vibe_bootstrap import install_bundled_brain


def _repo_root() -> Path:
    return Path(__file__).resolve().parents[1]


def main(argv: list[str]) -> int:
    root = _repo_root()
    config = root / ".vibe" / "config.json"
    if not config.exists():
        setup = root / "scripts" / "setup_vibe_env.py"
        subprocess.check_call([sys.executable, str(setup)], cwd=str(root))
    else:
        install_bundled_brain(root)
    brain = root / ".vibe" / "brain"
    legacy_runner = brain / "precommit.py"

    if not legacy_runner.exists():
        print(
            "[pre-commit] vendored brain 또는 `.vibe/brain/precommit.py` 를 찾지 못해 vibe-kit 검사를 건너뜁니다.",
            file=sys.stderr,
        )
        print(
            "[pre-commit] 훅은 정상 동작하며 커밋은 계속 진행됩니다. "
            "vibe-kit 소스 번들이 없는 환경이면 `scripts/vibe_kit/brain` 공급 상태를 확인해야 합니다.",
            file=sys.stderr,
        )
        return 0

    sys.path.insert(0, str(brain))
    old_argv = sys.argv[:]
    try:
        sys.argv = [str(legacy_runner), *argv]
        runpy.run_path(str(legacy_runner), run_name="__main__")
    finally:
        sys.argv = old_argv
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
