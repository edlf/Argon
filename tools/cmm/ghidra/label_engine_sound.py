# Ghidra Jython: engine sound names (see docs\audio\README.md, ported in src\audio\engine_voices.cpp).
from ghidra.program.model.symbol import SourceType
fm = currentProgram.getFunctionManager()
names = {
    0x000c5200: ('engine_sound_update', 'Per craft and engine set (sound +0x9ec normal, +0xa04 h_ set): S (+0xa1c) follows |forward speed| (dataport +0x28 -> craft +0x990)\n'
                 'by at most 1.0 per call; x = 100*S / (absolute_max_speed[speed class] * 0.27778), clamped +0xa38..+0xa3c (0..500);\n'
                 'per layer: volume = 8-point Vol curve (x +0x24.., mB +0x44..) clamped -10000..0, pitch = 4-point Pitch curve (x +0x04.., +0x14..)\n'
                 'clamped +-8142; mute arg -> -10000; volume scaled by the SFX option (0xc7990); 3D position/velocity (craft 0 = listener).'),
    0x000c7450: ('engine_ini_load', 'Parses audio\\ships\\<p>\\engine\\<p>.ini (or h_engine\\h_<p>.ini when the last arg is 1): Loop / file / Vol_i / Pitch_i,\n'
                 '0x98-byte layers; sets the clamps x 0..500, volume -10000..0, pitch +-8142.'),
    0x000c5700: ('craft_sound_bind', 'Binds "craft%d sound", loads the engine set for every craft and the h_engine set for craft 0 (not in mode 4).'),
    0x000c4bf0: ('sound_frame_update', 'Per frame: message flags, engine_sound_update for both sets of every craft (muted by +0x66e / +0x66f),\n'
                 'rain, reverb (I3DL2 by node flags), CommitDeferredSettings.'),
    0x000c7990: ('sound_apply_volume_option', 'Scales a mB volume by the player sound-effect volume option.'),
    0x000c9890: ('sound_load_f_settings', 'Reads audio\\f_settings.txt (impact / scrape thresholds).'),
}
tx = currentProgram.startTransaction('QR engine sound labels')
done = 0
try:
    for a, (n, cmt) in names.items():
        f = fm.getFunctionAt(toAddr(a))
        if f is None:
            print('no function at %x' % a); continue
        f.setName(n, SourceType.USER_DEFINED)
        old = f.getComment() or ''
        if cmt not in old:
            f.setComment((old + '\n' if old else '') + cmt)
        done += 1
    print('QR engine sound labels applied: %d' % done)
finally:
    currentProgram.endTransaction(tx, True)
