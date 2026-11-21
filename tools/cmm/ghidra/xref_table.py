# Ghidra Jython: list code that reads each dword of a global table, with the
# decompiled line that uses it.  usage: -postScript xref_table.py <out> <base_hex> <size_hex>
from ghidra.app.decompiler import DecompInterface
args = getScriptArgs()
out = open(args[0], 'w')
base, size = int(args[1], 16), int(args[2], 16)
fm = currentProgram.getFunctionManager()
dec = DecompInterface(); dec.openProgram(currentProgram)
cache = {}
for off in range(0, size, 4):
    a = toAddr(base + off)
    refs = list(getReferencesTo(a))
    if not refs:
        continue
    out.write('+%03x (%s): %d refs\n' % (off, a, len(refs)))
    seen = set()
    for r in refs[:6]:
        f = fm.getFunctionContaining(r.getFromAddress())
        if f is None or f.getEntryPoint() in seen:
            continue
        seen.add(f.getEntryPoint())
        if f.getEntryPoint() not in cache:
            res = dec.decompileFunction(f, 60, monitor)
            cache[f.getEntryPoint()] = res.getDecompiledFunction().getC() if res.decompileCompleted() else ''
        sym = getSymbolAt(a)
        key = sym.getName() if sym else ('%08x' % (base + off))
        lines = [l.strip() for l in cache[f.getEntryPoint()].split('\n') if key[-6:] in l][:2]
        out.write('    %s: %s\n' % (f.getName(), ' | '.join(lines)[:220]))
out.close()
