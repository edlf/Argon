# Ghidra Jython: list D3DDevice_SetRenderState_Simple(method, value) call sites with
# their constant arguments (from the PUSH instructions before each CALL).
# usage: -postScript rs_calls.py <out> <lo_hex> <hi_hex>
args = getScriptArgs()
out = open(args[0], 'w')
lo, hi = int(args[1], 16), int(args[2], 16)
fm = currentProgram.getFunctionManager()
listing = currentProgram.getListing()
target = toAddr(0x00109f70)
NAMES = {0x300: 'ALPHA_TEST_ENABLE', 0x304: 'BLEND_ENABLE', 0x308: 'CULL_FACE_ENABLE', 0x30c: 'DEPTH_TEST',
         0x33c: 'ALPHA_FUNC', 0x340: 'ALPHA_REF', 0x344: 'BLEND_SFACTOR', 0x348: 'BLEND_DFACTOR',
         0x350: 'BLEND_EQUATION', 0x354: 'DEPTH_FUNC', 0x358: 'COLOR_MASK', 0x35c: 'DEPTH_MASK'}
for r in getReferencesTo(target):
    a = r.getFromAddress()
    if not (lo <= a.getOffset() <= hi):
        continue
    ins = listing.getInstructionAt(a)
    # __fastcall: method in ECX, value in EDX (look back up to 12 instructions)
    regs = {}
    p = ins.getPrevious() if ins else None
    for _ in range(12):
        if p is None:
            break
        m = p.getMnemonicString()
        if m == 'CALL':
            break
        if m in ('MOV', 'XOR') and p.getNumOperands() == 2:
            dst = p.getDefaultOperandRepresentation(0)
            src = p.getDefaultOperandRepresentation(1)
            if dst in ('ECX', 'EDX') and dst not in regs:
                regs[dst] = '0' if (m == 'XOR' and src == dst) else src
        p = p.getPrevious()
    f = fm.getFunctionContaining(a)
    meth = regs.get('ECX', '?')
    name = ''
    try:
        name = NAMES.get(int(meth, 16) & 0x3fff, '')
    except Exception:
        pass
    out.write('%s %-28s method=%-10s %-18s value=%s\n' % (a, f.getName() if f else '?', meth, name,
                                                           regs.get('EDX', '?')))
out.close()
