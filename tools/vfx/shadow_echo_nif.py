"""Build Meshes/CalamityAffixes/ShadowEchoHit.nif from the vanilla AbsorbSpellHitEffect01.nif.

1. Purple: the wisp particles' colour modifier turns dark violet.
2. Shadow fist: the spark particles (pSparks01) become one or two large camera-facing
   sprites that play the 16-frame smoke-fist atlas once (BSPSysSubTexModifier), emitted
   from the effect origin (MagicEffectsNode), alpha-blended and drawn over the target.

Every patch asserts the vanilla value it replaces, so a different source file fails
loudly instead of producing a broken mesh. No existing block moves.
"""
from __future__ import annotations

import struct

from nif import Nif, replace_sized_string

VANILLA_PATH = "meshes\\magic\\absorbspellhiteffect01.nif"
FRAMES = 16
LIFE_SECONDS = 0.45
# BSPSysSubTexModifier's frame count is how many frames a sprite steps through over its whole
# life, not a rate: 34 played the 16-frame atlas twice per sprite (2026-10-09 recording). The
# loop start sits on the last, empty frame, so a sprite holds it instead of wrapping to the fist.
FRAME_COUNT = float(FRAMES)
SPRITE_RADIUS = 34.0

WISP_COLOR, DATA, PS, SHADER, ALPHA, LOD, EMIT, COLOR, SCALE, GRAVITY, RATE, ACTIVE, ORIGIN = 71, 81, 82, 83, 84, 86, 87, 89, 90, 91, 53, 55, 58
MAX_SPRITES = 2          # the fist and one trailing afterimage
EMIT_SECONDS = 0.30      # long enough to survive a late first update; the cap stops a third sprite


def _near(a, b):
    return abs(a - b) < 1e-3


def _f(block, offset):
    return struct.unpack_from("<f", block, offset)[0]


def _properties_offset(block) -> int:
    """Offset just past NiObjectNET (name, extra data list, controller)."""
    extra_count, = struct.unpack_from("<I", block, 4)
    return 8 + 4 * extra_count + 4


def build(vanilla: bytes, texture_path: str) -> bytes:
    nif = Nif(vanilla)
    assert nif.type_of(PS) == "NiParticleSystem" and nif.name_of(PS) == "pSparks01"
    assert nif.type_of(ORIGIN) == "NiNode" and nif.name_of(ORIGIN) == "XYZDebugGeo01"
    assert nif.type_of(DATA) == "NiPSysData"

    # --- 1. purple wisps (were ice blue) and violet spark palette
    block = nif.blocks[WISP_COLOR]
    assert nif.type_of(WISP_COLOR) == "BSPSysSimpleColorModifier"
    assert [round(v, 3) for v in struct.unpack_from("<4f", block, 53)] == [0.216, 0.514, 0.878, 1.0]
    edge, peak = (0.32, 0.0, 0.62, 0.0), (0.62, 0.14, 0.95, 1.0)
    struct.pack_into("<12f", block, 37, *edge, *peak, *edge)
    nif.blocks[SHADER] = replace_sized_string(
        nif.blocks[SHADER], b"textures\\effects\\gradients\\GradIceSparkle.dds", b"textures\\effects\\gradients\\GradVioBright.dds"
    )

    # --- 2. shadow fist sprites
    block = nif.blocks[SHADER]
    o = _properties_offset(block)
    assert struct.unpack_from("<I", block, o)[0] == 0x80000010      # ZBuffer_Test | Greyscale_To_PaletteColor
    struct.pack_into("<I", block, o, 0x8)                           # Vertex_Alpha only: own colours, drawn over the body
    block = replace_sized_string(block, b"textures\\effects\\FXGlowSpotLinearAlpha.dds", texture_path.encode())
    tail = o + 8 + 16 + 4 + len(texture_path)
    assert block[tail] == 3 and block[tail + 1] == 255
    block[tail + 1] = 0                                             # lighting influence: unlit
    nif.blocks[SHADER] = block

    block = nif.blocks[ALPHA]
    o = _properties_offset(block)
    assert struct.unpack_from("<H", block, o)[0] == 4109            # SRC_ALPHA / ONE (additive)
    struct.pack_into("<H", block, o, 4333)                          # SRC_ALPHA / INV_SRC_ALPHA

    block = nif.blocks[EMIT]
    assert nif.type_of(EMIT) == "NiPSysMeshEmitter"
    assert _near(_f(block, 13), 240) and _near(_f(block, 53), 2.0) and _near(_f(block, 61), 0.6666667)
    struct.pack_into("<f", block, 13, 0.0)                          # speed
    struct.pack_into("<4f", block, 53, SPRITE_RADIUS, 2.0, LIFE_SECONDS, 0.0)
    # Sphere emitter on the origin node; the name stays, so the emitter controller still finds it.
    nif.blocks[EMIT] = bytearray(block[:69]) + struct.pack("<if", ORIGIN, 6.0)
    nif.set_type(EMIT, "NiPSysSphereEmitter")

    block = nif.blocks[COLOR]
    assert [round(v, 3) for v in struct.unpack_from("<6f", block, 13)] == [0.1, 0.9, 0.0, 0.1, 0.9, 1.0]
    struct.pack_into("<6f", block, 13, 0.02, 0.98, 0.0, 0.02, 0.98, 1.0)   # the frames fade themselves
    struct.pack_into("<12f", block, 37, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0)

    block = nif.blocks[SCALE]
    count, = struct.unpack_from("<I", block, 13)
    assert count == 40
    struct.pack_into(f"<{count}f", block, 17, *[1.0 + 0.06 * i / (count - 1) for i in range(count)])

    block = nif.blocks[GRAVITY]
    assert nif.type_of(GRAVITY) == "NiPSysGravityModifier" and block[12] == 1
    block[12] = 0

    # Emission. The sparks' LOD modifier scales emission down to 20% at ordinary combat
    # range, which turns a one- or two-sprite burst into nothing; switch it off. Then cap
    # the particle pool at MAX_SPRITES and keep the emitter on long enough that a late
    # first update still emits: the cap, not the window, decides how many sprites appear.
    block = nif.blocks[LOD]
    assert nif.type_of(LOD) == "BSPSysLODModifier" and block[12] == 1
    assert [round(v, 2) for v in struct.unpack_from("<3f", block, 13)] == [0.03, 0.23, 0.2]
    block[12] = 0
    block = nif.blocks[RATE]
    assert _near(_f(block, 0), 150)
    struct.pack_into("<f", block, 0, 60.0)
    block = nif.blocks[ACTIVE]
    assert struct.unpack_from("<II", block, 0) == (4, 5) and _near(_f(block, 13), 1.6666666)
    struct.pack_into("<f", block, 13, EMIT_SECONDS)

    # Particle data: subtexture offsets for a 4x4 atlas.
    block = nif.blocks[DATA]
    assert len(block) == 70 and block[46:51] == b"\0" * 5           # no texture indices, no offsets
    assert struct.unpack_from("<H", block, 4)[0] == 102              # BS Max Vertices = particle pool size
    struct.pack_into("<H", block, 4, MAX_SPRITES)
    offsets = b"".join(struct.pack("<4f", (k % 4) * 0.25, 0.25, (k // 4) * 0.25, 0.25) for k in range(FRAMES))
    nif.blocks[DATA] = bytearray(block[:46]) + b"\x01" + struct.pack("<I", FRAMES) + offsets + block[51:]

    # BSPSysSubTexModifier: play frames 0..15 once over the sprite's life, then hold frame 15.
    name = nif.add_string("BSPSysSubTexModifier:12")
    order_general = 3000
    last = FRAMES - 1
    subtex = struct.pack("<iIiB", name, order_general, PS, 1) + struct.pack(
        "<7f", 0.0, 0.0, last, last, 0.0, FRAME_COUNT, 0.0
    )
    subtex_index = nif.append_block("BSPSysSubTexModifier", subtex)

    # Insert it right after the colour modifier, as the vanilla flipbook effects do.
    block = nif.blocks[PS]
    old = struct.pack("<I", 12) + struct.pack("<12i", *range(85, 97))
    at = bytes(block).find(old)
    assert at >= 0 and bytes(block).count(old) == 1
    modifiers = list(range(85, 97))
    modifiers.insert(modifiers.index(COLOR) + 1, subtex_index)
    nif.blocks[PS] = bytearray(block[:at]) + struct.pack("<I", len(modifiers)) + struct.pack(f"<{len(modifiers)}i", *modifiers) + block[at + len(old):]

    return nif.to_bytes()
