# Ghidra Jython: Options (FE OPTIONS, Game / Sound Settings and the settings list engine). See docs/frontend/README.md
# "Options"; ported in src/ui/options.cpp.
from ghidra.program.model.symbol import SourceType
fm = currentProgram.getFunctionManager()
names = {
    0x0002c740: ('fe_options_build', 'FE OPTIONS (vtable 0x19da00): 9 rows Game Settings .. Cheats, font 1 x 0.75 with panels.'),
    0x0002d9d0: ('fe_options_enter', 'Title 0x280; rows slide 640 -> 62 at y 100 + 30 i; Easter Eggs / Cheats only when unlocked.'),
    0x0002cef0: ('fe_options_input', 'A -> the row page (OPTIONS MAIN / SOUND / FASTESTLAP / HIGHSCORES / GHOSTMANAGE / CREDITS / EASTEREGGS / CHEATMENU), Exit / B back.'),
    0x0002d830: ('fe_options_update', 'Leaves to the chosen page (0x31440); draws the Options item model (shell +0x1ff8) at (-4, -1, 0).'),
    0x0002e000: ('fe_options_ticker', 'Row descriptions 0x370..0x378.'),
    0x0002f000: ('fe_optmain_build', 'FE OPTIONS MAIN (Game Settings, vtable 0x19d990): settings list 0x1baf6c / 0x1baf70.'),
    0x0002f1a0: ('fe_optmain_enter', 'Title 0x281; rebuild the rows.'),
    0x0002f0b0: ('fe_optmain_input', 'Settings input, then the gamma ramp.'),
    0x0002e970: ('fe_optsound_build', 'FE OPTIONS SOUND (Sound Settings, vtable 0x19dc70): settings list 0x1baf64 / 0x1baf68.'),
    0x0002ef90: ('fe_optsound_enter', 'Rebuild the rows.'),
    0x0002ea20: ('fe_optsound_input', 'Settings input.'),
    0x000abca0: ('settings_build_rows', 'Label (62, 100 + 20 x (count - i)) with a panel, options from x 300 (+ width), choice bright / others grey.'),
    0x000ac040: ('settings_input', 'Up next / down previous entry, left / right choice, A Exit / Reset (0x341) / action, B back.'),
    0x000abf90: ('settings_link_rows', 'Up / down links between the row elements.'),
    0x000aca90: ('settings_cursor_last', 'Cursor on the last entry.'),
    0x000ac6c0: ('settings_enter', 'Settings page enter.'),
    0x00081d80: ('settings_list_head', 'Exit (type 0) and Reset to Default (type 1).'),
    0x00081e00: ('settings_add_entry', '(label, type, description, action, change, slot width): new 0x260 entry appended.'),
    0x00081ee0: ('settings_add_option', 'Option string; width = text width + 10; flag 1 = default.'),
    0x00081fd0: ('settings_build_game', 'Vibration 4..1, Speed Units, Brightness, HUD, Multiplayer Laps.'),
    0x000824c0: ('settings_build_sound', 'Music Options, Engine / SFX / Music Volume, Headphones.'),
    0x00081fb0: ('settings_value', 'Choice of the entry with this type.'),
    0x0002f100: ('setup_gamma_ramp', 'Brightness setting -> gamma ramp.'),
}


def add_comment(f, cmt):
    old = f.getComment() or ''
    if cmt not in old:
        f.setComment((old + '\n' if old else '') + cmt)


tx = currentProgram.startTransaction('QR options labels')
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
print('QR options labels applied: %d' % done)
