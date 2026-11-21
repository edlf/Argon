# Ghidra Jython: the front-end model fade and the race loading screen (LOADER).
# See docs/frontend/README.md "Model fade" / "Loading screen"; ported in src/ui/menu.cpp and src/ui/loader.cpp.
from ghidra.program.model.symbol import SourceType
fm = currentProgram.getFunctionManager()
names = {
    0x00030303: ('fe_shell_draw_scene', 'Shell draw: lights, the 3D render lists, then fe_draw_fade(shell +0xa238) over them (other branch: at least 0.2), clear depth, menu bars (not FE TITLE / CREDITS), ticker panel (62, 370) 506 x 20.'),
    0x000a6f00: ('fe_draw_echo_quad', 'Full-screen quad of the feedback texture 0x50e160 in the given colour (alpha = fade).'),
    0x00081be0: ('loader_next_hint', 'Y on the loading screen: next hint (Quick Race 0x1bbc2c wraps after 31, Tournament 0x1bbc30 after 39, Time Attack 0x1bbc28 after 15).'),
    0x0002f2e0: ('fe_start_transition', '(start, duration): copy the front buffer into the echo texture 0x50e160 and arm the fade (+0xf61fe / +0xf6200 / +0xf6204); 0.25 s at 0x80556, 0x97486, 0x30b86.'),
    0x000a72a0: ('fe_draw_transition', 'Frozen echo texture full screen at alpha 255 - 255 x clamp((now - start) / duration); off when done.'),
    0x00034cd0: ('fe_title_enter', 'FE TITLE enter: page clock +0x7f8, hints, title; message box results.'),
    0x00034660: ('fe_title_update', 'FE TITLE: sequences 1 -> 8 (Press START, logo 0..4.9 s) -> 5 -> 7 (logo back at 4x, 1.225 s) -> FE CHOOSESAVE; clocks of dodechstart (+0x3e48) and titlelogo (+0x4458).'),
    0x00034970: ('fe_title_input', 'START / A in sequence 5 -> 7; else hint state 4.'),
    0x000808e0: ('loader_draw_frame', 'LOADER frame (loader thread): title box Welcome to / track / Home circuit + portrait, hint panel, Loading... / Press A to continue; Nemesis intro; multiplayer list.'),
}
labels = {
    0x005a20c8: 'g_fe_model_fade',
    0x001bbc2c: 'g_quickrace_hint',
    0x0050e160: 'g_fe_echo_texture',
    0x001ef590: 'g_loader_thread_event',
}

def add_comment(f, cmt):
    old = f.getComment() or ''
    if cmt not in old:
        f.setComment((old + '\n' if old else '') + cmt)


tx = currentProgram.startTransaction('QR loader labels')
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
    f = fm.getFunctionAt(toAddr(0x000c2810))
    if f: add_comment(f, 'Called after the 3D model with shell +0xa238: the model pages set 1 - clamp(now - model shown, 0, 1).')
    for a, n in labels.items():
        createLabel(toAddr(a), n, True, SourceType.USER_DEFINED)
finally:
    currentProgram.endTransaction(tx, True)
print('QR loader + title labels applied: %d' % done)
