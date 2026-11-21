# Ghidra Jython: FE SPEEDSELECT and FE TRACK INFO. See docs/frontend/race-setup.md "Select Speed and Track Data"; ported in
# src/ui/speed.cpp.
from ghidra.program.model.symbol import SourceType
fm = currentProgram.getFunctionManager()
names = {
    0x00033560: ('fe_speedselect_build', 'FE SPEEDSELECT: infotext 0xfa + i at (62, 100 + 40 i), font 1 x 0.75.'),
    0x00034200: ('fe_speedselect_enter', 'Title 0x2bc; cursor = game speed capped at 0x1bb0e4; locked rows grey.'),
    0x00033ac0: ('fe_speedselect_input', 'Up / down wrap, A sets the speed on an open row, B back.'),
    0x00033fa0: ('fe_speedselect_update', 'Leaves to FE COURSE SELECT / FE GHOSTSELECT / FE GOALDISPLAY / LOADER; draws a shell model.'),
    0x000343e0: ('fe_speedselect_ticker', 'Locked speed: fe 0x116 + speed, else none.'),
    0x00036a60: ('fe_trackinfo_build', 'FE TRACK INFO: right-aligned texts at 578 (y 90..240), description at (62, 280), arrows.'),
    0x00037370: ('fe_trackinfo_enter', 'Track name, location / length (strings 0x640 + 10 t), home character, nemesis, description.'),
    0x000371e0: ('fe_trackinfo_input', 'B back, up / down scroll the description by 4 lines.'),
    0x00037800: ('fe_trackinfo_update', 'Leaves to the previous page.'),
    0x00036840: ('fe_trackinfo_draw', 'Rotated quad (320, 150) and the frame (52, 265) 536 x 125.'),
    0x00078ad0: ('infotext_character_name', 'Copies infotext 200 + id.'),
}


def add_comment(f, cmt):
    old = f.getComment() or ''
    if cmt not in old:
        f.setComment((old + '\n' if old else '') + cmt)


tx = currentProgram.startTransaction('QR speed select / track info labels')
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
print('QR speed / track info labels applied: %d' % done)
