# Ghidra Jython: the Quick Race end (finish order, POSTABLE, WINMENU / LOSEMENU). See docs/frontend/race-end.md "Race end";
# ported in src/app/app_results.cpp. Run after fix_msg_noreturn.py (which rebuilt these bodies).
from ghidra.program.model.symbol import SourceType
fm = currentProgram.getFunctionManager()
names = {
    0x00062ba0: ('race_manager_update', 'Per racer: progress, lap counter (+0x10, from 1), finish at laps + 1 (0x63850).'),
    0x00063850: ('race_finish_order_add', 'Appends a finished racer to the finish order (+0x514).'),
    0x000638b0: ('race_finish_order_dnf', 'A knocked-out racer from the back of the finish order; "dnf" to its HUD.'),
    0x00063820: ('race_finish_place', 'A racer\'s 1-based place in the finish order, -1 if not finished.'),
    0x00063c40: ('race_mark_dnf', 'Marks a racer DNF (+0x6a) unless it finished.'),
    0x00058d20: ('craft_retire', 'Craft finished / out: +0x1145, its state parked.'),
    0x0003fba0: ('gamelogic_update_quickrace', 'Player lap counter == laps + 1: race time, POSTABLE (0x52afa8 = 5).'),
    0x00041490: ('gamelogic_update_mode2', 'As 0x3fba0 for another mode.'),
    0x0003b920: ('gamelogic_update_common', '20-minute limit (0x1b9790), messages, crafts.'),
    0x0003bf60: ('gamelogic_post_race_input', 'Pad -> POSTABLE (5) / WINMENU, LOSEMENU (1 / 2) / multiplayer (7).'),
    0x0008dbb0: ('postable_init', 'POSTABLE: "Results", headers at y 165, columns 67 / 283 / 403 / 540.'),
    0x0008ea30: ('postable_update', 'POSTABLE ON: HUD off, place / Congratulations / Try Again; A closes.'),
    0x0008f140: ('postable_rows', 'Rows in finish order: place: name, race time (estimated if racing), lap time, kills.'),
    0x0008fa90: ('postable_close', 'A: to WINMENU / LOSEMENU.'),
    0x0008d610: ('postable_open_end_menu', 'WINMENU if the player finished first and not DNF, else LOSEMENU.'),
    0x000845b0: ('losemenu_init', 'LOSEMENU: "You Lost", 4 items at y 151 + 35 n, confirm page.'),
    0x00084b30: ('losemenu_update', 'LOSEMENU ON / OFF / QUIT; up / down / A; music fade.'),
    0x00085a50: ('losemenu_items', 'Items: Race Again, View Replay, Track Select, Main Menu (Tournament: Upgrade Menu ...).'),
    0x00085bf0: ('losemenu_confirm', 'Are you sure? Yes / No for the chosen item.'),
    0x000a19d0: ('winmenu_confirm', 'Are you sure? for Race Again / Track Select / Main Menu / Next Race.'),
}


def add_comment(f, cmt):
    old = f.getComment() or ''
    if cmt not in old:
        f.setComment((old + '\n' if old else '') + cmt)


tx = currentProgram.startTransaction('QR race end labels')
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
    createLabel(toAddr(0x00596db0), 'g_race_manager', True, SourceType.USER_DEFINED)
    createLabel(toAddr(0x0052afa8), 'g_post_race_state', True, SourceType.USER_DEFINED)
    createLabel(toAddr(0x0052cdd8), 'g_race_laps', True, SourceType.USER_DEFINED)
finally:
    currentProgram.endTransaction(tx, True)
print('QR race end labels applied: %d' % done)
