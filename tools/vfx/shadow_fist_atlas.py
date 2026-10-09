"""Shadow Boxer echo: a 16-frame (4x4) flipbook of a spectral fist made of smoke.

The smoke material is the base game's own smoke textures (density from their alpha),
so the result reads like Skyrim's ghost and shadow effects: a translucent dark-violet
fist with a soft fresnel rim that forms, then breaks up into a vanilla smoke puff.
No outlines, starbursts or speed lines.
"""
from __future__ import annotations

import math

import numpy as np
from PIL import Image

FRAMES = 16
FRAME_SIZE = 256
WORK = 512  # per-frame working resolution, downsampled to FRAME_SIZE

SMOKE_TEXTURES = {
    "spiral": ("textures\\effects\\smokespiralstatic01.dds", (2, 2)),
    "soft": ("textures\\effects\\fxsmokesoftsub01.dds", (4, 4)),
    "puff": ("textures\\effects\\fxsmokepuffanim04.dds", (8, 8)),
}

DARK = np.array([0.030, 0.020, 0.045])
MID = np.array([0.30, 0.24, 0.38])
GLOW = np.array([0.55, 0.40, 0.80])

_ys, _xs = np.mgrid[0:WORK, 0:WORK].astype(np.float32)
X = (_xs + 0.5) / WORK * 2 - 1
Y = 1 - (_ys + 0.5) / WORK * 2


def smoothstep(a, b, x):
    t = np.clip((x - a) / (b - a), 0, 1)
    return t * t * (3 - 2 * t)


def _round_box(px, py, cx, cy, hw, hh, r):
    qx = np.abs(px - cx) - (hw - r)
    qy = np.abs(py - cy) - (hh - r)
    return np.hypot(np.maximum(qx, 0), np.maximum(qy, 0)) + np.minimum(np.maximum(qx, qy), 0) - r


def _capsule(px, py, ax, ay, bx, by, r):
    pax, pay, bax, bay = px - ax, py - ay, bx - ax, by - ay
    h = np.clip((pax * bax + pay * bay) / (bax * bax + bay * bay), 0, 1)
    return np.hypot(pax - bax * h, pay - bay * h) - r


def _smin(a, b, k):
    h = np.clip(0.5 + 0.5 * (b - a) / k, 0, 1)
    return b * (1 - h) + a * h - k * h * (1 - h)


def fist_sdf(px, py):
    """Signed distance to a closed fist seen from the front (knuckles toward the viewer)."""
    angle = math.radians(-8)
    qx = px * math.cos(angle) - py * math.sin(angle)
    qy = px * math.sin(angle) + py * math.cos(angle)
    body = _round_box(qx, qy, -0.01, -0.03, 0.47, 0.29, 0.17)
    knuckles = None
    for kx, ky in [(-0.335, 0.215), (-0.11, 0.255), (0.115, 0.25), (0.33, 0.205)]:
        bump = np.hypot(qx - kx, (qy - ky) * 1.15) - 0.118
        knuckles = bump if knuckles is None else np.minimum(knuckles, bump)
    hand = _smin(body, knuckles, 0.045)
    thumb = _capsule(qx, qy, -0.40, -0.17, 0.07, -0.245, 0.10)
    return _smin(hand, thumb, 0.012)


def _cells(image: Image.Image, grid):
    image = image.convert("RGBA")
    w, h = image.size
    cw, ch = w // grid[0], h // grid[1]
    return [image.crop((c * cw, r * ch, c * cw + cw, r * ch + ch)) for r in range(grid[1]) for c in range(grid[0])]


def _density(cell: Image.Image, rotate=0.0, scale=1.0):
    """Red channel and alpha of a smoke cell at working resolution, 0..1.

    Enlarged cells are cropped off-centre (window anchored toward the top-left), which
    keeps the smoke lopsided instead of a centred, symmetric blob.
    """
    image = cell.rotate(rotate, resample=Image.BICUBIC)
    if scale != 1.0:
        w, h = image.size
        nw = int(w * scale)
        scaled = image.resize((nw, nw), Image.BICUBIC)
        canvas = Image.new("RGBA", (w, h))
        canvas.paste(scaled, ((w - nw) // 2, (h - nw) // 2))
        image = canvas
        if nw > w:
            shift = (nw - w) // 2
            image = image.crop((shift, shift, shift + w, shift + w))
    rgba = np.asarray(image.resize((WORK, WORK), Image.BICUBIC)).astype(np.float32) / 255
    return rgba[..., 0] * rgba[..., 3], rgba[..., 3]


def _over(rgb, alpha, color, color_alpha):
    out = color_alpha + alpha * (1 - color_alpha)
    blended = (color * color_alpha[..., None] + rgb * (alpha * (1 - color_alpha))[..., None]) / np.maximum(out, 1e-5)[..., None]
    return blended, out


def _frame(k, spiral, soft, puff):
    t = k / (FRAMES - 1)
    rgb = np.zeros(X.shape + (3,), np.float32)
    alpha = np.zeros(X.shape, np.float32)

    # Soft violet light behind the fist while it forms.
    glow = smoothstep(0.75, 0.0, np.hypot(X, Y + 0.02)) ** 2 * 0.45 * smoothstep(0.0, 0.12, t) * smoothstep(0.55, 0.18, t)
    rgb, alpha = _over(rgb, alpha, np.broadcast_to(GLOW, rgb.shape), glow)

    # The fist breaks up into a vanilla smoke puff.
    if t > 0.30:
        tp = (t - 0.30) / 0.70
        _, puff_a = _density(puff[min(63, int(18 + tp * 45))], scale=1.25 + 0.5 * tp)
        puff_alpha = puff_a * (0.80 - 0.80 * tp ** 1.4) * smoothstep(0.95, 0.45, np.hypot(X, Y))
        puff_col = DARK + (MID - DARK) * (0.35 * np.clip(puff_a / max(puff_a.max(), 1e-3), 0, 1))[..., None]
        rgb, alpha = _over(rgb, alpha, puff_col, np.clip(puff_alpha, 0, 1))

    # The fist itself: smoke density drives its body, edge and break-up.
    scale = 0.86 + 0.14 * smoothstep(0.0, 0.22, t) + 0.22 * smoothstep(0.45, 1.0, t)
    sdf = fist_sdf(X / scale, (Y + 0.03) / scale) * scale
    _, a1 = _density(spiral[0], rotate=-60 * t - 20, scale=1.15 + 0.15 * t)
    _, a2 = _density(spiral[3], rotate=50 * t + 70, scale=1.0)
    l3, a3 = _density(soft[5], rotate=-25 * t + 60, scale=0.9)
    density = 0.45 * a1 / max(a1.max(), 1e-3) + 0.35 * a2 / max(a2.max(), 1e-3) + 0.35 * a3 / max(a3.max(), 1e-3)
    density = np.clip((np.clip(density, 0, 1) - 0.12) * 1.5, 0, 1)
    shade = np.clip(l3 / np.maximum(a3, 0.05), 0, 1)  # this texture carries shading in RGB
    breakup = smoothstep(0.40, 1.0, t)
    edge = sdf + (0.5 - density) * (0.14 + 0.60 * breakup) * (1.0 - 0.5 * smoothstep(0.0, 0.25, Y / scale))
    mask = smoothstep(0.035, -0.07, edge)
    form = smoothstep(0.0, 0.14, t) * smoothstep(1.0, 0.60, t)
    top = smoothstep(-0.20, 0.35, Y / scale) * smoothstep(-0.16, 0.0, sdf)

    fresnel = smoothstep(-0.13, -0.005, edge) * smoothstep(0.035, -0.01, edge) * (0.45 + 0.75 * density) * (0.6 + 0.4 * top)
    shed = smoothstep(0.16, 0.0, edge) * (1 - mask) * density * 0.35
    tone = np.clip(0.4 * density + 0.3 * shade + 0.3 * top, 0, 1)
    body_col = DARK + (MID - DARK) * (tone ** 1.4)[..., None]
    rgb, alpha = _over(rgb, alpha, body_col, np.clip((mask * (0.22 + 0.30 * density) + shed) * form, 0, 1))
    rgb, alpha = _over(rgb, alpha, np.broadcast_to(MID * 0.4 + GLOW * 0.6, rgb.shape), np.clip(fresnel * 0.85 * form, 0, 1))

    image = Image.fromarray((np.dstack([np.clip(rgb, 0, 1), np.clip(alpha, 0, 1)]) * 255).astype(np.uint8), "RGBA")
    return image.resize((FRAME_SIZE, FRAME_SIZE), Image.LANCZOS)


def build_atlas(textures: dict[str, Image.Image]) -> Image.Image:
    """textures: {'spiral','soft','puff'} -> decoded vanilla images (see SMOKE_TEXTURES)."""
    spiral = _cells(textures["spiral"], SMOKE_TEXTURES["spiral"][1])
    soft = _cells(textures["soft"], SMOKE_TEXTURES["soft"][1])
    puff = _cells(textures["puff"], SMOKE_TEXTURES["puff"][1])
    atlas = Image.new("RGBA", (FRAME_SIZE * 4, FRAME_SIZE * 4))
    for k in range(FRAMES):
        atlas.paste(_frame(k, spiral, soft, puff), ((k % 4) * FRAME_SIZE, (k // 4) * FRAME_SIZE))
    return atlas
