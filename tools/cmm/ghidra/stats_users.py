# For each decompiled function in a file, list character_stats fields read via this+0x1140
import re, sys
names = {}
for l in open(__import__('os').path.join(__import__('os').path.dirname(__file__) or '.', 'out_stats_offsets.txt')):
    p = l.split()
    if len(p) >= 2 and p[1].startswith('+0x'):
        names.setdefault(int(p[1], 16) + 0xc4, p[0])    # record starts 0xc4 before Shield_Strength
src = open(sys.argv[1]).read()
for blk in re.split(r'// ===== ', src)[1:]:
    fn = blk.split(' ')[0]
    offs = set()
    # direct: *(type *)(*(int *)((int)this + 0x1140) + 0xNNN)  or via a local holding the pointer
    for m in re.finditer(r'0x11(?:3c|40)\) \+ (0x[0-9a-f]+)\)', blk): offs.add(int(m.group(1), 16))
    locs = set(re.findall(r'(\w+) = \*\(\w+ \*\)\(\(int\)\w+ \+ 0x11(?:3c|40)\);', blk))
    for v in locs:
        for m in re.finditer(r'\(%s \+ (0x[0-9a-f]+)\)' % re.escape(v), blk): offs.add(int(m.group(1), 16))
    print(fn, ', '.join('%s(%x)' % (names.get(o, '?'), o) for o in sorted(offs)))
