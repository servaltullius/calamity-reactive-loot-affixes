"""Shadow Boxer echo sound: the vanilla large punch, pitched down and darkened, led in by a
reversed unarmed swing and carried by the low end of Unrelenting Force.

Each variant pairs the n-th vanilla punch with a swing and a push take, so the sound
descriptor can pick one at random like vanilla impacts do. Needs ffmpeg on PATH.
"""
from __future__ import annotations

import subprocess
import tempfile
from pathlib import Path

PUNCH = "sound\\fx\\fx\\melee\\punchlarge\\fx_melee_punchlarge_0{n}.wav"
SWING = "sound\\fx\\wpn\\swing\\unarmed\\wpn_swing_unarmed_0{n}.wav"
PUSH = "sound\\fx\\voc\\shout\\impact\\push\\voc_shoutpower_impact_push_0{n}.wav"
VARIANTS = ((2, 1, 3), (3, 2, 2), (1, 1, 1))   # (punch, swing, push) takes; the first is the approved demo
LEAD_IN_MS = 180


def _ffmpeg(*args: str) -> None:
    subprocess.run(["ffmpeg", "-v", "error", "-y", *args], check=True)


def build(read_vanilla, out_dir: Path) -> list[Path]:
    """read_vanilla(inner_path) -> bytes. Writes ShadowPunch0N.wav files into out_dir."""
    out_dir.mkdir(parents=True, exist_ok=True)
    written = []
    with tempfile.TemporaryDirectory(prefix="caff-shadow-punch-") as tmp:
        work = Path(tmp)
        for index, (punch, swing, push) in enumerate(VARIANTS, start=1):
            sources = {}
            for name, template, take in (("punch", PUNCH, punch), ("swing", SWING, swing), ("push", PUSH, push)):
                raw = work / f"{name}{index}.wav"
                raw.write_bytes(read_vanilla(template.format(n=take)))
                mono = work / f"{name}{index}_mono.wav"
                _ffmpeg("-i", str(raw), "-ac", "1", "-ar", "44100", "-c:a", "pcm_s16le", str(mono))
                sources[name] = mono
            deep = work / f"deep{index}.wav"
            _ffmpeg("-i", str(sources["punch"]), "-af",
                    "asetrate=44100*0.78,aresample=44100,lowpass=f=5200,"
                    "aecho=0.8:0.55:60|130:0.35|0.22,afade=t=out:st=0.9:d=0.35", "-t", "1.3", str(deep))
            whoosh = work / f"whoosh{index}.wav"
            _ffmpeg("-i", str(sources["swing"]), "-af",
                    "areverse,asetrate=44100*0.7,aresample=44100,highpass=f=180,afade=t=in:st=0:d=0.08,volume=0.9",
                    "-t", "0.22", str(whoosh))
            boom = work / f"boom{index}.wav"
            _ffmpeg("-i", str(sources["push"]), "-af", "lowpass=f=170,volume=2.2,afade=t=out:st=0.25:d=0.35",
                    "-t", "0.6", str(boom))
            target = out_dir / f"ShadowPunch0{index}.wav"
            _ffmpeg("-i", str(whoosh), "-i", str(deep), "-i", str(boom), "-filter_complex",
                    f"[1]adelay={LEAD_IN_MS}[d];[2]adelay={LEAD_IN_MS}[b];[0][d][b]amix=inputs=3:normalize=0,"
                    "alimiter=limit=0.95,loudnorm=I=-12:TP=-1:LRA=11:linear=true",
                    "-ac", "1", "-ar", "44100", "-c:a", "pcm_s16le", str(target))
            written.append(target)
    return written
