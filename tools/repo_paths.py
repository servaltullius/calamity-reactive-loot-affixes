#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import os
import shutil
import tempfile
from contextlib import contextmanager
from pathlib import Path
from typing import Iterator


def _candidate_repo_aliases(repo_root: Path) -> tuple[Path, ...]:
    return (
        repo_root.parent / "calamity",
        repo_root.parent / "projects" / "calamity",
    )


def _persistent_temp_repo_alias(repo_root: Path) -> Path:
    digest = hashlib.sha256(str(repo_root.resolve()).encode("utf-8")).hexdigest()[:12]
    alias_root = Path(tempfile.gettempdir()) / "caff-repo-links" / digest
    alias_path = alias_root / "calamity"
    alias_root.mkdir(parents=True, exist_ok=True)

    if alias_path.is_symlink() and alias_path.resolve() == repo_root.resolve():
        return alias_path
    if alias_path.exists() or alias_path.is_symlink():
        if alias_path.is_dir() and not alias_path.is_symlink():
            shutil.rmtree(alias_path)
        else:
            alias_path.unlink()

    alias_path.symlink_to(repo_root, target_is_directory=True)
    return alias_path


def stable_repo_run_root(repo_root: Path) -> Path:
    override = os.environ.get("CAFF_REPO_RUN_ROOT")
    if override:
        override_path = Path(override).resolve()
        if not override_path.is_dir():
            raise FileNotFoundError(f"CAFF_REPO_RUN_ROOT is not a directory: {override}")
        if override_path != repo_root.resolve():
            raise ValueError(f"CAFF_REPO_RUN_ROOT must resolve to the repo root: {override}")
        return override_path

    if " " not in str(repo_root):
        return repo_root

    for safe_link in _candidate_repo_aliases(repo_root):
        if safe_link.is_dir() and safe_link.resolve() == repo_root.resolve():
            return safe_link

    return _persistent_temp_repo_alias(repo_root)


@contextmanager
def repo_run_root(repo_root: Path) -> Iterator[Path]:
    stable_root = stable_repo_run_root(repo_root)
    if stable_root != repo_root or " " not in str(repo_root):
        yield stable_root
        return

    with tempfile.TemporaryDirectory(prefix="caff-repo-link-") as temp_dir:
        link_path = Path(temp_dir) / "calamity"
        link_path.symlink_to(repo_root, target_is_directory=True)
        yield link_path
