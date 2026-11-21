# Ghidra Jython script: find functions referencing the model.cmm path string,
# decompile them (plus their callees) and write the C to an output file.
# usage: analyzeHeadless ... -postScript find_cmm_loader.py <outfile> [extra_func_addrs...]
from ghidra.app.decompiler import DecompInterface
from ghidra.program.model.symbol import RefType

args = getScriptArgs()
out = open(args[0], 'w')
extra = [toAddr(int(a, 16)) for a in args[1:]]

dec = DecompInterface()
dec.openProgram(currentProgram)
fm = currentProgram.getFunctionManager()

def decomp(f):
    r = dec.decompileFunction(f, 120, monitor)
    out.write('\n// ===== %s @ %s =====\n' % (f.getName(), f.getEntryPoint()))
    out.write(r.getDecompiledFunction().getC() if r.decompileCompleted() else '// failed\n')

done = set()
targets = []
for s in ['data\\%s\\model.cmm', 'data\\%s\\tex%d.dds', 'model %d: stripverts (%d faces)']:
    addr = find(None, s)
    out.write('// string %r at %s\n' % (s, addr))
    if addr is None:
        continue
    for ref in getReferencesTo(addr):
        f = fm.getFunctionContaining(ref.getFromAddress())
        out.write('//   ref from %s in %s\n' % (ref.getFromAddress(), f))
        if f: targets.append(f)
for a in extra:
    f = fm.getFunctionAt(a) or fm.getFunctionContaining(a)
    if f: targets.append(f)

for f in targets:
    if f.getEntryPoint() in done: continue
    done.add(f.getEntryPoint())
    decomp(f)
    out.write('// callees: %s\n' % ', '.join('%s@%s' % (c.getName(), c.getEntryPoint())
                                          for c in f.getCalledFunctions(monitor)))
out.close()
