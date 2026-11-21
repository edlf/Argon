# Ghidra Jython: profiles (FE CHOOSESAVE, FE NAME ENTRY) and the profile reset. See docs/frontend/profiles.md "Profiles";
# ported in src/app/app_profile.cpp.
from ghidra.program.model.symbol import SourceType
fm = currentProgram.getFunctionManager()
names = {
    0x00083b60: ('profile_reset', 'New profile: best laps 999999, characters +0x16c = 0x7f, tracks +0xc88 = 0x7f, Mars 0, speeds +0xc90 = 1.'),
    0x00083c40: ('profile_merge_default_laps', 'Best laps: the default records where better.'),
    0x0001b440: ('fe_choosesave_build', 'FE CHOOSESAVE (vtable 0x19e090): header Select Profile / Percentage Complete at y 160, 5 rows from (112, 210 + 25 i), arrows, Press A to create profile.'),
    0x0001c850: ('fe_choosesave_enter', 'Title 0x64, hints 0x32 / 0x33 / 0x34 / 0x35, ticker 0x6a; selects the current profile.'),
    0x0001bab0: ('fe_choosesave_update', 'Selected row x 1.5 at y 205 + 25 i; leaves to FE NAME ENTRY / FE TITLE / FE MAIN.'),
    0x0001c070: ('fe_choosesave_input', 'A load, Y (or A with none) create, X delete question 0x67, B back to FE TITLE.'),
    0x0001be00: ('fe_choosesave_draw', 'Rotated quad (320, 260), gradient frames (52, 160) 536 x 20 and (52, 180) 536 x 180.'),
    0x0001cb30: ('fe_choosesave_fill_rows', 'Rows from the save list: name / percent, invalid -> 0x69.'),
    0x00037c70: ('fe_nameentry_build', 'FE NAME ENTRY (vtable 0x1a2478): name text (590, 105) orange; keyboard.'),
    0x00037d90: ('fe_nameentry_build_keys', 'Two key pages x 5 rows (code, slot width): letters and accents; specials 0x10000 Delete .. 0x10005 Done.'),
    0x000389d0: ('fe_nameentry_layout_keys', 'Keys from x 70, + slot + 6 (+4 after the first), rows y 160 + 36 r.'),
    0x00038c70: ('fe_nameentry_key_labels', 'Key labels in the current case (CAPS).'),
    0x00037a90: ('fe_nameentry_key_size', 'Key panel 16 x 20; specials 85, space 140, Done 380.'),
    0x00039630: ('fe_nameentry_enter', 'Title 0x2d0 (0x304 cheats), hints 0x33 / 0x32 / 0x3e / 0x3f, ticker 0x2dc; cursor on Done.'),
    0x00039480: ('fe_nameentry_update', 'Leaves to FE CHOOSESAVE (back) / FE MAIN (done) / FE CHEATMENU.'),
    0x00038f10: ('fe_nameentry_input', 'A key, X delete, Y done, B back; d-pad moves.'),
    0x0003ab30: ('fe_nameentry_draw', 'Gradient frames: keys (173, 150) 420 x 190, specials (50, 150) 123 x 190, name (50, 100) 543 x 45.'),
    0x00039ca0: ('fe_nameentry_press_key', 'Specials: 0 Delete, 1 clear, 2 CAPS, 3 letters page, 4 accents page, 5 Done; else append (max 9).'),
    0x00039fa0: ('fe_nameentry_default_name', 'PLAYER (0x4b0) + first free number, with an underscore cursor.'),
    0x0003a110: ('fe_nameentry_done', 'Checks 0x2da empty / 0x2dd spaces / 0x2db taken; profile_reset, CHEAT prefix -> 0x83af0; save; FE MAIN.'),
    0x0003aac0: ('fe_nameentry_delete', 'Remove the last character.'),
}
labels = {0x001baf74: 'g_profile'}


def add_comment(f, cmt):
    old = f.getComment() or ''
    if cmt not in old:
        f.setComment((old + '\n' if old else '') + cmt)


tx = currentProgram.startTransaction('QR profile labels')
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
print('QR profile labels applied: %d' % done)
