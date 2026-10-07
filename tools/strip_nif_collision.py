#!/usr/bin/env python3
"""Strip all Havok collision from an SSE NIF without renumbering blocks.

Every NiAVObject whose collision ref points at a collision object (cross-checked by
the collision object's Target pointer) gets the ref cleared to -1. Every bhk* block is
replaced in place by an unreferenced, empty NiStringExtraData, so no block index moves
and no other ref has to be rewritten. BSXFlags loses its physics bits.

Used to build Data/Meshes/CalamityAffixes/TrapMarkers from the vanilla meshes
(extracted from the game's BSAs; they are not stored in this repository):

    python3 tools/strip_nif_collision.py <vanilla.nif> <output.nif> [<vanilla.nif> <output.nif> ...]
"""
import struct, sys
COLLISION_TYPES = {'bhkCollisionObject','bhkSPCollisionObject','bhkNPCollisionObject','bhkBlendCollisionObject','bhkPCollisionObject'}
BSX_PHYSICS_BITS = 0x2 | 0x4 | 0x8 | 0x40 | 0x80   # Havok, Ragdoll, Complex, Dynamic, Articulated
DUMMY = 'NiStringExtraData'

def read_header(d):
    h = {}
    p = d.index(b'\n') + 1
    h['ver'], = struct.unpack_from('<I', d, p); p += 4 + 1
    h['uv'], h['nb'] = struct.unpack_from('<II', d, p); p += 8
    h['bsv'], = struct.unpack_from('<I', d, p); p += 4
    assert h['ver'] == 0x14020007 and h['bsv'] == 100, (hex(h['ver']), h['bsv'])
    for _ in range(3):
        n = d[p]; p += 1 + n
    h['types_count_off'] = p
    nt, = struct.unpack_from('<H', d, p); p += 2
    types = []
    for _ in range(nt):
        l, = struct.unpack_from('<I', d, p); p += 4; types.append(d[p:p+l].decode()); p += l
    h['types_end'] = p
    nb = h['nb']
    idx = list(struct.unpack_from('<%dH' % nb, d, p)); p += 2 * nb
    sizes = list(struct.unpack_from('<%dI' % nb, d, p)); p += 4 * nb
    h['after_sizes'] = p
    ns, ml = struct.unpack_from('<II', d, p); p += 8
    strs = []
    for _ in range(ns):
        l, = struct.unpack_from('<I', d, p); p += 4; strs.append(d[p:p+l].decode('latin1')); p += l
    ng, = struct.unpack_from('<I', d, p); p += 4 + 4 * ng
    h['blocks_off'] = p
    return h, types, idx, sizes, strs

def av_collision_offset(b):
    # NiObjectNET: name, extra count, extra refs, controller; NiAVObject (bsver 100):
    # flags u32, translation 3f, rotation 9f, scale f, then the collision ref.
    n, = struct.unpack_from('<I', b, 4)
    off = 4 + 4 + 4 * n + 4 + 4 + 12 + 36 + 4
    return off if off + 4 <= len(b) else None

def strip(src, dst):
    d = open(src, 'rb').read()
    h, types, idx, sizes, strs = read_header(d)
    nb = h['nb']
    blocks = []; q = h['blocks_off']
    for s in sizes:
        blocks.append(bytearray(d[q:q+s])); q += s
    footer = d[q:]
    tname = [types[i] for i in idx]
    collision = {i for i, t in enumerate(tname) if t in COLLISION_TYPES}
    bhk = {i for i, t in enumerate(tname) if t.startswith('bhk')}
    cleared = []
    for i, t in enumerate(tname):
        if t.startswith('bhk') or t in ('BSXFlags', 'BSBehaviorGraphExtraData'):
            continue
        b = blocks[i]
        if len(b) < 8:
            continue
        off = av_collision_offset(b)
        if off is None:
            continue
        ref, = struct.unpack_from('<i', b, off)
        if ref in collision:
            target, = struct.unpack_from('<i', blocks[ref], 0)
            assert target == i, f'{src}: collision {ref} targets {target}, not {i}'
            struct.pack_into('<i', b, off, -1)
            cleared.append((i, t, ref))
    assert {r for _, _, r in cleared} == collision, f'{src}: unclaimed collision objects'
    bsx = []
    for i, t in enumerate(tname):
        if t == 'BSXFlags':
            v, = struct.unpack_from('<I', blocks[i], 4)
            struct.pack_into('<I', blocks[i], 4, v & ~BSX_PHYSICS_BITS)
            bsx.append((hex(v), hex(v & ~BSX_PHYSICS_BITS)))
    # Dummy type index (append the type name if the file does not have it yet).
    out_types = list(types)
    if DUMMY not in out_types:
        out_types.append(DUMMY)
    dummy_index = out_types.index(DUMMY)
    for i in bhk:
        blocks[i] = bytearray(struct.pack('<ii', -1, -1))
        idx[i] = dummy_index
    hdr = bytearray(d[:h['types_count_off']])
    hdr += struct.pack('<H', len(out_types))
    for t in out_types:
        hdr += struct.pack('<I', len(t)) + t.encode()
    hdr += struct.pack('<%dH' % nb, *idx)
    hdr += struct.pack('<%dI' % nb, *[len(b) for b in blocks])
    hdr += d[h['after_sizes']:h['blocks_off']]
    out = bytes(hdr) + b''.join(bytes(b) for b in blocks) + footer
    open(dst, 'wb').write(out)
    return cleared, sorted(bhk), bsx, len(d), len(out)

if __name__ == '__main__':
    for src, dst in zip(sys.argv[1::2], sys.argv[2::2]):
        cleared, bhk, bsx, a, b = strip(src, dst)
        print(f'{src.split("/")[-1]}: cleared {[(i,t) for i,t,_ in cleared]} bhk->{DUMMY} {len(bhk)} BSX {bsx} {a}->{b} bytes')
