# Ghidra Jython: list instructions using given scalar operands, grouped by function.
# usage: -postScript find_scalar.py <out> <hex>+<hex>...
from ghidra.program.model.scalar import Scalar
args = getScriptArgs()
out = open(args[0], 'w')
vals = set(int(v, 16) for v in args[1].split('+'))
fm = currentProgram.getFunctionManager()
hits = {}
for ins in currentProgram.getListing().getInstructions(True):
    for i in range(ins.getNumOperands()):
        for o in ins.getOpObjects(i):
            if isinstance(o, Scalar) and (o.getUnsignedValue() & 0xffffffff) in vals:
                f = fm.getFunctionContaining(ins.getAddress())
                hits.setdefault(f.getName() + '@' + str(f.getEntryPoint()) if f else 'nofunc', []).append('%s  %s' % (ins.getAddress(), ins))
for k in sorted(hits):
    out.write('%s\n' % k)
    for l in hits[k][:8]: out.write('    %s\n' % l)
out.close()
