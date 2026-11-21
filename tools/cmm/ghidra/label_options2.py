# Ghidra Jython: the Options sub-pages (Music, Credits, Track Records, High Scores, Ghosts). See docs/frontend/README.md
# "Options sub-pages"; ported in src/ui/options.cpp, src/ui/credits.cpp, src/ui/records.cpp.
from ghidra.program.model.symbol import SourceType
fm = currentProgram.getFunctionManager()
names = {
    0x0002b920: ('fe_musicselect_build', 'FE MUSICSELECT: Music Source (Default / Custom), Edit Custom Playlist, Exit at y 120 / 140 / 160.'),
    0x0002c240: ('fe_musicselect_enter', 'Title 0x258; Custom needs 2+ ripped soundtracks (0x440e74), else hidden / grey.'),
    0x0002bd80: ('fe_musicselect_input', 'Left / right music source; A Edit -> FE LEVELMUSIC; Exit / B back.'),
    0x0002c430: ('fe_musicselect_update', 'Leaves to FE OPTIONS SOUND / FE LEVELMUSIC; right hint.'),
    0x0002c660: ('fe_musicselect_ticker', 'Ticker 0x3e8 / 0x3e9 / 0x3ea / 0x3eb / 0x3ec.'),
    0x0002e170: ('fe_credits_enter', 'Clock restart, text from y 50 scrolling after 2 s, picture slots (8 / motion 0, 0 / motion 1).'),
    0x0002e110: ('fe_credits_input', 'A / B leave.'),
    0x0002e390: ('fe_credits_play', 'Pages fe 0xb68 + n centred 0.65 scrolling 1 px a frame; 2 pictures moving / scaling over 8 s (motions 0x1b99a0).'),
    0x0001f9f0: ('fe_fastestlap_build', 'FE FASTESTLAP (Track Records): speed label, headers Track / Fastest Time / Profile, 9 rows.'),
    0x00020fa0: ('fe_fastestlap_enter', 'Title; new-record mode or the viewer; speed = game speed.'),
    0x00020a60: ('fe_fastestlap_input', 'Left / right speed, up / down routes, B back (A saves new records).'),
    0x000206c0: ('fe_fastestlap_update', 'Leaves; draws the frames (0x205f0).'),
    0x0001fef0: ('fe_fastestlap_rows', 'Rows y 180 + 16 i: track at 55, time at 350, profile at 508; record table 0x1bbc34.'),
    0x0001fe10: ('fe_fastestlap_row_time', 'Record time (table +4) formatted at x 350.'),
    0x000205f0: ('fe_fastestlap_frames', 'Header y 155 x 25 and body y 180 x 160 at x 50 / 280 / 430.'),
    0x00024fb0: ('fe_highscores_build', 'FE HIGHSCORES: character, headers Track / Tournament / Quick Race, rows.'),
    0x00026490: ('fe_highscores_enter', 'Title 0x3c0; the current character.'),
    0x00025f20: ('fe_highscores_input', 'Left / right unlocked characters, up / down routes, B back.'),
    0x00025b20: ('fe_highscores_update', 'Leaves to FE OPTIONS / FE MAIN; new-score flash.'),
    0x000254f0: ('fe_highscores_rows', 'Scores from 0x1bb108 / 0x1bb150, n/a; frames third column at 440.'),
    0x000215f0: ('fe_ghostmanage_build', 'FE GHOSTMANAGE: ghost list, no-ghosts message 0x3fd at (62, 150).'),
    0x00021c80: ('fe_ghostmanage_enter', 'Title 0x3fc; no ghosts: message, Back only.'),
    0x00022000: ('fe_ghostmanage_input', 'Up / down, X delete (0x3fe), B back.'),
    0x00021f10: ('fe_ghostmanage_frames', '(52, 100) 536 x 40 and (52, 140) 536 x 160.'),
}
labels = {0x001b99a0: 'g_credits_motions', 0x001bbc34: 'g_track_records'}


def add_comment(f, cmt):
    old = f.getComment() or ''
    if cmt not in old:
        f.setComment((old + '\n' if old else '') + cmt)


tx = currentProgram.startTransaction('QR options sub-page labels')
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
print('QR options sub-page labels applied: %d' % done)
