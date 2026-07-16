#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import struct
import zlib
from pathlib import Path
from typing import Iterator


COMPRESSED_RECORD_FLAG = 0x00040000
MAJOR_RECORD_HEADER_SIZE = 24
LOCAL_FORM_ID_MASK = 0x00FFFFFF


def _subrecords(payload: bytes) -> Iterator[tuple[bytes, bytes]]:
    position = 0
    extended_size: int | None = None

    while position + 6 <= len(payload):
        signature = payload[position : position + 4]
        size = struct.unpack_from("<H", payload, position + 4)[0]
        position += 6

        if signature == b"XXXX":
            if size != 4 or position + 4 > len(payload):
                raise ValueError("Invalid XXXX subrecord.")
            extended_size = struct.unpack_from("<I", payload, position)[0]
            position += 4
            continue

        actual_size = extended_size if extended_size is not None else size
        extended_size = None
        if position + actual_size > len(payload):
            raise ValueError(f"Invalid {signature!r} subrecord size.")

        yield signature, payload[position : position + actual_size]
        position += actual_size

    if position != len(payload):
        raise ValueError(f"Trailing subrecord bytes: {len(payload) - position}.")


def _read_payload(data: bytes, position: int, size: int, flags: int) -> bytes:
    payload = data[position + MAJOR_RECORD_HEADER_SIZE : position + MAJOR_RECORD_HEADER_SIZE + size]
    if not flags & COMPRESSED_RECORD_FLAG:
        return payload

    if len(payload) < 4:
        raise ValueError("Compressed record is missing its uncompressed-size prefix.")

    expected_size = struct.unpack_from("<I", payload, 0)[0]
    decompressed = zlib.decompress(payload[4:])
    if len(decompressed) != expected_size:
        raise ValueError(
            f"Compressed record size mismatch: expected {expected_size}, got {len(decompressed)}."
        )
    return decompressed


def _extract_metadata(signature: bytes, payload: bytes) -> tuple[str | None, int | None]:
    editor_id: str | None = None
    next_object_id: int | None = None

    for subrecord_signature, subrecord_payload in _subrecords(payload):
        if signature == b"TES4" and subrecord_signature == b"HEDR":
            if len(subrecord_payload) < 12:
                raise ValueError("TES4 HEDR subrecord is too small.")
            _, _, next_object_id = struct.unpack_from("<fII", subrecord_payload, 0)
        elif subrecord_signature == b"EDID" and editor_id is None:
            editor_id = subrecord_payload.rstrip(b"\0").decode("utf-8", "strict")

    return editor_id, next_object_id


def _extract_records(data: bytes) -> tuple[list[dict[str, str | None]], int]:
    records: list[dict[str, str | None]] = []
    next_object_id: int | None = None

    def walk(start: int, end: int) -> None:
        nonlocal next_object_id
        position = start

        while position < end:
            signature = data[position : position + 4]
            if signature == b"GRUP":
                group_size = struct.unpack_from("<I", data, position + 4)[0]
                if group_size < MAJOR_RECORD_HEADER_SIZE or position + group_size > end:
                    raise ValueError("Invalid GRUP size.")
                walk(position + MAJOR_RECORD_HEADER_SIZE, position + group_size)
                position += group_size
                continue

            if position + MAJOR_RECORD_HEADER_SIZE > end:
                raise ValueError("Truncated major-record header.")

            size, flags, raw_form_id = struct.unpack_from("<III", data, position + 4)
            record_end = position + MAJOR_RECORD_HEADER_SIZE + size
            if record_end > end:
                raise ValueError("Truncated major-record payload.")

            payload = _read_payload(data, position, size, flags)
            editor_id, header_next_object_id = _extract_metadata(signature, payload)
            if signature == b"TES4":
                next_object_id = header_next_object_id
            else:
                records.append(
                    {
                        "localFormId": f"0x{raw_form_id & LOCAL_FORM_ID_MASK:06X}",
                        "signature": signature.decode("ascii", "strict"),
                        "editorId": editor_id,
                    }
                )

            position = record_end

        if position != end:
            raise ValueError("Record walk did not end on a record boundary.")

    walk(0, len(data))
    if next_object_id is None:
        raise ValueError("TES4 HEDR next object ID was not found.")

    records.sort(key=lambda item: int(str(item["localFormId"])[2:], 16))
    return records, next_object_id


def _parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Extract a frozen local FormID/signature/EditorID allocation fixture from a Skyrim plugin."
    )
    parser.add_argument("--plugin", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--release-tag", required=True)
    parser.add_argument("--commit", required=True)
    parser.add_argument("--commit-short", required=True)
    parser.add_argument("--generator-version", required=True)
    parser.add_argument("--expected-record-count", required=True, type=int)
    return parser.parse_args()


def main() -> int:
    args = _parse_args()
    plugin_path = args.plugin.resolve()
    data = plugin_path.read_bytes()
    records, next_object_id = _extract_records(data)

    if len(records) != args.expected_record_count:
        raise ValueError(
            f"Expected {args.expected_record_count} major records, found {len(records)} in {plugin_path}."
        )

    fixture = {
        "fixtureVersion": 1,
        "source": {
            "plugin": args.plugin.as_posix(),
            "releaseTag": args.release_tag,
            "commit": args.commit,
            "commitShort": args.commit_short,
            "generatorPackage": "Mutagen.Bethesda.Skyrim",
            "generatorVersion": args.generator_version,
            "sourceEspSha256": hashlib.sha256(data).hexdigest(),
            "majorRecordCount": len(records),
            "nextObjectId": f"0x{next_object_id:06X}",
        },
        "records": records,
    }

    output_path = args.output.resolve()
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(
        json.dumps(fixture, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )
    print(f"Wrote {len(records)} records to {output_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
