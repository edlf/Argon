# Ghidra Jython: list references to given addresses (with the referencing instruction and function).
# usage: -postScript refs_to.py <out> <hex>...
args = getScriptArgs()
out = open(args[0], 'w')
fm = currentProgram.getFunctionManager()
lst = currentProgram.getListing()
for a in args[1:]:
    ad = toAddr(int(a, 16))
    out.write('%s\n' % a)
    for r in getReferencesTo(ad):
        fa = r.getFromAddress()
        f = fm.getFunctionContaining(fa)
        ins = lst.getInstructionAt(fa)
        out.write('    %s %s %s  %s\n' % (fa, r.getReferenceType(), f.getName() if f else '-', ins))
out.close()
