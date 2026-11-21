# Ghidra Jython: Tournament mode (FE TOURNEYTYPE, the track order, progress, the tournament win). See
# docs/frontend/tournament.md "Tournament"; ported in src/app/app_tourney.cpp.
from ghidra.program.model.symbol import SourceType
fm = currentProgram.getFunctionManager()
names = {
    0x00035110: ('fe_tourneytype_build', 'FE TOURNEYTYPE: fe 0xf1..0xf5 at (62, 100 + 40 i); New / Continue (0x140 / 0x141) at (568, 300 / 330).'),
    0x00036320: ('fe_tourneytype_enter', 'Enter (times out in the decompiler).'),
    0x00035940: ('fe_tourneytype_input', 'Input (times out in the decompiler).'),
    0x00035e50: ('fe_tourneytype_update', 'Leaves to FE CHARSELECT (back) or LOADER.'),
    0x000365b0: ('fe_tourneytype_ticker', '"Progress: %d/%d races completed. Next race: <track>." + level hint; locked: fe 0x116 + level.'),
    0x00035b20: ('fe_tourneytype_start', 'Level -> 0x52cd08 / game speed; track = 0x79130(level, character); state 7.'),
    0x00036530: ('fe_tourneytype_row', 'Cursor row 0..4 (New / Continue: the level).'),
    0x000367a0: ('fe_tourneytype_show', 'Shows / hides the rows and New / Continue.'),
    0x00079130: ('tourney_track', 'Track of race n: home, table 0x1b9438 row + level offset, other route, Draco.'),
    0x000791f0: ('tourney_alt_race', 'Amateur / Expert race 3, Master race 4, Redshift race 0: the home track\'s other route.'),
    0x000a1320: ('tourney_last_race', 'Player won and progress == races - 1 (2 / 4 / 4 / 5 / 9 races).'),
}


def add_comment(f, cmt):
    old = f.getComment() or ''
    if cmt not in old:
        f.setComment((old + '\n' if old else '') + cmt)


tx = currentProgram.startTransaction('QR tournament labels')
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
    createLabel(toAddr(0x001b9438), 'g_tourney_track_table', True, SourceType.USER_DEFINED)
    createLabel(toAddr(0x001bb0f4), 'g_tourney_progress', True, SourceType.USER_DEFINED)
    createLabel(toAddr(0x001bbc04), 'g_tourney_max_level', True, SourceType.USER_DEFINED)
    createLabel(toAddr(0x0052cd08), 'g_tourney_level', True, SourceType.USER_DEFINED)
    createLabel(toAddr(0x0052ccfc), 'g_game_mode', True, SourceType.USER_DEFINED)
finally:
    currentProgram.endTransaction(tx, True)
print('QR tournament labels applied: %d' % done)
