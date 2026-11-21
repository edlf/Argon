"""Survey the game's binary records dword by dword over every file on the disc.

For each record type (see src/original/*_records.hpp) prints, per 4-byte offset: how many records have it non-zero,
the number of distinct values and a few samples (hex and float). Fields that are zero everywhere are runtime-only
or unused; the varying ones are worth tracing in the XBE.

usage: python -I record_survey.py [record ...]      (default: all)
"""
import glob
import os
import struct
import sys
from collections import Counter, defaultdict

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import cmm  # noqa: E402

GAME = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'game_data')

samples = defaultdict(list)          # record name -> list of (source, bytes)


def add(name, source, raw):
    samples[name].append((source, raw))


def u32(d, o):
    return struct.unpack_from('<I', d, o)[0]


def collect_cmm(path):
    try:
        c = cmm.CMM(path)
    except Exception as e:          # noqa: BLE001 - a survey keeps going past odd files
        print('skip', path, e, file=sys.stderr)
        return
    d = c.data
    src = os.path.relpath(path, GAME)
    add('CmmHeader', src, d[:0x48])
    for m in c.meshes:
        add('CmmMesh', src, m.raw)
        if m.pushbuf_ofs is not None:
            add('CmmSurfaceBlock.setup%d' % u32(d, m.pushbuf_ofs + 4), src, d[m.pushbuf_ofs:m.pushbuf_ofs + u32(d, m.pushbuf_ofs)])
        if m.morph_ofs is not None:
            add('CmmMorphHeader', src, d[m.morph_ofs:m.morph_ofs + 0x10])
        if m.skin_ofs is not None:
            add('CmmSkinHeader', src, d[m.skin_ofs:m.skin_ofs + 0x38])
    for i in range(c.num_nodes):
        add('CmmMaterial', src, d[c.nodes_ofs + i * 0xec:c.nodes_ofs + (i + 1) * 0xec])
    if c.bones_ofs is not None:
        for i in range(c.num_bones):
            add('CmmBone', src, d[c.bones_ofs + i * 0x1a0:c.bones_ofs + (i + 1) * 0x1a0])
    if c.textable_ofs is not None:
        for i in range(c.num_textures):
            add('CmmTextureEntry', src, d[c.textable_ofs + i * 16:c.textable_ofs + (i + 1) * 16])
    eo = c.end_of_mesh_data
    ne = c.num_0x30 if c.num_0x30 < 64 else 0
    ao = eo + ne * 0x5bc
    for i in range(ne):
        r = d[eo + i * 0x5bc:eo + (i + 1) * 0x5bc]
        add('CmmEffect', src, r)
        ao += u32(r, 0x50)
    nt = c.num_0x40
    if 0 < nt < 4096 and ao + nt * 0x80 <= len(d):
        for i in range(nt):
            add('CmmAnimationTrack', src, d[ao + i * 0x80:ao + (i + 1) * 0x80])


def collect_array(name_header, name_record, path, header_size, record_size):
    d = open(path, 'rb').read()
    src = os.path.relpath(path, GAME)
    add(name_header, src, d[:header_size])
    for i in range(u32(d, 0)):
        o = header_size + i * record_size
        if o + record_size <= len(d):
            add(name_record, src, d[o:o + record_size])


def collect_tracks():
    for t in glob.glob(os.path.join(GAME, 'data', 'tracks', '*')):
        for f, h, r, size in (('locator.dat', 'LocatorHeader', 'Locator', (0x10, 0xc0)),
                              ('sprite.dat', 'QuadSpriteHeader', 'QuadSprite', (0x18, 0x84)),
                              ('water.dat', 'WaterHeader', 'WaterBox', (0x1c, 40))):
            p = os.path.join(t, f)
            if os.path.exists(p):
                collect_array(h, r, p, *size)
        p = os.path.join(t, 'prop.dat')
        if os.path.exists(p):
            add('TrackProperties', os.path.relpath(p, GAME), open(p, 'rb').read()[:0x178])


def collect_weapons():
    p = os.path.join(GAME, 'data', 'weapon.nwp')
    d = open(p, 'rb').read()
    for pilot in range(16):
        b = pilot * 0x2384
        add('NwpPilot', 'pilot%d' % pilot, d[b:b + 0x48])
        for w in range(11):
            o = b + 0x48 + w * 0x334
            add('NwpWeapon', 'pilot%d/%s' % (pilot, d[o + 0x0c:o + 0x2c].split(b'\0')[0].decode('latin-1')), d[o:o + 0x334])


def collect_nms():
    for p in glob.glob(os.path.join(GAME, '**', '*.nms'), recursive=True):
        try:
            collect_nms_file(p)
        except struct.error as e:
            print('skip', p, e, file=sys.stderr)


def collect_nms_file(p):
    if True:
        d = open(p, 'rb').read()
        src = os.path.relpath(p, GAME)
        add('NmsHeader', src, d[:0x1210])
        n = u32(d, 0x204)
        if not 0 < n < 4096 or 0x1210 + n * 0x7f0 > len(d):
            return
        objs = [d[0x1210 + i * 0x7f0:0x1210 + (i + 1) * 0x7f0] for i in range(n)]
        for o in objs:
            add('NmsObject', src, o)
        q = 0x1210 + n * 0x7f0
        for o in objs:
            for sl in range(10):
                if u32(o, 0x3c0 + sl * 4):
                    cnt = u32(d, q + 4)
                    if q + 0x20 + cnt * 16 > len(d):
                        raise struct.error('curve runs past the end')
                    add('NmsCurveHeader', src, d[q:q + 0x20])
                    q += 0x20 + cnt * 16
        tracks = u32(d, q)
        q += 4
        for _ in range(tracks):
            if q + 0xa4 > len(d):
                break
            add('NmsTrack', src, d[q:q + 0xa4])
            cnt = u32(d, q + 0x44)
            if q + 0xa4 + cnt * 0x20 > len(d):
                raise struct.error('track runs past the end')
            for k in range(cnt):
                add('NmsSegment', src, d[q + 0xa4 + k * 0x20:q + 0xa4 + (k + 1) * 0x20])
            q += 0xa4 + cnt * 0x20


def describe(v):
    f = struct.unpack('<f', struct.pack('<I', v))[0]
    fs = ('%g' % f) if v and 1e-6 < abs(f) < 1e7 else ''
    return '%x%s' % (v, ('/' + fs) if fs else '')


def report(name):
    recs = samples[name]
    size = max(len(r) for _, r in recs)
    print('\n== %s: %d records from %d files, %#x bytes' % (name, len(recs), len({s for s, _ in recs}), size))
    for off in range(0, size - 3, 4):
        vals = Counter(u32(r, off) for _, r in recs if off + 4 <= len(r))
        nz = sum(c for v, c in vals.items() if v)
        if not nz:
            continue
        top = ', '.join('%s x%d' % (describe(v), c) for v, c in vals.most_common(5))
        first = next((s for s, r in recs if off + 4 <= len(r) and u32(r, off)), '')
        print('  +%03x nonzero %5d distinct %5d  %s   e.g. %s' % (off, nz, len(vals), top, first))


def main():
    for p in glob.glob(os.path.join(GAME, 'data', '**', '*.cmm'), recursive=True):
        collect_cmm(p)
    collect_tracks()
    collect_weapons()
    collect_nms()
    wanted = sys.argv[1:] or sorted(samples)
    for name in sorted(samples):
        if any(name.startswith(w) for w in wanted):
            report(name)


if __name__ == '__main__':
    main()
