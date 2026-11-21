# Extract character_stats key -> address from the decompiled parser (out_stats.c)
import re, sys
src = open('out_stats.c').read()
keys = [(m.start(), m.group(1)) for m in re.finditer(r'pcVar11 = "([A-Za-z0-9_/ ]+)";', src)]
res = []
for i, (pos, k) in enumerate(keys):
    end = keys[i + 1][0] if i + 1 < len(keys) else len(src)
    body = src[pos:end]
    m = re.search(r'(?:iVar\d+|puVar\d+|pfVar\d+|pAVar\d+) = (?:\(\w+ \*\))?&?(?:DAT_|0x)([0-9a-fA-F]{5,8})', body)
    stride = re.search(r'\+ (0x3a4|0xe9)', body)
    res.append((k, int(m.group(1), 16) if m else None, stride.group(1) if stride else '-'))
addrs = [a for _, a, _ in res if a]
base = min(addrs)
print('base %x' % base)
for k, a, s in res:
    print('%-36s %s %s' % (k, ('+0x%03x' % (a - base)) if a else '?', s))
