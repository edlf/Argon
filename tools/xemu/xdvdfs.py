"""Minimal XDVDFS reader: find a file's byte offset/size in an Xbox ISO.
usage: python -I xdvdfs.py <iso> [path, e.g. default.xbe]"""
import sys, struct

MAGIC = b'MICROSOFT*XBOX*MEDIA'
SECTOR = 2048

def find_partition(f):
    for base in (0, 0x18300000, 0x2080000, 0xFD90000):
        f.seek(base + 32 * SECTOR)
        if f.read(20) == MAGIC:
            return base
    raise SystemExit('XDVDFS magic not found')

def read_dir(f, base, sector, size):
    f.seek(base + sector * SECTOR)
    data = f.read(size)
    out = {}
    def walk(off):
        if off * 4 + 14 > len(data):
            return
        l, r, st, sz, attr, nl = struct.unpack_from('<HHIIBB', data, off * 4)
        if l == 0xFFFF and r == 0xFFFF:
            return
        name = data[off * 4 + 14: off * 4 + 14 + nl].decode('latin-1')
        out[name.lower()] = (st, sz, attr)
        if l: walk(l)
        if r: walk(r)
    walk(0)
    return out

def locate(iso, path):
    with open(iso, 'rb') as f:
        base = find_partition(f)
        f.seek(base + 32 * SECTOR + 20)
        root_sec, root_sz = struct.unpack('<II', f.read(8))
        sec, sz = root_sec, root_sz
        parts = path.replace('\\', '/').split('/')
        for i, p in enumerate(parts):
            ents = read_dir(f, base, sec, sz)
            if p.lower() not in ents:
                raise SystemExit('not found: %s (have: %s)' % (p, sorted(ents)[:40]))
            sec, sz, attr = ents[p.lower()]
        return base + sec * SECTOR, sz

if __name__ == '__main__':
    off, sz = locate(sys.argv[1], sys.argv[2] if len(sys.argv) > 2 else 'default.xbe')
    print('offset %#x size %d' % (off, sz))
