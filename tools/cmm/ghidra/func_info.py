# Ghidra Jython: size / callers / callees of functions, and vtable dumps.
# usage: -postScript func_info.py <out> <hex>... | v:<hex>:<n>
args = getScriptArgs()
out = open(args[0], 'w')
fm = currentProgram.getFunctionManager()
def info(f):
    callers = sorted(set('%s' % (fm.getFunctionContaining(r.getFromAddress()) or r.getFromAddress()) for r in getReferencesTo(f.getEntryPoint())))
    return '%s @ %s  size %d  callers: %s\n   callees: %s' % (f.getName(), f.getEntryPoint(), f.getBody().getNumAddresses(),
        ', '.join(callers[:12]), ', '.join('%s(%d)' % (c.getName(), c.getBody().getNumAddresses()) for c in f.getCalledFunctions(monitor)))
for it in args[1:]:
    if it.startswith('v:'):
        _, a, n = it.split(':')
        va = int(a, 16)
        for k in range(int(n)):
            p = getInt(toAddr(va + 4 * k)) & 0xffffffff
            f = fm.getFunctionAt(toAddr(p))
            out.write('vt %x[%d] = %08x %s %s\n' % (va, k, p, f.getName() if f else '-', ('size %d' % f.getBody().getNumAddresses()) if f else ''))
    elif it.startswith('r:'):              # list functions in an address range
        _, lo, hi = it.split(':')
        for f in fm.getFunctions(toAddr(int(lo, 16)), True):
            if f.getEntryPoint().getOffset() > int(hi, 16): break
            out.write(info(f) + '\n')
    else:
        f = fm.getFunctionContaining(toAddr(int(it, 16)))
        out.write((info(f) if f else 'none %s' % it) + '\n')
out.close()
