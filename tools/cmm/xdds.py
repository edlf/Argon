"""Decode Quantum Redshift (Xbox) .dds textures to RGBA and write PNG.

The game's DDS files use a normal 128-byte DDS header, but the fourCC slot holds an
Xbox D3DFORMAT number instead of a FourCC, and uncompressed formats are swizzled.
Only mip level 0 is decoded.
"""
import struct, zlib

X_DXT1, X_DXT3, X_DXT5 = 0x0C, 0x0E, 0x0F


def read_header(data):
    if data[:4] != b'DDS ':
        raise ValueError('not a DDS file')
    h, w = struct.unpack_from('<2I', data, 12)
    mips = struct.unpack_from('<I', data, 28)[0]
    pf_flags = struct.unpack_from('<I', data, 80)[0]
    fmt = struct.unpack_from('<I', data, 84)[0]
    bits, rm, gm, bm, am = struct.unpack_from('<5I', data, 88)
    return dict(w=w, h=h, mips=mips, pf_flags=pf_flags, fmt=fmt, bits=bits,
                masks=(rm, gm, bm, am))


def _c565(c):
    return (((c >> 11) & 31) * 255 // 31, ((c >> 5) & 63) * 255 // 63, (c & 31) * 255 // 31)


def _dxt_color(blk, o, out, w, x0, y0, opaque4):
    c0, c1, bits = struct.unpack_from('<HHI', blk, o)
    p0, p1 = _c565(c0), _c565(c1)
    if c0 > c1 or opaque4:
        pal = [p0 + (255,), p1 + (255,),
               tuple((2 * a + b) // 3 for a, b in zip(p0, p1)) + (255,),
               tuple((a + 2 * b) // 3 for a, b in zip(p0, p1)) + (255,)]
    else:
        pal = [p0 + (255,), p1 + (255,),
               tuple((a + b) // 2 for a, b in zip(p0, p1)) + (255,), (0, 0, 0, 0)]
    for i in range(16):
        px = pal[(bits >> (2 * i)) & 3]
        j = ((y0 + i // 4) * w + x0 + i % 4) * 4
        out[j:j + 4] = bytes(px)


def _decode_dxt(data, w, h, fmt):
    out = bytearray(w * h * 4)
    bsize = 8 if fmt == X_DXT1 else 16
    o = 128
    for by in range(0, max(h, 4), 4):
        for bx in range(0, max(w, 4), 4):
            if fmt == X_DXT1:
                _dxt_color(data, o, out, w, bx, by, False)
            else:
                _dxt_color(data, o + 8, out, w, bx, by, True)
                if fmt == X_DXT3:
                    a = struct.unpack_from('<Q', data, o)[0]
                    alphas = [((a >> (4 * i)) & 15) * 17 for i in range(16)]
                else:
                    a0, a1 = data[o], data[o + 1]
                    ab = int.from_bytes(data[o + 2:o + 8], 'little')
                    if a0 > a1:
                        pal = [a0, a1] + [((6 - k) * a0 + (k + 1) * a1) // 7 for k in range(6)]
                    else:
                        pal = [a0, a1] + [((4 - k) * a0 + (k + 1) * a1) // 5 for k in range(4)] + [0, 255]
                    alphas = [pal[(ab >> (3 * i)) & 7] for i in range(16)]
                for i in range(16):
                    out[((by + i // 4) * w + bx + i % 4) * 4 + 3] = alphas[i]
            o += bsize
    return out


def _unswizzle_index(w, h):
    """Map linear (x,y) -> swizzled texel index for Xbox Morton-ordered textures."""
    xs, ys = [], []
    bit = 0
    mw, mh = w - 1, h - 1
    xbit = ybit = 1
    while xbit <= mw or ybit <= mh:
        if xbit <= mw:
            xs.append(1 << bit); bit += 1; xbit <<= 1
        else:
            xs.append(0); xbit <<= 1
        if ybit <= mh:
            ys.append(1 << bit); bit += 1; ybit <<= 1
        else:
            ys.append(0); ybit <<= 1
    def spread(v, table):
        r, k = 0, 0
        while v:
            if v & 1:
                r |= table[k]
            v >>= 1; k += 1
        return r
    xt = [spread(x, xs) for x in range(w)]
    yt = [spread(y, ys) for y in range(h)]
    return xt, yt


def _decode_rgb(data, w, h, masks, bits=16, swizzled=False):
    # QR's .dds files on disc are stored linear (checked tex9 normal map); keep
    # the swizzle path for raw texture data pulled from memory/XPR resources.
    rm, gm, bm, am = masks
    def chan(v, m):
        if not m:
            return 255
        s = (m & -m).bit_length() - 1
        mx = m >> s
        return ((v & m) >> s) * 255 // mx
    if swizzled:
        xt, yt = _unswizzle_index(w, h)
    else:
        xt, yt = list(range(w)), [y * w for y in range(h)]
    src = struct.unpack_from('<%d%s' % (w * h, 'I' if bits == 32 else 'H'), data, 128)
    out = bytearray(w * h * 4)
    j = 0
    for y in range(h):
        yo = yt[y]
        for x in range(w):
            v = src[yo | xt[x]]
            out[j] = chan(v, rm); out[j + 1] = chan(v, gm); out[j + 2] = chan(v, bm)
            out[j + 3] = chan(v, am) if am else 255
            j += 4
    return out


def decode(path):
    """Return (w, h, rgba bytearray)."""
    data = open(path, 'rb').read()
    hd = read_header(data)
    w, h = hd['w'], hd['h']
    if hd['fmt'] in (X_DXT1, X_DXT3, X_DXT5):
        return w, h, _decode_dxt(data, w, h, hd['fmt'])
    if hd['bits'] in (16, 32):
        masks = hd['masks']
        if hd['bits'] == 32 and not any(masks[:3]):
            masks = (0xff0000, 0xff00, 0xff, 0xff000000)   # A8R8G8B8
        return w, h, _decode_rgb(data, w, h, masks, hd['bits'])
    raise ValueError('%s: unsupported format %#x / %d bits' % (path, hd['fmt'], hd['bits']))


def png_bytes(w, h, rgba):
    raw = b''.join(b'\0' + bytes(rgba[y * w * 4:(y + 1) * w * 4]) for y in range(h))
    def chunk(t, b):
        return struct.pack('>I', len(b)) + t + b + struct.pack('>I', zlib.crc32(t + b) & 0xffffffff)
    return (b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>2I5B', w, h, 8, 6, 0, 0, 0))
            + chunk(b'IDAT', zlib.compress(raw, 6)) + chunk(b'IEND', b''))


if __name__ == '__main__':
    import sys
    for p in sys.argv[1:]:
        w, h, px = decode(p)
        open(p[:-4] + '.png', 'wb').write(png_bytes(w, h, px))
        print(p, w, h)
