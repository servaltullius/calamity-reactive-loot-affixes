#!/usr/bin/env python3
"""Rebuild the Shadow Boxer echo hit art from the base game's files.

    python3 tools/vfx/build_shadow_boxer_vfx.py --game-data "<Skyrim Special Edition>/Data"

Writes Data/Meshes/CalamityAffixes/ShadowEchoHit.nif, Data/Textures/CalamityAffixes/ShadowFistSmoke.dds
and Data/Sound/FX/CalamityAffixes/ShadowPunch0[1-3].wav (needs ffmpeg). --preview also writes the
atlas as PNG.
"""
from __future__ import annotations

import argparse
import io
import sys
from pathlib import Path

from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parent))
import bsa  # noqa: E402
import dds  # noqa: E402
import shadow_echo_nif  # noqa: E402
import shadow_fist_atlas  # noqa: E402
import shadow_punch_sound  # noqa: E402

REPO_ROOT = Path(__file__).resolve().parents[2]
TEXTURE_GAME_PATH = "textures\\CalamityAffixes\\ShadowFistSmoke.dds"
MESH_OUT = REPO_ROOT / "Data" / "Meshes" / "CalamityAffixes" / "ShadowEchoHit.nif"
TEXTURE_OUT = REPO_ROOT / "Data" / "Textures" / "CalamityAffixes" / "ShadowFistSmoke.dds"
SOUND_OUT = REPO_ROOT / "Data" / "Sound" / "FX" / "CalamityAffixes"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--game-data", type=Path, required=True, help="Skyrim Special Edition Data folder (with the vanilla BSAs)")
    parser.add_argument("--preview", type=Path, help="also write the atlas PNG here")
    args = parser.parse_args()

    textures = {}
    for key, (inner_path, _grid) in shadow_fist_atlas.SMOKE_TEXTURES.items():
        image = Image.open(io.BytesIO(bsa.find(args.game_data, inner_path)))
        image.load()
        textures[key] = image
    atlas = shadow_fist_atlas.build_atlas(textures)
    if args.preview:
        atlas.save(args.preview)
    TEXTURE_OUT.parent.mkdir(parents=True, exist_ok=True)
    mips = dds.write_dxt5(atlas, TEXTURE_OUT)

    vanilla = bsa.find(args.game_data, shadow_echo_nif.VANILLA_PATH)
    MESH_OUT.parent.mkdir(parents=True, exist_ok=True)
    MESH_OUT.write_bytes(shadow_echo_nif.build(vanilla, TEXTURE_GAME_PATH))
    sounds = shadow_punch_sound.build(lambda inner: bsa.find(args.game_data, inner), SOUND_OUT)
    print(f"wrote {MESH_OUT.relative_to(REPO_ROOT)}, {TEXTURE_OUT.relative_to(REPO_ROOT)} ({mips} mips)")
    for sound in sounds:
        print(f"wrote {sound.relative_to(REPO_ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
