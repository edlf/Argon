# Ghidra Jython: print the address and the stored float / int of named symbols.
# usage: -postScript sym_values.py <outfile> <name>...
args = getScriptArgs()
out = open(args[0], 'w')
st = currentProgram.getSymbolTable()
for n in args[1:]:
    for s in st.getSymbols(n):
        a = s.getAddress()
        try:
            iv = getInt(a)
            fv = getFloat(a)
            out.write('%s %s int 0x%x float %r\n' % (n, a, iv & 0xffffffff, fv))
        except Exception as e:
            out.write('%s %s (%s)\n' % (n, a, e))
out.close()
