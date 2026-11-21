"""XBE helpers: VA <-> file offset, hexdump.  usage: python xbe.py <xbe> <va_hex> [len]"""
import struct, sys

def sections(data):
    base = struct.unpack_from('<I', data, 0x104)[0]
    n, hdr = struct.unpack_from('<II', data, 0x11C)
    out = []
    for i in range(n):
        o = hdr - base + i * 56
        flags, va, vsz, raw, rsz = struct.unpack_from('<IIIII', data, o)
        out.append((va, vsz, raw, rsz))
    return out

def va2off(data, va):
    for sva, vsz, raw, rsz in sections(data):
        if sva <= va < sva + rsz:
            return raw + va - sva
    return None

def off2va(data, off):
    for sva, vsz, raw, rsz in sections(data):
        if raw <= off < raw + rsz:
            return sva + off - raw
    return None

CERTIFICATE_SIGNATURE_KEY = 0xC0   # the certificate's 16-byte signature key (the save signing key is derived from it)


def certificate_signature_key(data):
    base = struct.unpack_from('<I', data, 0x104)[0]
    certificate = struct.unpack_from('<I', data, 0x118)[0] - base
    start = certificate + CERTIFICATE_SIGNATURE_KEY
    return data[start:start + 16]


CERTIFICATE_TITLE_ID = 0x08
RETAIL_THUNK_KEY = 0x5B6D40B6   # the header's kernel thunk address (+0x158) is XORed with it in retail XBEs
KERNEL_ORDINAL_FLAG = 0x80000000


def certificate_title_id(data):
    base = struct.unpack_from('<I', data, 0x104)[0]
    certificate = struct.unpack_from('<I', data, 0x118)[0] - base
    return struct.unpack_from('<I', data, certificate + CERTIFICATE_TITLE_ID)[0]


def kernel_thunk_table(data):
    return struct.unpack_from('<I', data, 0x158)[0] ^ RETAIL_THUNK_KEY


def kernel_import_slot(data, ordinal):
    """The VA of the thunk entry the kernel fills with the address of its export `ordinal` when it loads the XBE."""
    table = kernel_thunk_table(data)
    offset = va2off(data, table)
    if offset is None:
        raise ValueError('the kernel thunk table %#x is outside the XBE (not a retail XBE?)' % table)
    for index in range(0x200):
        entry = struct.unpack_from('<I', data, offset + 4 * index)[0]
        if entry == 0:
            break
        if entry == KERNEL_ORDINAL_FLAG | ordinal:
            return table + 4 * index
    raise ValueError('the XBE does not import kernel export %d' % ordinal)


if __name__ == '__main__':
    d = open(sys.argv[1], 'rb').read()
    va = int(sys.argv[2], 16)
    ln = int(sys.argv[3], 16) if len(sys.argv) > 3 else 0x40
    o = va2off(d, va)
    print('va %08x -> off %08x' % (va, o))
    for i in range(0, ln, 16):
        print('%08x  %s' % (va + i, d[o + i:o + i + 16].hex(' ')))
