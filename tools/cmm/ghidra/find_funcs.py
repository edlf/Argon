# Ghidra Jython: list functions whose name matches a regex, with their callers
# that lie inside an address range.  usage: -postScript find_funcs.py <out> <regex> [lo hi]
import re
args = getScriptArgs()
out = open(args[0], 'w')
rx = re.compile('|'.join(args[1].split('+')), re.I)   # '+' = alternative (| and , break the .bat)
lo = int(args[2], 16) if len(args) > 2 else 0
hi = int(args[3], 16) if len(args) > 3 else 0xffffffff
fm = currentProgram.getFunctionManager()
for f in fm.getFunctions(True):
    if not rx.search(f.getName()):
        continue
    callers = set()
    for r in getReferencesTo(f.getEntryPoint()):
        cf = fm.getFunctionContaining(r.getFromAddress())
        if cf and lo <= cf.getEntryPoint().getOffset() <= hi:
            callers.add('%s@%s' % (cf.getName(), cf.getEntryPoint()))
    out.write('%s @ %s  callers in range: %s\n' % (f.getName(), f.getEntryPoint(), ', '.join(sorted(callers))))
out.close()
