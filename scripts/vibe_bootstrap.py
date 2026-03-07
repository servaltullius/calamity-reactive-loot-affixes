#!/usr/bin/env python3
from __future__ import annotations

import shutil
from pathlib import Path


REQUIRED_BRAIN_FILES = (
    "configure.py",
    "context_db.py",
    "doctor.py",
    "indexer.py",
    "precommit.py",
)


def repo_root() -> Path:
    return Path(__file__).resolve().parents[1]


def vibe_dir(root: Path | None = None) -> Path:
    resolved_root = root or repo_root()
    return resolved_root / ".vibe"


def bundled_brain_dir(root: Path | None = None) -> Path:
    resolved_root = root or repo_root()
    return resolved_root / "scripts" / "vibe_kit" / "brain"


def bundled_readme_path(root: Path | None = None) -> Path:
    resolved_root = root or repo_root()
    return resolved_root / "scripts" / "vibe_kit" / "templates" / "README.md"


def installed_brain_dir(root: Path | None = None) -> Path:
    return vibe_dir(root) / "brain"


def has_complete_brain(path: Path) -> bool:
    return path.is_dir() and all((path / name).is_file() for name in REQUIRED_BRAIN_FILES)


def install_bundled_brain(root: Path | None = None, *, force: bool = False) -> bool:
    resolved_root = root or repo_root()
    source = bundled_brain_dir(resolved_root)
    destination = installed_brain_dir(resolved_root)

    if not has_complete_brain(source):
        return False

    if force and destination.exists():
        shutil.rmtree(destination)

    destination.mkdir(parents=True, exist_ok=True)
    for source_path in sorted(source.iterdir()):
        if source_path.is_dir():
            shutil.copytree(source_path, destination / source_path.name, dirs_exist_ok=True)
            continue
        shutil.copy2(source_path, destination / source_path.name)

    readme_source = bundled_readme_path(resolved_root)
    readme_destination = vibe_dir(resolved_root) / "README.md"
    if readme_source.is_file() and (force or not readme_destination.exists()):
        readme_destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(readme_source, readme_destination)

    return True

