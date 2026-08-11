#!/usr/bin/env python3
"""Resolve deterministic metadata for generated public documentation."""

from __future__ import annotations

import re
from dataclasses import dataclass
from datetime import date
from pathlib import Path

from verify_version_consistency import CMAKE_FILE, CHANGELOG_FILE, read_cmake_version


@dataclass(frozen=True)
class PublicDocMetadata:
    version: str
    release_date: str


def read_changelog_release_date(version: str, path: Path = CHANGELOG_FILE) -> str | None:
    pattern = re.compile(
        rf"^##\s*\[{re.escape(version)}\]\s*-\s*(\d{{4}}-\d{{2}}-\d{{2}})\s*$",
        re.MULTILINE,
    )
    match = pattern.search(path.read_text(encoding="utf-8"))
    if not match:
        return None

    value = match.group(1)
    try:
        date.fromisoformat(value)
    except ValueError:
        return None
    return value


def load_public_doc_metadata(
    cmake_path: Path = CMAKE_FILE,
    changelog_path: Path = CHANGELOG_FILE,
) -> PublicDocMetadata:
    version = read_cmake_version(cmake_path)
    if not version:
        raise ValueError(f"Could not read the authoritative version from {cmake_path}")

    release_date = read_changelog_release_date(version, changelog_path)
    if not release_date:
        raise ValueError(
            f"CHANGELOG.md must contain an explicit dated heading for version {version}: "
            f"## [{version}] - YYYY-MM-DD"
        )

    return PublicDocMetadata(version=version, release_date=release_date)
