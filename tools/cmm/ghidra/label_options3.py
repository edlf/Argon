# Ghidra Jython: settings effects, Easter Eggs / Cheats, custom music, rumble. See docs/frontend/options.md "Settings effects",
# "Easter Eggs and Cheats", "Custom music" and docs/physics/rumble.md "Rumble"; ported in src/ui/eggs.cpp,
# src/ui/playlist.cpp, src/app/app_settings.cpp (applySettings), src/app/app_overlay.cpp (drawBrightness),
# src/craft/craft_engine.cpp (GameCraft::rumble).
from ghidra.program.model.symbol import SourceType
fm = currentProgram.getFunctionManager()
names = {
    0x0002f100: ('fe_brightness_gamma', 'Brightness: ramp[i] = clamp(i + (step * 3 - 12) * 4, 0, 255) for R G B -> SetGammaRamp.'),
    0x0002eb80: ('fe_engine_volume', 'Engine volume: SetVolume(millibels 0x1a8564[step]).'),
    0x000dee90: ('music_volume_mb', 'Music volume: millibels 0x1a8564[setting 0xd].'),
    0x0001ec40: ('fe_eastereggs_build', 'FE EASTEREGGS: 8 rows (62, 100 + 20 i), Off at 462 / On at 512, Exit (62, 260).'),
    0x0001f390: ('fe_eastereggs_enter', 'Title 0x314; hint Back; values + ticker.'),
    0x0001f180: ('fe_eastereggs_input', 'B / A on Exit back; left / right switch an unlocked egg (0x1bbc10).'),
    0x0001f440: ('fe_eastereggs_update', 'Leaves to FE OPTIONS; the cursor row choice flashes 255 -> 64 (-16 a frame).'),
    0x0001f5e0: ('fe_eastereggs_values', 'Names fe 0x30c + i or ???? 0x17f; Locked 0x315 / Off-On colours.'),
    0x0001f7d0: ('fe_eastereggs_ticker', 'Ticker 0x317 locked / fe 9 unlocked / 0x33b Exit.'),
    0x00019f80: ('fe_cheatmenu_build', 'FE CHEATMENU: 7 cheats (62, 100 + 20 i), value at 462 / On at 512, Enter Cheat Code (62, 240), Exit (62, 260).'),
    0x0001abd0: ('fe_cheatmenu_enter', 'Compares the typed code with fe 0x3d4 + i (unlocks 0x1bbc18 + i); names 0x2f8 + i.'),
    0x0001a5e0: ('fe_cheatmenu_input', 'A activates cheats 0..4 (chars, speeds, tracks, eggs, results); L / R switch 5 / 6; Enter Code -> FE NAME ENTRY.'),
    0x0001b030: ('fe_cheatmenu_update', 'Leaves to FE OPTIONS or FE NAME ENTRY (code mode).'),
    0x0001b1a0: ('fe_cheatmenu_row', 'Cursor row: 0..6, -2 Enter Cheat Code, -1 Exit.'),
    0x0001b230: ('fe_cheatmenu_ticker', 'Ticker 0x303 / 0x302 / 0x375 / 0x33b and the right hint.'),
    0x00083af0: ('profile_cheat_name', 'A name starting CHEAT (fe 0x4b1): 0x1bbc26 = 1, settings to defaults, playlist cleared.'),
    0x00026af0: ('fe_levelmusic_build', 'FE LEVELMUSIC: headers 0x26c / 0x26d at y 130, areas 250 x 200 at (62, 160) / (328, 160), 10 rows.'),
    0x00027930: ('fe_levelmusic_enter', 'Hints Back / Select Song / Select Column / Preview Song; counts the lists.'),
    0x00027380: ('fe_levelmusic_input', 'Up / down, left / right column, A move song, Y preview, B back.'),
    0x00027650: ('fe_levelmusic_update', 'Leaves to FE MUSICSELECT, front-end music again.'),
    0x00027b80: ('fe_levelmusic_list_free', 'Songs not in the playlist: soundtrack\\song rows (right column).'),
    0x00027e10: ('fe_levelmusic_list_playlist', 'The playlist 0x4406fc rows (left column).'),
    0x00027fe0: ('fe_levelmusic_move', 'A: playlist song out / free song into the first free of 32 slots.'),
    0x00028550: ('fe_levelmusic_preview', 'Y: play the song under the cursor.'),
    0x00028320: ('fe_levelmusic_count', 'Counts the playlist (+0x3cf0) and the free songs (+0x3cec).'),
    0x000bf350: ('profile_save_playlist', 'Saves the 32 playlist slots (soundtrack id, song, name).'),
    0x000bf710: ('profile_load_playlist', 'Loads the playlist; none found: Music Source Default (0x1bbc27 = 1).'),
    0x0005b610: ('craft_rumble', 'Surface rumble: amp = a^2 x rumble_<surface> x 0.2 x ground, motors +0x9ea / +0x9e8.'),
    0x000b4530: ('pad_update_feedback', 'Pad timers; XInputSetState when the motor speeds (+0x194 / +0x196) change.'),
    0x000b46d0: ('pad_set_rumble', 'Sets the pad motor speeds (+0x194, +0x196).'),
}
labels = {0x001a8564: 'g_volume_millibels', 0x001bbc08: 'g_egg_unlocked', 0x001bbc10: 'g_egg_on',
          0x001bbc18: 'g_cheat_unlocked', 0x001bbc1f: 'g_cheat_on', 0x001bbc26: 'g_cheat_menu',
          0x001bbc27: 'g_music_default', 0x004406fc: 'g_custom_playlist', 0x00440e70: 'g_soundtracks',
          0x00440e74: 'g_soundtrack_count', 0x001bbbf0: 'g_cheat_infinite_shield', 0x001bbbf1: 'g_cheat_infinite_turbo'}


def add_comment(f, cmt):
    old = f.getComment() or ''
    if cmt not in old:
        f.setComment((old + '\n' if old else '') + cmt)


tx = currentProgram.startTransaction('QR options effects / eggs / cheats / music / rumble labels')
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
print('QR options effects labels applied: %d' % done)
