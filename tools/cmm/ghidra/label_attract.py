# Ghidra Jython: the front end's attract demo (gl_attractdemo.cpp, replays d:\replays\replay1..3). See
# docs/frontend/intro-title.md "Attract demo".
from ghidra.program.model.symbol import SourceType
fm = currentProgram.getFunctionManager()
names = {
    0x00031c10: ('fe_shell_start_attract', 'After 75 s idle (fe_shell_update 0x307d3): shell +0x1630 = 1, the page name saved to +0x1631 (0x5994c1, the return page), replay<n>.info parsed (track 0x52ccf8, pilots 0x52cdc0[]), game mode 0x52ccfc = 2, 1 player, 6 craft, 5 drones, LOADER.'),
    0x00040440: ('attract_init', 'CAttractDemo (vtable 0x1a1a0c, game mode 2) init: hud0 "set none", z:\\replay<n> copied from d:\\replays, CReplay "demo", camera0 "replay on" / "replay0output", 0x52c8b4 = next replay (0..2).'),
    0x00040af0: ('attract_update', 'Advances the event buffer (0xdb260), craft_replay_inputs per craft; GAMELOGIC message 7 (quit to FE) on replay end, START / BACK / A B X Y White Black / stick clicks / analog 0x88380 / a new pad.'),
    0x00040d50: ('attract_shutdown', 'Frees the crafts, the event buffer and the "demo" replay.'),
    0x00040310: ('attract_create_crafts', 'Craft 0 a player craft (vtable 0x19f3e8), the others drones (vtable 0x1a1c10).'),
    0x000402b0: ('msg_quit_to_frontend_id', 'Message id 7 (GAMELOGIC: back to the front end).'),
    0x00088450: ('pads_any_pressed_bit', 'Any of the 4 pads has 1 << n newly pressed (+0x88).'),
    0x00088380: ('pads_any_analog', 'Any pad has +0x7c or +0x80 above 0.'),
    0x000885a0: ('pads_new_inserted', 'A controller has just been inserted (0x1ef638[i] latch).'),
    0x000db260: ('eventbuffer_advance', 'CEventBuffer::Advance(t) (eventbuffer.cpp; mode +8: 0 -> 0xdad40, 1 -> 0xdab80, 2 unreadable); non-zero ends the attract demo.'),
}


def add_comment(f, cmt):
    old = f.getComment() or ''
    if cmt not in old:
        f.setComment((old + '\n' if old else '') + cmt)


tx = currentProgram.startTransaction('QR attract demo labels')
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
print('QR attract demo labels applied: %d' % done)
