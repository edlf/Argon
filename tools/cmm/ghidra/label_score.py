# Ghidra Jython: scoring (points awards, popup queue, air time). See docs/physics/README.md (HUD, Score);
# ported in src/app/app_pickups.cpp, app_drive.cpp and hud/hud_elements.cpp (HudPointsPopup).
from ghidra.program.model.symbol import SourceType
fm = currentProgram.getFunctionManager()
names = {
    0x00083f00: ('score_award', '(score, kind, points, extra): posts a HUD event type 4 (class 0x1a1d64) -> score target + popup.'),
    0x00083e70: ('score_powerup', 'Power-up pickup: count +0x24, score_award(kind 9, cashmodifiers pickup_bonus).'),
    0x00083ea0: ('score_powerchip', 'Power chip: count +0x28, score_award(kind 8, powerchip_bonus).'),
    0x00083ed0: ('score_megaup', 'Mega-up: count +0x2c, score_award(kind 10, megaup_bonus).'),
    0x00083d80: ('score_lap', 'Lap under the trackgoals lap time goal [track][speed] (0x1bc5ac): (goal - lap) x 100 x lap_time_bonus, kind 4.'),
    0x00083e00: ('score_hit', 'Weapon hit / kill: kind 2 (kills_bonus) or 11 (x2).'),
    0x0004cb00: ('hud_popup_add', 'Points popup queue (max 32): merge same kind, 0 points only for kinds 12..15. docs/physics/README.md (HUD).'),
    0x0004cc30: ('hud_popup_update', '0.8 s per entry; points / label zoom (7,6)/(3.5,3) x age, rise, fade; echo copy after half.'),
    0x00049360: ('hud_score_reset', 'Score shown / target = 0.'),
}
comments = {
    0x0005e6d0: 'On landing (0x5f717, player): air time +0x938 - 1 s > 0 -> int(t x 100) x air_time_bonus points (kind 6).',
}
labels = {
    0x001bc4c0: 'g_cash_modifiers',
    0x001b9748: 'g_popup_label_ids',
    0x001bc570: 'g_score',
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
            print('created function %s at %x' % (n, a))
        f.setName(n, SourceType.USER_DEFINED)
        add_comment(f, cmt)
        done += 1
    for a, cmt in sorted(comments.items()):
        f = fm.getFunctionAt(toAddr(a))
        if f is None:
            print('no function at %x' % a)
            continue
        add_comment(f, cmt)
        done += 1
    st = currentProgram.getSymbolTable()
    for a, n in sorted(labels.items()):
        st.createLabel(toAddr(a), n, SourceType.USER_DEFINED)
        done += 1
    print('QR score labels applied: %d' % done)
finally:
    currentProgram.endTransaction(tx, True)
