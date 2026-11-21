# Ghidra Jython: front end shell (CFEShell) and its main menu page. See docs/frontend/README.md;
# ported in src/ui/fe_shell.cpp and src/ui/mainmenu.cpp.
from ghidra.program.model.symbol import SourceType
fm = currentProgram.getFunctionManager()
names = {
    0x0002f4b0: ('fe_shell_ctor', 'CFEShell (vtable 0x1a2890, object 0x597e90): pages, title text +0x9ff0, cameras +0x60c0 / +0x64c0 (eye +0x9f50, look +0x9f68, sway speed +0x9f4c).'),
    0x0002ffa0: ('fe_shell_draw', 'Background 0xc2570, scene (items), menu bars 0xc2870, description ticker: panel (62,370) 506x20 fill 0x80303050, text at (x,372), x -= 2 a frame, wraps at -(w+19) -> 640.'),
    0x000306b0: ('fe_shell_update', 'Per frame: page checks (FE TITLE / MESSAGEBOX / INTROSCREENS / CHARSELECT), controller messages.'),
    0x00031390: ('fe_shell_goto_page', 'Switch page by name ("FE MAIN", "FE OPTIONS", "FE CHARSELECT", "LOADER", ...).'),
    0x00031510: ('fe_shell_set_title', 'Title text +0x9ff0 = fe string EAX, shrunk to 240 wide.'),
    0x00031620: ('fe_shell_title_slide_in', 'Title at right edge 630-62, y 36, slides from above.'),
    0x000316e0: ('fe_shell_set_state', 'Header / footer hint layout per state (0 hide, 1 show, 4/7 leave); camera eye (0,0,1)/look (0,0,0) for the main menu.'),
    0x000288b0: ('fe_main_build', 'FE MAIN rows: ids 0x79..0x7e at x 62, y 105 + 40 i, scale 0.75, flag 2 (panel); Game Demos only if d:\\xdemos exists; cursor +0xe1c (margin 4).'),
    0x00029d30: ('fe_main_enter', 'Enter FE MAIN: hints 0x32 / 0x33, title 0x78, model + description per selection, rows slide in from x 640 (10 frames).'),
    0x00028e90: ('fe_main_input', 'Up / down: selection +0x102c (0 tournament .. 5 demos), model +0x1008, description 0x8c..0x91; A / START: pick (char select / options / demos 0x2a810); B: back to title.'),
    0x0002a520: ('fe_main_update', 'Sequences 1/3/5 draw the item model; 7: leave to the next page 0.1 s after the pick.'),
    0x0002a720: ('fe_main_draw_item', 'Selected row model: rotation 0, at (-4, -2 quick race / -1, 0), multiplayer x/y axes x 0.75.'),
    0x0002a810: ('fe_main_demos_confirm', 'Game Demos: message box 0x4 "Are you sure you wish to go to the Demos area?".'),
    0x0002a860: ('fe_launch_xdemos', 'XLaunchNewImage d:\\XDemos\\XDemos.xbe.'),
    0x0002a930: ('fe_main_show', 'Show / hide the 6 rows, the cursor, the hints.'),
    0x000c2570: ('fe_draw_background', 'frontend\\background: eye (0,0,-10) looking at the origin, fov pi*0.2857, model turned yaw sin(t/2)*15 deg, pitch sin(t/4)*15 deg; darken panel 0x50000000; feedback 0xa7080(15, 8).'),
    0x000c2810: ('fe_draw_fade', 'Fade: the feedback texture at alpha x 255.'),
    0x000c2870: ('neon_draw_menu_bars', 'Bars: panel (-400,-10) 1400x84 fill 0x304080c0 and (-400,391) fill 0xa0ffa030; qrlogo at (62,2) 256x128.'),
    0x000a7080: ('fe_feedback_swirl', 'Previous frame (0x50e160) drawn back twice, zoomed (1 + 15/320) and rotated +-8 x t.'),
    0x000a6eb0: ('fe_feedback_copy', 'Copy the back buffer into the feedback texture.'),
    0x000acce0: ('hud_cursor_select', 'Cursor element: mark the selected element (+0x1c4 |= 0x10), size it to the element plus margins.'),
}
comments = {}
labels = {
    0x00597e90: 'g_fe_shell',
}

def add_comment(f, cmt):
    old = f.getComment() or ''
    if cmt not in old:
        f.setComment((old + '\n' if old else '') + cmt)


tx = currentProgram.startTransaction('QR front end labels')
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
    for a, n in labels.items():
        createLabel(toAddr(a), n, True, SourceType.USER_DEFINED)
finally:
    currentProgram.endTransaction(tx, True)
print('QR front end labels applied: %d' % done)
