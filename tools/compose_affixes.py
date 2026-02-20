#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any, Dict, Iterable, List


def _read_json(path: Path) -> Any:
    try:
        with path.open("r", encoding="utf-8") as handle:
            return json.load(handle)
    except FileNotFoundError as exc:
        raise RuntimeError(f"File not found: {path}") from exc
    except json.JSONDecodeError as exc:
        raise RuntimeError(f"Invalid JSON ({path}:{exc.lineno}:{exc.colno}): {exc.msg}") from exc


def _write_json(path: Path, payload: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as handle:
        json.dump(payload, handle, ensure_ascii=False, indent=2)
        handle.write("\n")


def _ensure_object(value: Any, *, path: Path, key: str) -> Dict[str, Any]:
    if not isinstance(value, dict):
        raise RuntimeError(f"{path}:{key} must be a JSON object.")
    return value


def _ensure_array(value: Any, *, path: Path, key: str) -> List[Any]:
    if not isinstance(value, list):
        raise RuntimeError(f"{path}:{key} must be a JSON array.")
    return value


def _resolve_relative(base_path: Path, relative_path: str) -> Path:
    candidate = (base_path.parent / relative_path).resolve()
    return candidate


def _read_object_module(manifest_path: Path, relative_path: str, *, label: str) -> Dict[str, Any]:
    path = _resolve_relative(manifest_path, relative_path)
    payload = _read_json(path)
    return _ensure_object(payload, path=path, key=label)


def _read_array_modules(manifest_path: Path, modules: Iterable[str], *, label: str) -> List[Any]:
    merged: List[Any] = []
    for module_relative_path in modules:
        path = _resolve_relative(manifest_path, module_relative_path)
        payload = _read_json(path)
        if isinstance(payload, dict):
            array_payload = payload.get(label)
            if array_payload is None:
                raise RuntimeError(
                    f"{path}: object modules for '{label}' must contain the '{label}' property."
                )
            merged.extend(_ensure_array(array_payload, path=path, key=label))
            continue

        merged.extend(_ensure_array(payload, path=path, key=label))

    return merged


def compose_spec(manifest_path: Path) -> Dict[str, Any]:
    manifest_payload = _read_json(manifest_path)
    manifest = _ensure_object(manifest_payload, path=manifest_path, key="root")

    root_relative = manifest.get("root")
    if not isinstance(root_relative, str) or not root_relative.strip():
        raise RuntimeError(f"{manifest_path}: 'root' must be a non-empty string path.")

    keywords_section = manifest.get("keywords")
    if not isinstance(keywords_section, dict):
        raise RuntimeError(f"{manifest_path}: 'keywords' must be an object.")

    tags_modules = keywords_section.get("tags")
    affixes_modules = keywords_section.get("affixes")
    kid_rules_modules = keywords_section.get("kidRules")
    spid_rules_modules = keywords_section.get("spidRules")

    if not isinstance(tags_modules, list) or not all(isinstance(item, str) and item.strip() for item in tags_modules):
        raise RuntimeError(f"{manifest_path}: keywords.tags must be an array of non-empty strings.")
    if not isinstance(affixes_modules, list) or not all(isinstance(item, str) and item.strip() for item in affixes_modules):
        raise RuntimeError(f"{manifest_path}: keywords.affixes must be an array of non-empty strings.")
    if not isinstance(kid_rules_modules, list) or not all(isinstance(item, str) and item.strip() for item in kid_rules_modules):
        raise RuntimeError(f"{manifest_path}: keywords.kidRules must be an array of non-empty strings.")
    if not isinstance(spid_rules_modules, list) or not all(isinstance(item, str) and item.strip() for item in spid_rules_modules):
        raise RuntimeError(f"{manifest_path}: keywords.spidRules must be an array of non-empty strings.")

    root_spec = _read_object_module(manifest_path, root_relative, label="root")

    keywords = root_spec.get("keywords")
    if keywords is None:
        keywords = {}
        root_spec["keywords"] = keywords

    if not isinstance(keywords, dict):
        raise RuntimeError(f"{manifest_path}: root module must contain 'keywords' as an object when present.")

    keywords["tags"] = _read_array_modules(manifest_path, tags_modules, label="tags")
    keywords["affixes"] = _read_array_modules(manifest_path, affixes_modules, label="affixes")
    keywords["kidRules"] = _read_array_modules(manifest_path, kid_rules_modules, label="kidRules")
    keywords["spidRules"] = _read_array_modules(manifest_path, spid_rules_modules, label="spidRules")

    return root_spec


def _parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Compose affixes/affixes.json from modular JSON files."
    )
    parser.add_argument(
        "--manifest",
        default="affixes/affixes.modules.json",
        help="Path to modules manifest JSON.",
    )
    parser.add_argument(
        "--output",
        default="affixes/affixes.json",
        help="Output path for composed spec JSON.",
    )
    parser.add_argument(
        "--check",
        action="store_true",
        help="Do not write output. Fail if output file differs from composed JSON.",
    )
    return parser.parse_args()


def main() -> int:
    args = _parse_args()

    manifest_path = Path(args.manifest).resolve()
    output_path = Path(args.output).resolve()
    try:
        composed = compose_spec(manifest_path)
    except RuntimeError as exc:
        print(f"[ERROR] {exc}", file=sys.stderr)
        return 2

    if args.check:
        try:
            current = _read_json(output_path)
        except RuntimeError as exc:
            print(f"[ERROR] {exc}", file=sys.stderr)
            return 2
        if current != composed:
            print(
                f"ERROR: Composed spec does not match output file.\n"
                f" - Manifest: {manifest_path}\n"
                f" - Output:   {output_path}\n"
                "Run: python3 tools/compose_affixes.py",
                file=sys.stderr,
            )
            return 2

        print(f"OK: {output_path} is up to date with {manifest_path}")
        return 0

    should_write = True
    if output_path.exists():
        try:
            current = _read_json(output_path)
        except RuntimeError:
            should_write = True
        else:
            should_write = current != composed

    if should_write:
        _write_json(output_path, composed)
        print(f"Wrote composed spec: {output_path}")
    else:
        print(f"No changes: {output_path} already matches {manifest_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
