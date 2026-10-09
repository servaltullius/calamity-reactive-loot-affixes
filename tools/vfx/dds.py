"""DXT5 DDS writer with a full mip chain (Pillow encodes each level, we stitch them)."""
from __future__ import annotations

import io
import struct
from pathlib import Path

from PIL import Image


def _dxt5_level(image: Image.Image) -> bytes:
    buffer = io.BytesIO()
    image.save(buffer, format="DDS", pixel_format="DXT5")
    raw = buffer.getvalue()
    if raw[84:88] != b"DXT5":
        raise RuntimeError("Pillow did not write a DXT5 surface")
    return raw[128:]


def write_dxt5(image: Image.Image, path: Path) -> int:
    """Write image as DXT5 with mips down to 4x4. Returns the mip count."""
    image = image.convert("RGBA")
    width, height = image.size
    levels = [image]
    while levels[-1].size[0] > 4 and levels[-1].size[1] > 4:
        w, h = levels[-1].size
        # Resample premultiplied so transparent texels do not bleed dark fringes.
        levels.append(levels[-1].convert("RGBa").resize((w // 2, h // 2), Image.LANCZOS).convert("RGBA"))
    data = b"".join(_dxt5_level(level) for level in levels)
    flags = 0x1 | 0x2 | 0x4 | 0x1000 | 0x20000 | 0x80000  # CAPS|HEIGHT|WIDTH|PIXELFORMAT|MIPMAPCOUNT|LINEARSIZE
    pixel_format = struct.pack("<II4sIIIII", 32, 0x4, b"DXT5", 0, 0, 0, 0, 0)
    caps = 0x1000 | 0x8 | 0x400000  # TEXTURE | COMPLEX | MIPMAP
    header = (
        b"DDS "
        + struct.pack("<IIIIIII", 124, flags, height, width, width * height, 0, len(levels))
        + b"\0" * 44
        + pixel_format
        + struct.pack("<IIIII", caps, 0, 0, 0, 0)
    )
    Path(path).write_bytes(header + data)
    return len(levels)
