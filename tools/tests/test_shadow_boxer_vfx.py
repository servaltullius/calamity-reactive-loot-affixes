#!/usr/bin/env python3
"""The Shadow Boxer echo art ships as one NIF plus one flipbook texture.

These checks keep the two in step: the NIF's spark particles must point at the shipped
atlas, play its 16 frames exactly once (no wrap back to the fist before the sprite dies),
and come from the effect origin. Rebuild with tools/vfx/build_shadow_boxer_vfx.py.
"""
from __future__ import annotations

import json
import struct
import sys
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools" / "vfx"))

from nif import Nif  # noqa: E402

MESH = REPO_ROOT / "Data" / "Meshes" / "CalamityAffixes" / "ShadowEchoHit.nif"
TEXTURES = REPO_ROOT / "Data" / "Textures"
PS, SHADER, LOD, EMIT, COLOR, DATA, ORIGIN, RATE, ACTIVE = 82, 83, 86, 87, 89, 81, 58, 53, 55


def shipped_path(game_path: str) -> Path:
    parts = game_path.split("\\")
    assert parts[0].lower() == "textures", game_path
    return TEXTURES.joinpath(*parts[1:])


class ShadowBoxerVfxTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.nif = Nif.load(MESH)

    def test_art_record_points_at_the_shipped_mesh(self) -> None:
        spec = json.loads((REPO_ROOT / "affixes" / "modules" / "spec.root.json").read_text(encoding="utf-8"))
        arts = {
            record["artObject"]["editorId"]: record["artObject"]["modelPath"]
            for record in spec["keywords"]["appendedRecords"]
            if record["type"] == "ArtObject"
        }
        self.assertEqual("CalamityAffixes\\ShadowEchoHit.nif", arts["CAFF_ARTO_VFX_SHADOW_ECHO_HIT"])
        self.assertTrue(MESH.is_file())

    def test_sprite_shader_uses_the_shipped_dxt5_atlas(self) -> None:
        block = self.nif.blocks[SHADER]
        extra, = struct.unpack_from("<I", block, 4)
        offset = 8 + 4 * extra + 4 + 8 + 16
        length, = struct.unpack_from("<I", block, offset)
        texture = block[offset + 4:offset + 4 + length].decode()
        atlas = shipped_path(texture)
        self.assertTrue(atlas.is_file(), f"shader texture {texture} is not shipped")
        header = atlas.read_bytes()[:128]
        height, width, _pitch, _depth, mips = struct.unpack_from("<IIIII", header, 12)
        self.assertEqual((1024, 1024), (width, height))
        self.assertEqual(b"DXT5", header[84:88])
        self.assertGreater(mips, 1)

    def test_flipbook_plays_sixteen_frames_once(self) -> None:
        nif = self.nif
        subtex = [i for i in range(len(nif.blocks)) if nif.type_of(i) == "BSPSysSubTexModifier"]
        self.assertEqual(1, len(subtex))
        block = nif.blocks[subtex[0]]
        _name, _order, target, active = struct.unpack_from("<iIiB", block, 0)
        start, start_fudge, end, loop, loop_fudge, count, count_fudge = struct.unpack_from("<7f", block, 13)
        self.assertEqual((PS, 1), (target, active))
        self.assertEqual((0.0, 0.0, 15.0, 0.0), (start, start_fudge, end, count_fudge))
        # The count is frames stepped over the sprite's whole life, not frames per second:
        # 34 played the fist twice per sprite in game. 16 steps reach the last frame at death.
        self.assertEqual(16.0, count, "one pass through the atlas per sprite life")
        self.assertEqual((end, 0.0), (loop, loop_fudge), "hold the last (empty) frame instead of wrapping to the fist")

        emitter = nif.blocks[EMIT]
        self.assertEqual("NiPSysSphereEmitter", nif.type_of(EMIT))
        _life, life_variation = struct.unpack_from("<2f", emitter, 61)
        self.assertEqual(0.0, life_variation)
        self.assertEqual(ORIGIN, struct.unpack_from("<i", emitter, 69)[0])

        data = nif.blocks[DATA]
        self.assertEqual(2, struct.unpack_from("<H", data, 4)[0], "pool capped at the fist plus one afterimage")
        self.assertEqual(1, data[46])
        self.assertEqual(16, struct.unpack_from("<I", data, 47)[0])

        ps = nif.blocks[PS]
        count_at = bytes(ps).find(struct.pack("<I", 13) + struct.pack("<i", 85))
        self.assertGreaterEqual(count_at, 0)
        modifiers = list(struct.unpack_from("<13i", ps, count_at + 4))
        self.assertEqual(subtex[0], modifiers[modifiers.index(COLOR) + 1], "SubTex must follow the colour modifier")

    def test_emission_survives_lod_and_a_late_first_update(self) -> None:
        nif = self.nif
        self.assertEqual("BSPSysLODModifier", nif.type_of(LOD))
        self.assertEqual(0, nif.blocks[LOD][12], "LOD would scale a two-sprite burst down to nothing")
        rate = struct.unpack_from("<f", nif.blocks[RATE], 0)[0]
        keys = nif.blocks[ACTIVE]
        on_until = struct.unpack_from("<f", keys, 8 + 5)[0]
        self.assertGreaterEqual(on_until, 0.2)
        self.assertGreaterEqual(rate * on_until, 10, "enough births that the pool cap, not timing, decides the count")


if __name__ == "__main__":
    unittest.main()
