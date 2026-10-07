#!/usr/bin/env python3
"""Trap world markers are presentation only: their meshes must carry no Havok.

The vanilla bear trap mesh keeps its jaws on the L_TRAP collision layer, so the
engine's TESTrapListener tracked our generated marker as a live trap. Retiring the
marker while an actor touched it crashed a Havok job thread (SkyrimSE.exe+3FF101,
TrapEntry / TrapTargetEntry in the registers). Each marker therefore points at a
collision-stripped copy under Meshes\\CalamityAffixes\\TrapMarkers.
"""
from __future__ import annotations

import json
import struct
import sys
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from strip_nif_collision import BSX_PHYSICS_BITS, read_header  # noqa: E402

MARKER_MESH_PREFIX = "CalamityAffixes\\TrapMarkers\\"


def movable_static_records() -> list[dict]:
    spec = json.loads((REPO_ROOT / "affixes" / "modules" / "spec.root.json").read_text(encoding="utf-8"))
    return [
        record["movableStatic"]
        for record in spec["keywords"]["appendedRecords"]
        if record["type"] == "MovableStatic"
    ]


class TrapMarkerMeshTests(unittest.TestCase):
    def test_every_world_marker_uses_a_shipped_collision_free_mesh(self) -> None:
        records = movable_static_records()
        self.assertEqual(6, len(records))
        for record in records:
            model_path = record["modelPath"]
            with self.subTest(editor_id=record["editorId"]):
                self.assertTrue(
                    model_path.startswith(MARKER_MESH_PREFIX),
                    f"{record['editorId']} must use a collision-free marker mesh, got {model_path}",
                )
                mesh = REPO_ROOT / "Data" / "Meshes" / Path(*model_path.split("\\"))
                self.assertTrue(mesh.is_file(), f"missing shipped marker mesh: {mesh}")
                self.assert_mesh_has_no_havok(mesh)

    def assert_mesh_has_no_havok(self, mesh: Path) -> None:
        data = mesh.read_bytes()
        header, types, block_type_indices, sizes, _ = read_header(data)
        used_types = [types[index] for index in block_type_indices]
        self.assertEqual(
            [],
            sorted({name for name in used_types if name.startswith("bhk")}),
            f"{mesh.name} still contains Havok blocks",
        )
        offset = header["blocks_off"]
        bsx_values = []
        for name, size in zip(used_types, sizes):
            if name == "BSXFlags":
                bsx_values.append(struct.unpack_from("<I", data, offset + 4)[0])
            offset += size
        self.assertEqual(1, len(bsx_values), f"{mesh.name} should keep exactly one BSXFlags block")
        self.assertEqual(0, bsx_values[0] & BSX_PHYSICS_BITS, f"{mesh.name} BSXFlags still claims physics")


if __name__ == "__main__":
    unittest.main()
