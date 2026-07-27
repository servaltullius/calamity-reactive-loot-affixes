#!/usr/bin/env python3
"""Verify that an existing GitHub release exactly matches this build.

Exit code 3 means that no release exists for the requested tag. Any other
non-zero exit is a verification failure and must not fall through to release
creation.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
import sys
import tempfile
from pathlib import Path
from urllib.parse import quote


NOT_FOUND_EXIT_CODE = 3
EXPECTED_ASSET_COUNT = 4


class VerificationError(RuntimeError):
    pass


def _gh(*args: str) -> str:
    result = subprocess.run(
        ["gh", *args],
        text=True,
        capture_output=True,
        check=False,
    )
    if result.returncode != 0:
        detail = result.stderr.strip() or result.stdout.strip() or "no diagnostic output"
        raise VerificationError(f"gh {' '.join(args)} failed: {detail}")
    return result.stdout


def _gh_json(*args: str) -> object:
    output = _gh(*args)
    try:
        return json.loads(output)
    except json.JSONDecodeError as exc:
        raise VerificationError(f"gh {' '.join(args)} returned invalid JSON: {exc}") from exc


def _require_mapping(value: object, description: str) -> dict[str, object]:
    if not isinstance(value, dict):
        raise VerificationError(f"{description} was not a JSON object")
    return value


def _sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def _asset_files(artifacts_dir: Path) -> list[Path]:
    if not artifacts_dir.is_dir():
        raise VerificationError(f"artifact directory does not exist: {artifacts_dir}")
    assets = sorted(
        (path for path in artifacts_dir.iterdir() if path.is_file()),
        key=lambda path: path.name,
    )
    if len(assets) != EXPECTED_ASSET_COUNT:
        raise VerificationError(
            f"release-artifacts must contain exactly {EXPECTED_ASSET_COUNT} files; "
            f"found {len(assets)}"
        )
    return assets


def _tag_commit(repo: str, tag: str) -> tuple[str, str]:
    encoded_tag = quote(tag, safe="")
    ref = _require_mapping(
        _gh_json("api", f"repos/{repo}/git/ref/tags/{encoded_tag}"),
        f"tag ref {tag}",
    )
    tag_object = _require_mapping(ref.get("object"), f"tag ref {tag} object")
    object_type = tag_object.get("type")
    object_sha = tag_object.get("sha")
    if not isinstance(object_type, str) or not isinstance(object_sha, str):
        raise VerificationError(f"tag ref {tag} has no object type/SHA")

    ref_sha = object_sha
    seen: set[str] = set()
    while object_type == "tag":
        if object_sha in seen:
            raise VerificationError(f"annotated tag cycle detected at {object_sha}")
        seen.add(object_sha)
        annotated = _require_mapping(
            _gh_json("api", f"repos/{repo}/git/tags/{object_sha}"),
            f"annotated tag object {object_sha}",
        )
        target = _require_mapping(annotated.get("object"), f"annotated tag {object_sha} target")
        object_type = target.get("type")
        object_sha = target.get("sha")
        if not isinstance(object_type, str) or not isinstance(object_sha, str):
            raise VerificationError(f"annotated tag {tag} has no target type/SHA")

    if object_type != "commit":
        raise VerificationError(f"tag {tag} resolves to {object_type}, not a commit")
    return ref_sha, object_sha


def verify(args: argparse.Namespace) -> int:
    artifacts_dir = args.artifacts.resolve()
    local_assets = _asset_files(artifacts_dir)
    local_names = [path.name for path in local_assets]

    ref_sha, commit_sha = _tag_commit(args.repo, args.tag)
    if commit_sha.lower() != args.expected_commit.lower():
        raise VerificationError(
            f"tag {args.tag} resolves to commit {commit_sha}, "
            f"but this workflow built {args.expected_commit}"
        )

    releases = _gh_json(
        "release",
        "list",
        "--repo",
        args.repo,
        "--limit",
        "1000",
        "--json",
        "tagName,isDraft,isPrerelease",
    )
    if not isinstance(releases, list):
        raise VerificationError("gh release list did not return a JSON array")
    matches = [
        release
        for release in releases
        if isinstance(release, dict) and release.get("tagName") == args.tag
    ]
    if not matches:
        print(
            f"No existing GitHub release for {args.tag}; verified remote tag "
            f"{ref_sha} -> {commit_sha}."
        )
        return NOT_FOUND_EXIT_CODE
    if len(matches) != 1:
        raise VerificationError(f"found {len(matches)} releases for tag {args.tag}")

    release = _require_mapping(
        _gh_json(
            "release",
            "view",
            args.tag,
            "--repo",
            args.repo,
            "--json",
            "tagName,isDraft,isPrerelease,assets",
        ),
        f"release {args.tag}",
    )
    if release.get("tagName") != args.tag:
        raise VerificationError(
            f"release tag is {release.get('tagName')!r}, expected {args.tag!r}"
        )
    if release.get("isDraft") is not False:
        raise VerificationError(f"release {args.tag} is a draft; expected a published release")
    if release.get("isPrerelease") is not args.expected_prerelease:
        raise VerificationError(
            f"release {args.tag} prerelease={release.get('isPrerelease')!r}; "
            f"expected {args.expected_prerelease}"
        )

    remote_assets = release.get("assets")
    if not isinstance(remote_assets, list):
        raise VerificationError(f"release {args.tag} assets were not a JSON array")
    remote_names = sorted(
        asset.get("name")
        for asset in remote_assets
        if isinstance(asset, dict) and isinstance(asset.get("name"), str)
    )
    if len(remote_assets) != EXPECTED_ASSET_COUNT or remote_names != local_names:
        raise VerificationError(
            "release asset names differ from release-artifacts: "
            f"local={local_names!r}, published={remote_names!r}"
        )

    with tempfile.TemporaryDirectory(prefix="caff-published-release-") as temp_dir:
        published_dir = Path(temp_dir)
        _gh(
            "release",
            "download",
            args.tag,
            "--repo",
            args.repo,
            "--dir",
            str(published_dir),
        )
        published_assets = _asset_files(published_dir)
        published_names = [path.name for path in published_assets]
        if published_names != local_names:
            raise VerificationError(
                "downloaded asset names differ from release-artifacts: "
                f"local={local_names!r}, downloaded={published_names!r}"
            )

        for local_path, published_path in zip(local_assets, published_assets, strict=True):
            local_hash = _sha256(local_path)
            published_hash = _sha256(published_path)
            if local_hash != published_hash:
                raise VerificationError(
                    f"SHA256 mismatch for {local_path.name}: "
                    f"local={local_hash}, published={published_hash}"
                )
            print(f"SHA256 match: {local_path.name} {local_hash}")

    print(
        f"Existing release {args.tag} exactly matches this build "
        f"(tag {ref_sha} -> {commit_sha}, draft=false, "
        f"prerelease={str(args.expected_prerelease).lower()}, 4 assets)."
    )
    return 0


def _parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", required=True, help="GitHub repository as owner/name")
    parser.add_argument("--tag", required=True)
    parser.add_argument("--artifacts", required=True, type=Path)
    parser.add_argument("--expected-commit", required=True)
    parser.add_argument(
        "--expected-prerelease",
        required=True,
        choices=("true", "false"),
    )
    args = parser.parse_args()
    args.expected_prerelease = args.expected_prerelease == "true"
    return args


def main() -> int:
    try:
        return verify(_parse_args())
    except VerificationError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
