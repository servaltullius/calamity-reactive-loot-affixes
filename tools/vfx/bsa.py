"""Minimal reader for Skyrim SE (v105) BSA archives: list and extract files."""
from __future__ import annotations

import struct
from pathlib import Path

try:
    import lz4.frame
except ImportError:  # only needed for compressed archives (meshes)
    lz4 = None


def _index(path: Path):
    stream = path.open("rb")
    magic, version, offset, flags, folder_count, _file_count, _folder_names_len, file_names_len, _ = struct.unpack(
        "<4sIIIIIIII", stream.read(36)
    )
    if magic != b"BSA\0" or version != 105:
        raise ValueError(f"{path} is not an SSE (v105) BSA")
    stream.seek(offset)
    folders = [struct.unpack("<QIIQ", stream.read(24)) for _ in range(folder_count)]
    records = []
    end = 0
    for _hash, count, _pad, folder_offset in folders:
        # The stored offset includes the file-name block length.
        stream.seek(folder_offset - file_names_len)
        name_len = stream.read(1)[0]
        name = stream.read(name_len).rstrip(b"\0").decode("latin1")
        files = [struct.unpack("<QII", stream.read(16)) for _ in range(count)]
        records.append((name, files))
        end = max(end, stream.tell())
    stream.seek(end)
    names = stream.read(file_names_len).split(b"\0")
    entries = {}
    i = 0
    for folder, files in records:
        for _hash, size, file_offset in files:
            entries[(folder + "\\" + names[i].decode("latin1")).lower()] = (size, file_offset)
            i += 1
    compressed_default = bool(flags & 0x4)
    embedded_names = bool(flags & 0x100)
    return stream, entries, compressed_default, embedded_names


def read(archive: Path, inner_path: str) -> bytes:
    """Return the bytes of one file (path like 'meshes\\magic\\foo.nif')."""
    stream, entries, compressed_default, embedded_names = _index(archive)
    with stream:
        key = inner_path.replace("/", "\\").lower()
        if key not in entries:
            raise KeyError(f"{inner_path} not in {archive}")
        size, offset = entries[key]
        compressed = compressed_default ^ bool(size & 0x40000000)
        size &= 0x3FFFFFFF
        stream.seek(offset)
        raw = stream.read(size)
        if embedded_names:
            raw = raw[1 + raw[0]:]
        if compressed:
            if lz4 is None:
                raise RuntimeError("python lz4 is required for compressed BSAs")
            raw = lz4.frame.decompress(raw[4:])
        return raw


def find(data_dir: Path, inner_path: str) -> bytes:
    """Search the base game's archives under data_dir for inner_path."""
    for archive in sorted(data_dir.glob("Skyrim - *.bsa")):
        try:
            return read(archive, inner_path)
        except KeyError:
            continue
    raise KeyError(f"{inner_path} not found in {data_dir}")
