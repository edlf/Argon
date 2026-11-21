# Ghidra Jython: list instructions that write [reg + <disp>] (MOV / FSTP / FST / FISTP to memory), grouped by function.
# usage: -postScript find_disp_write.py <out> <hex disp>
args = getScriptArgs()
out = open(args[0], 'w')
disp = '+ 0x%x]' % int(args[1], 16)
fm = currentProgram.getFunctionManager()
hits = {}
for ins in currentProgram.getListing().getInstructions(True):
    m = ins.getMnemonicString()
    if m not in ('MOV', 'FSTP', 'FST', 'FISTP', 'MOVSS'):
        continue
    if ins.getNumOperands() < 1:
        continue
    op0 = ins.getDefaultOperandRepresentation(0)
    if disp in op0:
        f = fm.getFunctionContaining(ins.getAddress())
        k = (f.getName() + '@' + str(f.getEntryPoint())) if f else 'nofunc'
        hits.setdefault(k, []).append('%s  %s' % (ins.getAddress(), ins))
for k in sorted(hits):
    out.write('%s\n' % k)
    for l in hits[k][:6]:
        out.write('    %s\n' % l)
out.close()
