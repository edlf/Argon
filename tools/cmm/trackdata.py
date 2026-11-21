"""Read/write Quantum Redshift per-track data (everything except the .cmm model).

Binary (data\\tracks\\<t>\\):   locator.dat (start grid), sprite.dat (glow quads),
                                prop.dat (fog / lighting)
Text   (config\\):              ai_<t>.txt (AI node graph), <t>pup.txt (pickups),
                                camera_<t>.txt (replay/attract cameras)

All positions are game coordinates (D3D left-handed, Y up).
Writers patch values in place, so untouched bytes/lines stay identical.
Run `python -I trackdata.py <track>` for a summary + round-trip self-test.
"""
import math, os, re, struct

HERE = os.path.dirname(os.path.abspath(__file__))
GAME = os.path.normpath(os.path.join(HERE, '..', '..', 'game_data'))


def track_dir(t):
    return os.path.join(GAME, 'data', 'tracks', t)


def config_path(kind, t):
    name = {'ai': 'ai_%s.txt', 'pup': '%spup.txt', 'camera': 'camera_%s.txt'}[kind] % t
    return os.path.join(GAME, 'config', name)


# --------------------------------------------------------------- locator.dat
# header 0x10: u32 count, stale ptr, 0, 0.  entries 0xc0 (parsed by FUN_000af270):
#   +0x00 char name[0x40]
#   +0x40 float m[4][4]   world matrix, D3D row-vector (row 3 = position)
#   +0x80 u32 0
#   +0x84 float pos[3]    \  the same placement as editable values;
#   +0x90 float rot[3]    |  rot = degrees (x, y, z); starts only use y (yaw)
#   +0x9c float scale[3]  /
LOC_HDR, LOC_SIZE = 0x10, 0xc0


class Locator:
    def __init__(self, name, matrix, pos, rot, scale):
        self.name, self.matrix, self.pos, self.rot, self.scale = name, matrix, pos, rot, scale


def read_locators(t):
    d = open(os.path.join(track_dir(t), 'locator.dat'), 'rb').read()
    n = struct.unpack_from('<I', d, 0)[0]
    out = []
    for i in range(n):
        o = LOC_HDR + i * LOC_SIZE
        name = d[o:o + 0x40].split(b'\0')[0].decode('latin-1')
        m = [list(struct.unpack_from('<4f', d, o + 0x40 + r * 16)) for r in range(4)]
        out.append(Locator(name, m, list(struct.unpack_from('<3f', d, o + 0x84)),
                           list(struct.unpack_from('<3f', d, o + 0x90)),
                           list(struct.unpack_from('<3f', d, o + 0x9c))))
    return out


def locator_matrix(pos, rot, scale):
    """Scale * Rx * Ry * Rz * Translation (D3D row-vector), angles in degrees.
    Order found by matching the tilted alt_mars starts; exact on all tracks."""
    p, y, r = (math.radians(a) for a in rot)   # rot = (x=pitch, y=yaw, z=roll)
    cy, sy, cp, sp, cr, sr = math.cos(y), math.sin(y), math.cos(p), math.sin(p), math.cos(r), math.sin(r)
    Rz = [[cr, sr, 0], [-sr, cr, 0], [0, 0, 1]]
    Rx = [[1, 0, 0], [0, cp, sp], [0, -sp, cp]]
    Ry = [[cy, 0, -sy], [0, 1, 0], [sy, 0, cy]]
    mul = lambda a, b: [[sum(a[i][k] * b[k][j] for k in range(3)) for j in range(3)] for i in range(3)]
    R = mul(mul(Rx, Ry), Rz)
    rows = [[R[i][j] * scale[i] for j in range(3)] + [0.0] for i in range(3)]
    return rows + [list(pos) + [1.0]]


def write_locators(t, locs, path=None):
    src = os.path.join(track_dir(t), 'locator.dat')
    d = bytearray(open(src, 'rb').read())
    for i, L in enumerate(locs):
        o = LOC_HDR + i * LOC_SIZE
        old = struct.unpack_from('<9f', d, o + 0x84)
        if tuple(L.pos) + tuple(L.rot) + tuple(L.scale) != old:   # only rebuild if changed
            L.matrix = locator_matrix(L.pos, L.rot, L.scale)
        for r in range(4):
            struct.pack_into('<4f', d, o + 0x40 + r * 16, *L.matrix[r])
        struct.pack_into('<9f', d, o + 0x84, *(list(L.pos) + list(L.rot) + list(L.scale)))
    open(path or src, 'wb').write(d)


# ---------------------------------------------------------------- sprite.dat
# header 0x18: u32 count, stale ptr, 4 more dwords. entries 0x84
# (quadsprite_AddCMMSprites 0x000bdf40):
#   +0x00 s16 facing (x3 = the vertex shader's a0): 0 world, 1 upright, 2 camera
#   +0x04 u32 flags   bit0 = additive blend (else alpha blend)
#   +0x08 float pos[3]               sprite centre
#   +0x14 / +0x20 / +0x2c / +0x38    float[3] corner offsets v3, v2, v0, v1
#   +0x44..+0x50 u32[4]              D3DCOLOR v0..v3
#   +0x54..+0x70 float[8]            corner UVs, v x 0.5 + 0.5 (v0 u=+54 v=+70, v1 u=+5c v=+68,
#                                     v2 u=+6c v=+58, v3 u=+64 v=+60)
SPR_HDR, SPR_SIZE = 0x18, 0x84


def read_sprites(t):
    d = open(os.path.join(track_dir(t), 'sprite.dat'), 'rb').read()
    n = struct.unpack_from('<I', d, 0)[0]
    out = []
    for i in range(n):
        o = SPR_HDR + i * SPR_SIZE
        out.append({'frame': struct.unpack_from('<h', d, o)[0],
                    'additive': bool(struct.unpack_from('<I', d, o + 4)[0] & 1),
                    'pos': list(struct.unpack_from('<3f', d, o + 8)),
                    'corners': [list(struct.unpack_from('<3f', d, o + k)) for k in (0x2c, 0x38, 0x20, 0x14)]})
    return out


def write_sprites(t, sprites, path=None):
    src = os.path.join(track_dir(t), 'sprite.dat')
    d = bytearray(open(src, 'rb').read())
    for i, s in enumerate(sprites):
        o = SPR_HDR + i * SPR_SIZE
        struct.pack_into('<3f', d, o + 8, *s['pos'])
    open(path or src, 'wb').write(d)


# ------------------------------------------------------------------ prop.dat
# Copied whole into a global table at 0x390fa8 (track_load_prop_dat 0x000b03f0).
# Field meanings from the code that reads each global (see FORMAT notes).
PROP_FIELDS = [  # (offset, struct fmt, name, confirmed?)
    (0x000, '<3i', 'fog_colour_rgb', True),          # packed 0xffRRGGBB for fog
    (0x00c, '<f', 'fog_density', True),               # neon_setup_fog arg
    (0x010, '<3i', 'colour2_rgb', False),             # default 0x40,0x40,0x40
    (0x01c, '<2i', 'flags_1c', False),
    (0x024, '64s', 'track_name', True),
    (0x13c, '<i', 'unk_13c', False),                  # 0 / 20 / 50
    (0x140, '<i', 'unk_140_distance?', False),        # 500..40000
    (0x144, '<i', 'unk_144_fov?', False),             # 60 / 70
    (0x148, '<f', 'unk_148', False),                  # 0..4.9
    (0x14c, '<3i', 'ambient_rgb', True),              # mixed into ship lighting
    (0x158, '<3i', 'sun_colour_rgb', True),
    (0x164, '<3f', 'sun_vector', True),               # normalised by the game
    (0x170, '<i', 'fog_mode', True),                  # 1 / 3
    (0x174, '<f', 'fog_range', True),                 # neon_setup_fog arg
]


def read_prop(t):
    d = open(os.path.join(track_dir(t), 'prop.dat'), 'rb').read()
    out = {}
    for off, fmt, name, _ in PROP_FIELDS:
        v = struct.unpack_from(fmt, d, off)
        if fmt.endswith('s'):
            v = v[0].split(b'\0')[0].decode('latin-1')
        elif len(v) == 1:
            v = v[0]
        else:
            v = list(v)
        out[name] = v
    return out


def write_prop(t, props, path=None):
    src = os.path.join(track_dir(t), 'prop.dat')
    d = bytearray(open(src, 'rb').read())
    for off, fmt, name, _ in PROP_FIELDS:
        if name not in props:
            continue
        v = props[name]
        if fmt.endswith('s'):
            if d[off:off + 64].split(b'\0')[0].decode('latin-1') == v:
                continue                         # keep leftover bytes after the NUL
            b = v.encode('latin-1')[:63]
            d[off:off + 64] = b + b'\0' * (64 - len(b))
        else:
            struct.pack_into(fmt, d, off, *(v if isinstance(v, (list, tuple)) else [v]))
    open(path or src, 'wb').write(d)


# ------------------------------------------------------------- ai_<t>.txt
# <nodes> list of <node> (id, pos, down, forward*/backward*, flags), <hulls>
# with per-node transplanes (= half vector to a neighbour node + its id) and
# boundaryplanes (track edges, offsets relative to the node), <routes>.
NODE_RE = re.compile(r'(<node>\s*<id=(\d+)>\s*<pos=)([^>]*)(>)(.*?)</node>', re.S)


def read_ai(t):
    txt = open(config_path('ai', t), newline='').read()
    nodes = {}
    for m in NODE_RE.finditer(txt):
        body = m.group(5)
        nodes[int(m.group(2))] = {
            'pos': [float(x) for x in m.group(3).split()],
            'forward': [int(x) for x in re.findall(r'<forward\d+=(\d+)>', body)],
            'backward': [int(x) for x in re.findall(r'<backward\d+=(\d+)>', body)],
            'flags': re.findall(r'<(flag_\w+|corner_\w+|turbo_\w+|env_\w+|reverb\w*)=true>', body),
        }
    return nodes


def _fmt(v, nd):
    s = '%.*f' % (nd, v)
    return '0.%s' % ('0' * nd) if s in ('-0.' + '0' * nd,) else s


def write_ai(t, nodes, path=None):
    """Write node positions back and recompute hull transplanes for moved nodes."""
    src = config_path('ai', t)
    txt = open(src, newline='').read()
    old = read_ai(t)
    moved = {i for i in nodes if [round(a, 2) for a in nodes[i]['pos']] != [round(a, 2) for a in old[i]['pos']]}

    def node_sub(m):
        i = int(m.group(2))
        if i not in moved:
            return m.group(0)
        return m.group(1) + ' '.join(_fmt(v, 2) for v in nodes[i]['pos']) + m.group(4) + m.group(5) + '</node>'
    txt = NODE_RE.sub(node_sub, txt)

    if moved:
        a, b = txt.index('<hulls>'), txt.index('</hulls>')
        def hull_sub(m):
            body = m.group(0)
            hid = int(re.search(r'<id=(\d+)>', body).group(1))
            def tp_sub(tm):
                nb = int(tm.group(2).split()[3])
                if hid not in moved and nb not in moved:
                    return tm.group(0)
                p, q = nodes[hid]['pos'], nodes[nb]['pos']
                half = ['%.6f' % ((q[k] - p[k]) / 2) for k in range(3)]
                return '%s%s %d>' % (tm.group(1), ' '.join(half), nb)
            return re.sub(r'(<transplane\d+=)([^>]*)>', tp_sub, body)
        txt = txt[:a] + re.sub(r'<hull>.*?</hull>', hull_sub, txt[a:b], flags=re.S) + txt[b:]
    open(path or src, 'w', newline='').write(txt)
    return moved


# ------------------------------------------------------------- <t>pup.txt
PUP_RE = re.compile(r'PUPSTART\r?\n(\w+)\r?\nX ([-\d.]+)\r?\nY ([-\d.]+)\r?\nZ ([-\d.]+)\r?\nPUPEND')


def read_pups(t):
    txt = open(config_path('pup', t), newline='').read()
    return [{'type': m.group(1), 'pos': [float(m.group(k)) for k in (2, 3, 4)]} for m in PUP_RE.finditer(txt)]


def write_pups(t, pups, path=None):
    src = config_path('pup', t)
    txt = open(src, newline='').read()
    it = iter(pups)
    def sub(m):
        p = next(it)
        s = m.group(0)
        if [round(v, 4) for v in p['pos']] == [round(float(m.group(k)), 4) for k in (2, 3, 4)] and p['type'] == m.group(1):
            return s
        nl = '\r\n' if '\r\n' in s else '\n'
        return nl.join(['PUPSTART', p['type'], 'X %.6f' % p['pos'][0], 'Y %.6f' % p['pos'][1],
                        'Z %.6f' % p['pos'][2], 'PUPEND'])
    open(path or src, 'w', newline='').write(PUP_RE.sub(sub, txt))


# ---------------------------------------------------------- camera_<t>.txt
CAM_RE = re.compile(r'<camera>(.*?)</camera>', re.S)


def read_cameras(t):
    txt = open(config_path('camera', t), newline='').read()
    out = []
    for m in CAM_RE.finditer(txt):
        b = m.group(1)
        g = lambda k: (re.search(r'<%s=([^>]*)>' % k, b) or [None, None])[1]
        out.append({'name': g('name'), 'type': g('type'),
                    'pos': [float(x) for x in g('pos').split()] if g('pos') else None,
                    'target': [float(x) for x in g('target').split()] if g('target') else None,
                    'fov': float(g('fov')) if g('fov') else None})
    return out


def write_cameras(t, cams, path=None):
    src = config_path('camera', t)
    txt = open(src, newline='').read()
    it = iter(cams)
    def sub(m):
        c = next(it); b = m.group(1)
        for k in ('pos', 'target'):
            if c.get(k) is None:
                continue
            cur = re.search(r'<%s=([^>]*)>' % k, b)
            if cur and [round(float(x), 4) for x in cur.group(1).split()] != [round(v, 4) for v in c[k]]:
                b = b.replace(cur.group(0), '<%s=%s>' % (k, ' '.join('%.6f' % v for v in c[k])))
        return '<camera>' + b + '</camera>'
    open(path or src, 'w', newline='').write(CAM_RE.sub(sub, txt))


# ------------------------------------------------------------------- test
def _selftest(t, tmp):
    """Read + write unchanged must give byte-identical files."""
    import filecmp
    ok = True
    jobs = [('locator.dat', lambda p: write_locators(t, read_locators(t), p), os.path.join(track_dir(t), 'locator.dat')),
            ('sprite.dat', lambda p: write_sprites(t, read_sprites(t), p), os.path.join(track_dir(t), 'sprite.dat')),
            ('prop.dat', lambda p: write_prop(t, read_prop(t), p), os.path.join(track_dir(t), 'prop.dat'))]
    for kind, w, r in (('ai', write_ai, read_ai), ('pup', write_pups, read_pups), ('camera', write_cameras, read_cameras)):
        if os.path.exists(config_path(kind, t)):
            jobs.append((os.path.basename(config_path(kind, t)), (lambda w, r: lambda p: w(t, r(t), p))(w, r), config_path(kind, t)))
    for name, fn, orig in jobs:
        if not os.path.exists(orig):
            continue
        out = os.path.join(tmp, name)
        fn(out)
        same = filecmp.cmp(orig, out, shallow=False)
        ok &= same
        print('  round-trip %-22s %s' % (name, 'identical' if same else 'DIFFERENT'))
    # matrix builder must reproduce shipped start matrices
    worst = max(abs(a - b) for L in read_locators(t)
                for ra, rb in zip(L.matrix, locator_matrix(L.pos, L.rot, L.scale)) for a, b in zip(ra, rb))
    print('  locator matrix rebuild max error %.5f' % worst)
    return ok and worst < 1e-3


if __name__ == '__main__':
    import sys, tempfile
    t = sys.argv[1] if len(sys.argv) > 1 else 'bracken'
    print(t, '| starts:', len(read_locators(t)), '| sprites:', len(read_sprites(t)),
          '| ai nodes:', len(read_ai(t)) if os.path.exists(config_path('ai', t)) else '-',
          '| pickups:', len(read_pups(t)) if os.path.exists(config_path('pup', t)) else '-',
          '| cameras:', len(read_cameras(t)) if os.path.exists(config_path('camera', t)) else '-')
    for k, v in read_prop(t).items():
        print('  prop %-24s %s' % (k, v))
    with tempfile.TemporaryDirectory() as tmp:
        print('selftest', 'PASS' if _selftest(t, tmp) else 'FAIL')
