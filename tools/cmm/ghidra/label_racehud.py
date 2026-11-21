# Ghidra Jython: race HUD (position list, minimap crafts, pilot icons). See docs/physics/README.md (HUD);
# ported in src/hud/hud_minimap.cpp and game_hud.cpp (updateMessages).
from ghidra.program.model.symbol import SourceType
fm = currentProgram.getFunctionManager()
names = {
    0x000793a0: ('hud_load_pilot_icons', 'Loads data/hud/tex<table 0x1b93f8[i]>.dds (+ arrow.dds) into 0x528ea8, stride 0x50: portrait +0, arrow +0x28.'),
}
comments = {
    0x0004f090: 'Also draws the other crafts (arrow.dds via hud_draw_icon, white player / yellow / orange pilot +-8) and the\n'
                'position list left of the map: rows of 38 in place order, frame, panel, 36x36 portrait, energy bar, place word\n'
                '(strings 607+) for the player. See docs/physics/README.md (HUD).',
    0x000cbc40: 'Corners at angle + 225 / 135 / 315 / 45 deg, size from the centre; uv (0,0) (1,0) (0,1) (1,1); colour in EAX.',
}
labels = {
    0x00528ea8: 'g_hud_pilot_icons',
    0x001b93f8: 'g_hud_pilot_icon_index',
}

def add_comment(f, cmt):
    old = f.getComment() or ''
    if cmt not in old:
        f.setComment((old + '\n' if old else '') + cmt)


tx = currentProgram.startTransaction('QR race HUD labels')
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
    print('QR race HUD labels applied: %d' % done)
finally:
    currentProgram.endTransaction(tx, True)
