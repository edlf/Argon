# Ghidra Jython: the front-end leftovers decoded 2026-11-14 (FE TOURNEYTYPE's input and models, the speedometer, the
# rotated quad's picture on FE TRACK INFO, FE MESSAGEBOX kinds 1 / 7..9, the LOADER's Nemesis Challenge intro, the
# second timed effect, Percentage Complete, Trophy Level). See docs/frontend/README.md; ported in src/ui and src/logic.
from ghidra.program.model.symbol import SourceType
fm = currentProgram.getFunctionManager()
names = {
    0x00035940: ('fe_tourneytype_input', 'B hides New / Continue (or leaves), A: locked -> sound 1; no races won -> start; else New / Continue (cursor on Continue); New asks fe 0x156 (kind 5).'),
    0x00035e50: ('fe_tourneytype_update', 'Leaves to FE CHARSELECT / LOADER (0x1bbbec = level); speedometer needle 35 deg x level; trophy tourn_C 40 x 40 and race icons tourn_B / tourn_A 30 x 30 after each row.'),
    0x00036320: ('fe_tourneytype_enter', 'Message 0xfe00/01: after the New Tournament question Yes (+0x1f4 == 1) clears the progress and starts; else cursor on 0x1bbbec.'),
    0x00036530: ('fe_tourneytype_cursor_row', 'The level under the hud cursor (New / Continue: +0x1214).'),
    0x000367a0: ('fe_tourneytype_show', 'Shows / hides the rows, New / Continue (+0x1218), the cursor; +0x121a = shown (draws the models).'),
    0x000b99f0: ('model_node_set_rotation', 'Node eax of model ecx: local 3x3 = YawPitchRoll(y, x, z) scaled by the node scale (+0x11c..); translation kept.'),
    0x000b9c40: ('model_find_node', 'Node index by name (SPEEDO_NEEDLE).'),
    0x000cbe50: ('hud_draw_rotated_quad', '(x, y, angle, from, to, width): angle + 180 deg (0x5a211c x 180); corners to-end -/+ half width, from-end -/+; uv (1,0) (0,0) (1,1) (0,1).'),
    0x000a7410: ('fe_draw_timed_black', 'Second timed effect (+0xf6208 armed, +0xf620c start, +0xf6210 duration): a black 640 x 480 rectangle; never armed (only cleared).'),
    0x000b11f0: ('font_draw_text_scaled', 'font_draw_text(text, x, y, s, s, colour ecx, identity) with font = arg 1, align eax.'),
    0x000b1270: ('font_draw_text_turned', 'font_draw_text with the matrix (cos a, sin a, -sin a, cos a).'),
    0x000791f0: ('tourney_is_nemesis_race', 'Tournament: Amateur / Expert race 3, Master race 4, Redshift race 0.'),
    0x000c03a0: ('save_profile_percent', 'Percentage Complete: sum of 0x82910 per character + characters 8..15 + eggs + 0x1bbc34 + 0x1bbc35 + 2 x 0x1bbc36.'),
    0x00082910: ('profile_trophy_level', 'Highest trophy level held + 1 (0 = none).'),
    0x000390d0: ('fe_nameentry_move_up', 'Row - 1 (round); from row 0 past col 1 onto Done remembering (+0x14a30); from Space / Done to the remembered column.'),
    0x00039230: ('fe_nameentry_move_down', 'Row + 1 (round); row 2 past col 6 onto Space, row 3 onto Done (remembering), Done to the remembered column (max 10).'),
    0x00039360: ('fe_nameentry_move_left', 'Column - 1 round the row length (0xa5ec); skips empty keys and, languages 1 / 2, the Accents key.'),
    0x000393f0: ('fe_nameentry_move_right', 'Column + 1 round the row length.'),
    0x000acbb0: ('hud_cursor_ctor', 'Cursor element (vtable 0x19b048): mode +0x1d4 0, speed +0x1d8 1, ease +0x1dc 0.5, margins +0x1e0 / +0x1e4 0.'),
    0x000acda0: ('hud_cursor_update', 'Moves the cursor rectangle onto the selected element: mode 1 at +0x1d8 px a frame, mode 2 easing +0x1dc; draws nothing.'),
    0x000ad070: ('hud_cursor_input', 'D-pad follows the selected element links +0x1a8..+0x1b4 (UI sound 2); A calls the element +0x5c.'),
    0x000429c0: ('timeattack_lap_end', 'Race best; a lap under the profile best (g_profile) sets it, the record when under the table (0x52ac03), pending page FE FASTESTLAP.'),
    0x00019990: ('fe_charselect_slide_info', 'Info rows (y 131..205) slide 1280 <-> 560 over 10 frames (cl = 1 in).'),
    0x000bfb60: ('save_enumerate', 'The save list (U:\\): per entry +0 invalid, +4 percent (0xc03a0), +8 name, +0x210 next.'),
}


def add_comment(f, cmt):
    old = f.getComment() or ''
    if cmt not in old:
        f.setComment((old + '\n' if old else '') + cmt)


tx = currentProgram.startTransaction('QR front-end leftovers labels')
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
print('QR front-end leftovers labels applied: %d' % done)
