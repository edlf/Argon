# Ghidra Jython: find functions that use a struct field, by memory-operand displacement, keeping only functions that
# also use enough of the struct's other ("anchor") fields, so unrelated structs with the same offset drop out.
# usage: -postScript find_field_users.py <out> <targets hex+hex> <anchors hex+hex> <min anchors>
import re

args = getScriptArgs()
out = open(args[0], 'w')
targets = set(int(v, 16) for v in args[1].split('+'))
anchors = set(int(v, 16) for v in args[2].split('+'))
min_anchors = int(args[3])
disp_re = re.compile(r'\+ 0x([0-9a-f]+)\]')
fm = currentProgram.getFunctionManager()

uses = {}   # function -> {disp: [instruction text]}
for ins in currentProgram.getListing().getInstructions(True):
    text = str(ins)
    for m in disp_re.finditer(text):
        d = int(m.group(1), 16)
        if d in targets or d in anchors:
            f = fm.getFunctionContaining(ins.getAddress())
            if f is None:
                continue
            uses.setdefault(f, {}).setdefault(d, []).append('%s  %s' % (ins.getAddress(), text))

for f in sorted(uses, key=lambda g: g.getEntryPoint().getOffset()):
    found = uses[f]
    hit = sorted(d for d in found if d in targets)
    anchored = sorted(d for d in found if d in anchors and d not in targets)
    if not hit or len(anchored) < min_anchors:
        continue
    out.write('%s @ %s  targets %s  anchors %s\n' % (f.getName(), f.getEntryPoint(), ' '.join('%x' % d for d in hit),
                                                    ' '.join('%x' % d for d in anchored)))
    for d in hit:
        for line in found[d][:4]:
            out.write('    %s\n' % line)
out.close()
