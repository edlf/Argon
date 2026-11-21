# Ghidra Jython: for each given address inside undefined code, walk backwards to the
# nearest address that is a CALL target, create a function there, and print its entry.
# usage: -postScript make_func.py <addr>...
from ghidra.program.model.symbol import RefType
fm = currentProgram.getFunctionManager()
for it in getScriptArgs():
    if it.startswith('@'):                 # exact entry point given
        a = toAddr(int(it[1:], 16))
        disassemble(a)
        print('MAKEFUNC %s -> %s' % (it, fm.getFunctionAt(a) or createFunction(a, None)))
        continue
    a = toAddr(int(it, 16))
    if fm.getFunctionContaining(a):
        print('MAKEFUNC %s already in %s' % (it, fm.getFunctionContaining(a)))
        continue
    cur, found = a, None
    for i in range(0x2000):
        cur = cur.subtract(1)
        if any(r.getReferenceType().isCall() for r in getReferencesTo(cur)):
            found = cur
            break
        if fm.getFunctionContaining(cur):
            break
    if found is None:
        print('MAKEFUNC %s: no call target found' % it)
        continue
    disassemble(found)
    f = createFunction(found, None)
    print('MAKEFUNC %s -> %s %s' % (it, found, f))
