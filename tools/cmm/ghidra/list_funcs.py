# Ghidra Jython: list functions whose entry lies in [lo, hi). usage: -postScript list_funcs.py <out> <lo hex> <hi hex>
args = getScriptArgs()
out = open(args[0], 'w')
lo, hi = int(args[1], 16), int(args[2], 16)
for f in currentProgram.getFunctionManager().getFunctions(toAddr(lo), True):
    a = f.getEntryPoint().getOffset()
    if a >= hi: break
    out.write('%08x %5d %s\n' % (a, f.getBody().getNumAddresses(), f.getName()))
out.close()
