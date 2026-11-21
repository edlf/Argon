"""List strings in game_data\\default.xbe (the original USA XBE) matching a regex, with the code locations that reference them.
usage: python -I xbe_strrefs.py <regex> [extra hex addresses to find references to...]
"""
import os, re, struct, sys

here = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(here, '..', 'xbe'))
import xbe as xbe_file  # tools\xbe\xbe.py

d = open(os.path.join(here, '..', '..', 'game_data', 'default.xbe'), 'rb').read()


def va(fo):
    return xbe_file.off2va(d, fo)


def refs(a):
    b = struct.pack('<I', a)
    out, j = [], d.find(b)
    while j >= 0:
        out.append(hex(va(j) or 0))
        j = d.find(b, j + 1)
    return out


pat = re.compile(sys.argv[1].encode())
for m in re.finditer(rb'[ -~]{3,}', d):
    if pat.search(m.group()):
        a = va(m.start())
        if a:
            print(hex(a), m.group()[:80], refs(a))
for x in sys.argv[2:]:
    a = int(x, 16)
    print(hex(a), 'refs', refs(a))
