# Ghidra Jython: the widescreen layout constants. See docs/frontend/README.md "Widescreen layout".
from ghidra.program.model.symbol import SourceType
labels = {
    0x00194fd4: ('g_ui_x_scale_16x9', '0.75: widescreen x squeeze of the 640 layout (undone by the 16:9 stretch); camera aspect / this.'),
    0x00194fd8: ('g_ui_x_offset_16x9', '80: widescreen x shift of the squeezed layout; safe margin there = 0.75 x 52 - this = -41.'),
    0x001a3580: ('g_ui_safe_margin', '52: the 4:3 safe-area margin of the front end / HUD layouts (items at margin + dx).'),
}

tx = currentProgram.startTransaction('QR widescreen labels')
try:
    for a, (n, cmt) in sorted(labels.items()):
        createLabel(toAddr(a), n, True, SourceType.USER_DEFINED)
        setEOLComment(toAddr(a), cmt)
finally:
    currentProgram.endTransaction(tx, True)
print('QR widescreen labels applied: %d' % len(labels))
