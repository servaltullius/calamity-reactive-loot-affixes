"""Block-level SSE NIF container: edit block bytes, append blocks and strings, write back.

Blocks keep their indices, so references never need renumbering. Every edit that
changes a block's length goes through the header's size table on write.
"""
from __future__ import annotations

import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from strip_nif_collision import read_header  # noqa: E402


class Nif:
    def __init__(self, data: bytes):
        header, types, type_indices, sizes, strings = read_header(data)
        self._data = data
        self._header = header
        self.types = list(types)
        self.type_indices = list(type_indices)
        self.strings = list(strings)
        self.blocks: list[bytearray] = []
        offset = header["blocks_off"]
        for size in sizes:
            self.blocks.append(bytearray(data[offset:offset + size]))
            offset += size
        self.footer = data[offset:]

    @classmethod
    def load(cls, path: Path) -> "Nif":
        return cls(Path(path).read_bytes())

    def type_of(self, index: int) -> str:
        return self.types[self.type_indices[index]]

    def name_of(self, index: int) -> str | None:
        name, = struct.unpack_from("<i", self.blocks[index], 0)
        return self.strings[name] if name >= 0 else None

    def set_type(self, index: int, type_name: str) -> None:
        if type_name not in self.types:
            self.types.append(type_name)
        self.type_indices[index] = self.types.index(type_name)

    def append_block(self, type_name: str, data: bytes) -> int:
        self.blocks.append(bytearray(data))
        self.type_indices.append(0)
        self.set_type(len(self.blocks) - 1, type_name)
        return len(self.blocks) - 1

    def add_string(self, value: str) -> int:
        if value in self.strings:
            raise ValueError(f"string already present: {value}")
        self.strings.append(value)
        return len(self.strings) - 1

    def to_bytes(self) -> bytes:
        data, header = self._data, self._header
        line_end = data.index(b"\n") + 1
        out = bytearray(data[:header["types_count_off"]])
        struct.pack_into("<I", out, line_end + 4 + 1 + 4, len(self.blocks))  # version, endian, user version
        out += struct.pack("<H", len(self.types))
        for name in self.types:
            out += struct.pack("<I", len(name)) + name.encode()
        out += struct.pack(f"<{len(self.blocks)}H", *self.type_indices)
        out += struct.pack(f"<{len(self.blocks)}I", *[len(block) for block in self.blocks])
        out += struct.pack("<II", len(self.strings), max(len(s) for s in self.strings))
        for value in self.strings:
            out += struct.pack("<I", len(value)) + value.encode("latin1")
        old_count, = struct.unpack_from("<I", data, header["after_sizes"])
        offset = header["after_sizes"] + 8
        for _ in range(old_count):
            length, = struct.unpack_from("<I", data, offset)
            offset += 4 + length
        out += data[offset:header["blocks_off"]]  # groups
        return bytes(out) + b"".join(bytes(block) for block in self.blocks) + self.footer


def sized_string(value: bytes) -> bytes:
    return struct.pack("<I", len(value)) + value


def replace_sized_string(block: bytearray, old: bytes, new: bytes) -> bytearray:
    needle = sized_string(old)
    at = bytes(block).find(needle)
    if at < 0 or bytes(block).count(needle) != 1:
        raise ValueError(f"sized string {old!r} not found exactly once")
    return bytearray(block[:at]) + sized_string(new) + block[at + len(needle):]
