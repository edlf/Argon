# Ghidra Jython: FE INTROSCREENS, the boot videos (ms_game_studios.bik, curly_seq.bik, <language>intro.bik) before
# FE TITLE. See docs/frontend/intro-title.md "Intro screens".
from ghidra.program.model.symbol import SourceType
fm = currentProgram.getFunctionManager()
functions = {
    0x00012050: ('fe_introscreens_update', 'FE INTROSCREENS update (vtable 0x19e368 slot 8). Sequence 1: launched by a title (0x52c8bf) -> the title texts laid out, sequence 7; else sequence 5. Sequence 5 by video state +0x1d8: 0 ms_game_studios.bik opened (+0x1dc = 3), 1 its last frame -> curly_seq.bik, 2 -> %s%sintro.bik (+0x1e0 = the language folder), 3 its last frame -> 4, 4 the title texts laid out, sequence 7. Sequence 4 / 7: the last frame drawn (unless 0x52c8bf), shell state 0, FE TITLE.'),
    0x00012630: ('fe_introscreens_input', 'FE INTROSCREENS input (slot 10): A (+0x88 & 0x10) or START (& 0x100) skips the playing video: state 1 -> curly_seq.bik (0x126f8), 2 -> intro.bik (0x1267f), 3 -> state 4 (FE TITLE next update).'),
    0x00012770: ('fe_introscreens_enter', 'FE INTROSCREENS message handler (slot 24), 0xfe00 / 0xfe01: back from the attract demo with a saved page (0x5994c1, not "" / "FE MAIN") -> that page; else +0x1e0 = Locale\\<language>\\ by 0x5293b0 (2 Japanese, 3 German, 4 French, 6 Italian, else English: Spanish plays the English intro), movie textures (0xe0480), video state 0, shell state 1.'),
    0x0001267f: ('fe_introscreens_skip_to_intro', 'Part of fe_introscreens_input (0x12630): the open video closed, %s%sintro.bik started, state 3.'),
    0x000126f8: ('fe_introscreens_skip_to_curly_seq', 'Part of fe_introscreens_input (0x12630): the open video closed, curly_seq.bik started, state 2.'),
    0x000748d0: ('fe_introscreens_ctor', 'FE INTROSCREENS page object (vtable 0x19e368).'),
    0x00079030: ('string_table_open', 'Opens a string table (ESI = its slot, EDI = the file name) from %sLocale\\<language>\\%s by *arg (0x5293b0): 1 English, 2 Japanese, 3 German, 4 French, 6 Italian; any other (Spanish 5) leaves the path unset.'),
}
data = {
    0x0019e368: 'vtable_FeIntroScreens',
    0x0052c8bf: 'g_launched_by_title',   # neon_start 0xb26fb: XGetLaunchInfo found, launch type 0 (a title): no intro videos
    0x005293b0: 'g_language',            # XGetLanguage, 2 (Japanese) forced in the Japanese region
}


def add_comment(f, cmt):
    old = f.getComment() or ''
    if cmt not in old:
        f.setComment((old + '\n' if old else '') + cmt)


tx = currentProgram.startTransaction('QR intro screens labels')
done = 0
try:
    for a, (n, cmt) in sorted(functions.items()):
        f = fm.getFunctionAt(toAddr(a))
        if f is None:
            f = createFunction(toAddr(a), n)
            if f is None:
                print('could not create a function at %x' % a)
                continue
        f.setName(n, SourceType.USER_DEFINED)
        add_comment(f, cmt)
        done += 1
    st = currentProgram.getSymbolTable()
    for a, n in sorted(data.items()):
        st.createLabel(toAddr(a), n, SourceType.USER_DEFINED).setPrimary()
        done += 1
finally:
    currentProgram.endTransaction(tx, True)
print('QR intro screens labels applied: %d' % done)
