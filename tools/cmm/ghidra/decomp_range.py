# Ghidra Jython: decompile every function in an address range into one file (for grepping field uses).
# usage: -postScript decomp_range.py <outfile> <lo hex> <hi hex> [timeout s]
from ghidra.app.decompiler import DecompInterface

args = getScriptArgs()
out = open(args[0], 'w')
lo, hi = int(args[1], 16), int(args[2], 16)
timeout = int(args[3]) if len(args) > 3 else 30
dec = DecompInterface()
dec.openProgram(currentProgram)
for f in currentProgram.getFunctionManager().getFunctions(toAddr(lo), True):
    if f.getEntryPoint().getOffset() >= hi:
        break
    r = dec.decompileFunction(f, timeout, monitor)
    out.write('\n// ===== %s @ %s =====\n' % (f.getName(), f.getEntryPoint()))
    out.write(r.getDecompiledFunction().getC() if r.decompileCompleted() else '// failed: %s\n' % r.getErrorMessage())
    out.flush()
out.close()
