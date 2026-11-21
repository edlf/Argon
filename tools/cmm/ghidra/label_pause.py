# Ghidra Jython: the in-race pause menu (PAUSEMENU). See docs/frontend/race-end.md "Pause menu"; ported in
# src/ui/pause.cpp.
from ghidra.program.model.symbol import SourceType
fm = currentProgram.getFunctionManager()
names = {
    0x00088930: ('pausemenu_ctor', 'PAUSEMENU hud group: 17 texts.'),
    0x00088ab0: ('pausemenu_init', 'Texts (race_strings 0x3ec..0x40c): title (578, 36), items font 1 x 0.7 centred on 320.'),
    0x000898e0: ('pausemenu_update', 'PAUSE ON / OFF / QUIT messages, cursor / A / B / left-right, page visibility.'),
    0x00089860: ('pausemenu_draw', 'Draws the group.'),
    0x0008b950: ('pausemenu_set_cursor', 'CPauseMenu: cursor item +0x60c (0..12).'),
    0x0008be20: ('pausemenu_layout', 'Main page items at y 116 + 35 n (ghost items / Track Select optional).'),
    0x0008bad0: ('pausemenu_cursor_up', 'Previous item on the page.'),
    0x0008bc80: ('pausemenu_cursor_down', 'Next item on the page.'),
    0x0008b800: ('pausemenu_volume_text', '"%s: < %d >" for SFX / Music / Engine Volume.'),
    0x0008b470: ('pausemenu_resume', 'Continue: vibration flags from the settings, PAUSE OFF.'),
    0x0008b560: ('pausemenu_replay_ghost', 'Replay a ghost (items 1 / 2).'),
    0x0008b5b0: ('pausemenu_main_menu', 'Main Menu (FE GHOSTSAVE first with a new Time Attack ghost).'),
    0x0008b630: ('pausemenu_track_select', 'Track Select (0x5a1e7a = 1).'),
    0x0008b6c0: ('pausemenu_open_options', 'Options page (+0x610 = 1, cursor Exit).'),
    0x0008b6f0: ('pausemenu_restart', 'Restart (0x52abe8 = 1, music restarts).'),
    0x0008b7d0: ('pausemenu_close_options', 'Back to the main page on Continue.'),
    0x0007e5a0: ('pause_background', 'pause_background_draw(param, 1.75).'),
    0x0007e5e0: ('pause_background_draw', '0: clear, echo drawn back 5 x offset -2..2 x param px, 3 passes x then 3 y, copied back; 1: FE swirl. Then the menu bars.'),
}


def add_comment(f, cmt):
    old = f.getComment() or ''
    if cmt not in old:
        f.setComment((old + '\n' if old else '') + cmt)


tx = currentProgram.startTransaction('QR pause menu labels')
done = 0
try:
    for a, (n, cmt) in sorted(names.items()):
        f = fm.getFunctionAt(toAddr(a))
        if f is None:
            f = createFunction(toAddr(a), n)
            if f is None:
                print('could not create a function at %x' % a)
                continue
        f.setName(n, SourceType.USER_DEFINED)
        add_comment(f, cmt)
        done += 1
finally:
    currentProgram.endTransaction(tx, True)
print('QR pause menu labels applied: %d' % done)
