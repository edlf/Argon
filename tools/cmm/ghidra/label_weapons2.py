# Ghidra Jython: weapon lock-on and the HUD crosshairs. See docs/weapons/README.md;
# ported in src/app/app_weapons.cpp.
from ghidra.program.model.symbol import SourceType
fm = currentProgram.getFunctionManager()
names = {
    0x0004d9d0: ('hud_draw_targets', 'Crosshairs: per colour 6 entries {state, craft, x, y, z, time} (white +0x1a84 / blue +0x1b14 / yellow +0x1ba4, stride 0x18); size clamp(2048/z,16,64) x scale; state 1 scale 10(0.5-age)+1 alpha 255-510(0.5-age), state 3 scale 1+10 age alpha 255-1020 age; spin time x -360; additive. Also the nemesis corner brackets + name. docs/weapons/README.md.'),
    0x0004e7f0: ('hud_targets_update', 'Marks entries 4, re-adds the player weapon lock lists (+0x80/+0x84/+0x88 counts), unmatched -> state 3 (released, 0.25 s); state 1 -> 2 after 0.5 s.'),
    0x0004e580: ('hud_target_add_red', 'New crosshair entry (state 1) at most every 0.25 s; crosshair.dds.'),
    0x0004e650: ('hud_target_add_blue', 'New crosshair entry (state 1) at most every 0.25 s; crosshair_blue.dds.'),
    0x0004e720: ('hud_target_add_yellow', 'New crosshair entry (state 1) at most every 0.25 s; crosshair_yellow.dds.'),
    0x0006c420: ('weapon_lock_targets', 'Per colour: count = rec+0x168 if rec+0x150 and armed (special overrides all), 0 while that shot is out; other crafts < 500 m with dir.fwd > 0.85, nearest first, max 6 -> list +0x60; sets craft +0xb4.'),
}
comments = {}
labels = {
    }

def add_comment(f, cmt):
    old = f.getComment() or ''
    if cmt not in old:
        f.setComment((old + '\n' if old else '') + cmt)


tx = currentProgram.startTransaction('QR weapon lock labels')
done = 0
try:
    for a, (n, cmt) in sorted(names.items()):
        f = fm.getFunctionAt(toAddr(a))
        if f is None:
            f = createFunction(toAddr(a), n)
            if f is None:
                print('could not create a function at %x' % a)
                continue
            print('created function %s at %x' % (n, a))
        f.setName(n, SourceType.USER_DEFINED)
        add_comment(f, cmt)
        done += 1
    for a, cmt in sorted(comments.items()):
        f = fm.getFunctionAt(toAddr(a))
        if f is None:
            print('no function at %x' % a)
            continue
        add_comment(f, cmt)
        done += 1
    st = currentProgram.getSymbolTable()
    for a, n in sorted(labels.items()):
        st.createLabel(toAddr(a), n, SourceType.USER_DEFINED)
        done += 1
    print('QR weapon labels applied: %d' % done)
finally:
    currentProgram.endTransaction(tx, True)
