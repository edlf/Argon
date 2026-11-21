# Ghidra Jython: the race scores (SCOREMENU, POINTSMENU, cash, high scores, upgrades). See docs/frontend/README.md "Race
# scores"; ported in src/app/app_score.cpp.
from ghidra.program.model.symbol import SourceType
fm = currentProgram.getFunctionManager()
names = {
    0x00083480: ('cashmodifiers_load', 'config\\cashmodifiers.txt into profile +0x1978.. in file order.'),
    0x00084060: ('race_points', 'Race time, lap time, weapon hits, chips, mega-ups, pickups, kills, air time x their bonuses.'),
    0x00084110: ('race_score', '(place bonus + race points) x speed modifier.'),
    0x00094390: ('scoremenu_init', 'SCOREMENU "Race Scores": rows y 100..260, cash line (320, 300).'),
    0x00095780: ('scoremenu_fill', 'Points, position bonus, speed multiplier texts.'),
    0x00095930: ('scoremenu_cash', 'High score per character / track; the gain to the high score and (Tournament) the cash.'),
    0x00095590: ('scoremenu_open_points', 'X: POINTSMENU (0x52afa8 = 6).'),
    0x00095650: ('scoremenu_open_end_menu', 'WINMENU (first, not DNF) / LOSEMENU.'),
    0x00095d80: ('tourney_race_progress', 'A win moves the tournament progress on; the last race wins the tournament.'),
    0x0008bfc0: ('pointsmenu_init', 'POINTSMENU "Points": the breakdown rows.'),
    0x0009fe40: ('upgrade_buy', 'Level + 1 if < 7 and the cost (0x1bc518 [upgrade x 7 + level]) <= the cash.'),
    0x0009f040: ('upgrademenu_texts', 'Upgrade rows: level, cost or Maximum.'),
}


def add_comment(f, cmt):
    old = f.getComment() or ''
    if cmt not in old:
        f.setComment((old + '\n' if old else '') + cmt)


tx = currentProgram.startTransaction('QR score labels')
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
    for a, n in ((0x001bb108, 'g_hiscore_tourney'), (0x001bb150, 'g_hiscore_quick'), (0x001bb0e8, 'g_char_cash'),
                 (0x001bb0ec, 'g_char_upgrades'), (0x001bc518, 'g_upgrade_costs'), (0x001bab30, 'g_profile')):
        createLabel(toAddr(a), n, True, SourceType.USER_DEFINED)
finally:
    currentProgram.endTransaction(tx, True)
print('QR score labels applied: %d' % done)
