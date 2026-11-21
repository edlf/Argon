# Ghidra Jython: front end character pages (FE CHARSELECT, FE CHAR INFO, FE SHIP INFO) and FE COURSE SELECT.
# See docs/frontend/race-setup.md "Character select"; ported in src/ui/charsel.cpp.
from ghidra.program.model.symbol import SourceType
fm = currentProgram.getFunctionManager()
names = {
    0x00015550: ('fe_charselect_build', 'FE CHARSELECT (vtable 0x19e298): info rows Trophy Level / value / Home Circuit / value / (difficulty) right-aligned at 568, y 131/149/169/187/205.'),
    0x000193b0: ('fe_charselect_enter', 'Hints Back / Select, footers [Y] Ship Data / Character Data [X] (not in multiplayer), title 0xa0.'),
    0x000163b0: ('fe_charselect_input', '16 characters in 2 rows of 8: left/right wrap in the row, up/down swap rows; A picks an unlocked one (-> FE TOURNEYTYPE / FE COURSE SELECT), B FE MAIN, Y FE SHIP INFO, X FE CHAR INFO.'),
    0x00015db0: ('fe_charselect_update', 'Draws the 3D character (0x59e8cc, frontend\\characters\\<pilot>) at table 0x1b9a88[id] into render list +0x690c (camera 2); locked = lights off.'),
    0x000178a0: ('fe_charselect_draw', 'Name (560,110) 0.75 right; frame panel (84,240) 472x125 fill 0x80404080/EAX 0x80804040; tiles 54x54 at x 90+58i, y 246/305 (selected fill 0xffa08040); pilot icon or "?"; portrait icon 128x128 at (88,110).'),
    0x00019760: ('fe_charselect_ticker', 'Description: locked -> fe 0xaf+id (7: 0xb6); unlocked original with a locked nemesis -> 0xb4/0xb5.'),
    0x00019ba0: ('fe_charselect_info', 'Trophy level (infotext 250+best or fe 0xcc None), home circuit infotext 300+id, difficulty strings 1700+id%8 "(%s)".'),
    0x00019990: ('fe_charselect_slide', 'Info rows slide between x 720 and 568 (y 131..205).'),
    0x000142b0: ('load_frontend_characters', 'Loads frontend\\characters\\<pilot> into 0x59e8cc.'),
    0x00014370: ('fe_charinfo_build', 'FE CHAR INFO (vtable 0x19d7f0): name (568,86) font 1 0.75 orange; Age/Origin/Quote labels at 568 (131/167/207), values orange (149/187), quote (62,225) 516 wide; biography (62,280) 4 lines; arrows icons\\up/down at (300,268/368).'),
    0x00014bf0: ('fe_charinfo_enter', 'Biography strings 1500+4id, age 1501+4id, origin 1502+4id, quote 1720+id; title 0x168.'),
    0x00014a50: ('fe_charinfo_input', 'B back; up/down scroll the biography by the visible line count.'),
    0x00014f70: ('fe_charinfo_update', 'Draws the 3D character; leaves to FE CHARSELECT / FE SHIP INFO / FE COURSE SELECT.'),
    0x00032a30: ('fe_shipinfo_build', 'FE SHIP INFO (vtable 0x19dd48): labels Ship Name/Weapons/Weight (fe 0xdd..) at (70,310+20i) 0.8, values (190,..) 0.5x0.55 orange; 7 stat labels (fe 0xe0..) right at 560.'),
    0x00032ee0: ('fe_shipinfo_enter', 'Ship name strings 1503+4id, weapons 1564+id, weight table 0x1b9798 (kg, or lbs x2.2); loads frontend\\shipmodel\\<pilot>; title 0xdc.'),
    0x00032e80: ('fe_shipinfo_input', 'B back to FE CHARSELECT.'),
    0x00033230: ('fe_shipinfo_update', 'Ship model: YPR(0,25 deg,0) after a y turn of -0.5 deg a frame, at (-2, 6.95, -12).'),
    0x000326a0: ('fe_shipinfo_draw', '7 stat bars (0xc2420) at x 368 200x24 from y 100 (+28, +44 after rows 2/3); 3 panels (62,309+20i) 120x17 fill 0x60404070; name (62,100); portrait 64x64 in a frame at (62,130).'),
    0x000c2420: ('hud_draw_stat_bar', 'Black panel w x h, then fill 0xffff8040 of value x 1 % (from the right when the flag is set).'),
    0x000c2310: ('hud_draw_gradient_frame', '4-colour quad 0xb0102070 / 0xb0702010 (flag swaps) + black outline.'),
    0x00084160: ('profile_tournament_won', 'Tournament win: marks the trophy, unlocks the nemesis (id < 7: id + 8; the originals unlock 7), raises the speed class.'),
    0x0001dfd0: ('fe_course_enter', 'FE COURSE SELECT (vtable 0x19de18): track names infotext 300+i, locked grey; globe models.'),
    0x0001d860: ('fe_course_update', 'Leaves to FE CHARSELECT (back) or FE SPEEDSELECT / FE TRACK INFO.'),
    0x0001d360: ('fe_course_draw', 'Earth / Mars globe model turning to the selected track (table 0x1b9a18), corner brackets.'),
}
labels = {
    0x001b9a88: 'g_fe_char_pos',
    0x001b9798: 'g_ship_stats',
    0x001bb0e0: 'g_char_unlock_mask',
    0x001b93f8: 'g_pilot_icon_file',
}

def add_comment(f, cmt):
    old = f.getComment() or ''
    if cmt not in old:
        f.setComment((old + '\n' if old else '') + cmt)


tx = currentProgram.startTransaction('QR character page labels')
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
print('QR character page labels applied: %d' % done)
