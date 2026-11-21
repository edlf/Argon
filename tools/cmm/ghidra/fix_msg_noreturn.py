# Ghidra Jython: FUN_000b4e00 (send a message to a named object) returns; it was marked no-return, which cut every
# caller's body at the call (the post-race / pause code). Clear the flag, disassemble after each call, fix bodies.
from ghidra.app.cmd.function import CreateFunctionCmd
from ghidra.program.model.symbol import SourceType
fm = currentProgram.getFunctionManager()
listing = currentProgram.getListing()
target = toAddr(0xb4e00)
tx = currentProgram.startTransaction('b4e00 returns')
fixed = set()
try:
    f = fm.getFunctionAt(target)
    f.setNoReturn(False)
    f.setName('obj_send_message', SourceType.USER_DEFINED)
    for r in getReferencesTo(target):
        src = r.getFromAddress()
        ins = listing.getInstructionAt(src)
        if ins is None:
            continue
        nxt = ins.getMaxAddress().add(1)
        # the call was treated as terminal: clear its fall-through override and disassemble on
        ins.setFlowOverride(ghidra.program.model.listing.FlowOverride.NONE)
        if listing.getInstructionAt(nxt) is None:
            disassemble(nxt)
        g = fm.getFunctionContaining(src)
        if g is not None:
            fixed.add(g.getEntryPoint())
    for e in fixed:
        g = fm.getFunctionAt(e)
        CreateFunctionCmd.fixupFunctionBody(currentProgram, g, monitor)
finally:
    currentProgram.endTransaction(tx, True)
print('b4e00 callers fixed: %d' % len(fixed))
