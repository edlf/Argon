# Ghidra Jython: FE MESSAGEBOX and the front-end UI sound player. See docs/frontend/profiles.md "FE MESSAGEBOX" and
# docs/audio/README.md "Front end"; ported in src/app/app_profile.cpp and audio.cpp.
from ghidra.program.model.symbol import SourceType
fm = currentProgram.getFunctionManager()
names = {
    0x0002b000: ('fe_messagebox_open', 'Opens FE MESSAGEBOX (text, kind 0..0xe, seconds): buttons by kind.'),
    0x0002ad70: ('fe_messagebox_update', 'Draws the text white from (62, 180), 516 wide; shows the buttons.'),
    0x0002b4b0: ('fe_messagebox_input', 'Per kind: A closes / picks, timed kinds close themselves.'),
    0x0002b470: ('fe_messagebox_close', 'Back to the calling page.'),
    0x000c3ca0: ('sound_play_ui', 'Plays sound id (frontend.txt / generic.txt order).'),
    0x000dec40: ('music_set_mode', 'Music mode (3 = the front-end music).'),
}


def add_comment(f, cmt):
    old = f.getComment() or ''
    if cmt not in old:
        f.setComment((old + '\n' if old else '') + cmt)


tx = currentProgram.startTransaction('QR message box / UI sound labels')
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
print('QR message box / sound labels applied: %d' % done)
