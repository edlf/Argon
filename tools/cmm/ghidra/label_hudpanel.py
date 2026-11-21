# Ghidra Jython: HUD panels (chamfered plasma boxes, shield bar). See docs/physics/README.md (HUD);
# ported in src/app/app_overlay.cpp (drawPanel) and shaders/hudpanel.frag.
from ghidra.program.model.symbol import SourceType
fm = currentProgram.getFunctionManager()
names = {
    0x000c1b40: ('hud_draw_panel', '(x, y, w, h, highlight, glow, line col, fill, fill col, scale): chamfered hexagon (TL / BR cut 8),\n'
                 'fill, glow ring 6 out, feplasma0/1/2/0 overlay (MOD2X/MOD/MOD/MOD2X, uv from 0xc16f0), outline. docs/physics/README.md (HUD).'),
    0x000c16f0: ('hud_panel_uv_scroll', 'Panel uv sets: pos x 0.005 x scale, t = (EAX % 1000) / 1000: (sx, sy+t) (sx+t, sy) (-t, sy) (sx+t, sy+t).'),
    0x000c1600: ('hud_panel_init', 'Panel vertex colours (0x8080ff, white / orange corners, ring black) and data/hud/feplasma0..2, febackball.'),
    0x0006aba0: ('craft_shield_weapon_up', 'True if a type-2 weapon slot has +0x90 > 0: highlights / glows the shield bar.'),
}
comments = {
    0x000c24d0: 'Back panel (highlight = glow = flag, fill EAX) + value panel (fill EBX) via hud_draw_panel.',
}
labels = {
    0x001b9c2c: 'g_hud_feplasma0',
    0x001b9c54: 'g_hud_feplasma1',
    0x001b9c7c: 'g_hud_feplasma2',
}

def add_comment(f, cmt):
    old = f.getComment() or ''
    if cmt not in old:
        f.setComment((old + '\n' if old else '') + cmt)


tx = currentProgram.startTransaction('QR HUD panel labels')
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
    print('QR HUD panel labels applied: %d' % done)
finally:
    currentProgram.endTransaction(tx, True)
