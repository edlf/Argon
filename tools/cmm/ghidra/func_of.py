# Ghidra Jython: print the function containing each address (hex args); out file first.
args = getScriptArgs()
out = open(args[0], 'w')
fm = currentProgram.getFunctionManager()
for a in args[1:]:
    f = fm.getFunctionContaining(toAddr(int(a, 16)))
    out.write('%s %s %s\n' % (a, f, f.getEntryPoint() if f else ''))
out.close()
