# Ghidra Jython: list symbols whose name contains any of the given words (case-insensitive).
# usage: -postScript find_syms.py <outfile> <word>...
args = getScriptArgs()
out = open(args[0], 'w')
words = [w.lower() for w in args[1:]]
for s in currentProgram.getSymbolTable().getAllSymbols(True):
    n = s.getName().lower()
    if any(w in n for w in words):
        out.write('%s %s\n' % (s.getAddress(), s.getName()))
out.close()
