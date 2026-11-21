# Ghidra Jython: decompile functions that reference given strings, or given addresses.
# usage: -postScript decomp_refs.py <outfile> <item>...   item = "s:<string>" or hex address
from ghidra.app.decompiler import DecompInterface

args = getScriptArgs()
out = open(args[0], 'w')
dec = DecompInterface(); dec.openProgram(currentProgram)
fm = currentProgram.getFunctionManager()
targets = []
timeout = 90
for it in args[1:]:
    if it.startswith('t:'):               # decompile timeout in seconds
        timeout = int(it[2:]); continue
    if it.startswith('s:') or it.startswith('r:'):
        if it.startswith('r:'):            # references to a data address
            a = toAddr(int(it[2:], 16)); s = it
        else:
            s = it[2:].replace('\\\\', '\\')
            a = find(None, s)
        out.write('// target %r at %s\n' % (s, a))
        if a is None: continue
        for r in getReferencesTo(a):
            f = fm.getFunctionContaining(r.getFromAddress())
            out.write('//   ref %s in %s\n' % (r.getFromAddress(), f))
            if f: targets.append(f)
    elif it.startswith('v:'):              # vtable: decompile first 8 entries
        va = toAddr(int(it[2:], 16))
        for k in range(24):
            p = toAddr(getInt(va.add(k * 4)) & 0xffffffff)
            f = fm.getFunctionAt(p) or fm.getFunctionContaining(p)
            if f is None:
                disassemble(p); f = createFunction(p, None)
            out.write('// vtable %s[%d] -> %s %s\n' % (it, k, p, f))
            if f: targets.append(f)
    elif it.startswith('c:'):              # callers of a function (2 levels)
        f0 = fm.getFunctionContaining(toAddr(int(it[2:], 16)))
        level = [f0]
        for depth in range(2):
            nxt = []
            for g in level:
                for r in getReferencesTo(g.getEntryPoint()):
                    cf = fm.getFunctionContaining(r.getFromAddress())
                    out.write('// %s caller of %s: %s (%s)\n' % ('  ' * depth, g.getName(), cf, r.getFromAddress()))
                    if cf and depth == 0: targets.append(cf); nxt.append(cf)
            level = nxt
    else:
        f = fm.getFunctionContaining(toAddr(int(it, 16)))
        if f: targets.append(f)
seen = set()
for f in targets:
    if f.getEntryPoint() in seen: continue
    seen.add(f.getEntryPoint())
    r = dec.decompileFunction(f, timeout, monitor)
    out.write('\n// ===== %s @ %s =====\n' % (f.getName(), f.getEntryPoint()))
    out.write(r.getDecompiledFunction().getC() if r.decompileCompleted() else '// failed: %s\n' % r.getErrorMessage())
    out.write('// callees: %s\n' % ', '.join('%s@%s' % (c.getName(), c.getEntryPoint())
                                          for c in f.getCalledFunctions(monitor)))
    out.flush()
out.close()
