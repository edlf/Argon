# Ghidra Jython: the race start (loader -> grid intro camera -> countdown). See docs/frontend/loading.md "Loading screen" and
# "Race intro camera".
from ghidra.program.model.symbol import SourceType
fm = currentProgram.getFunctionManager()
names = {
    0x000802c0: ('loader_run', 'LOADER: starts the loader thread (0x808e0 each frame), loads the track (0x885e0), waits for A; no 3D drawn.'),
    0x00043d60: ('race_intro_start', '0x52af9c = 1; 0x52af9d (grid intro camera) = 1 unless 0x52af9f or mode 3 / 4, only the first race after a load (0x1b9788).'),
    0x00043de0: ('race_intro_update', 'Grid intro: camera on craft (crafts - t / 2) for 2 s each, last the player eased in; countdown 3/2/1 at clock 1/2/3, music_set_mode(1) at GO.'),
    0x0000dec40: ('music_set_mode', '0x4406f0 +0x6ac: 0 intro.wav, 1 on_start / playlist, 3 playlist (per track flags +0x6e4).'),
    0x00043be0: ('countdown_init', '"Countdown" hud group: full-screen viewport, camera (0,0,-10); 0x1b9788 = 1 (grid intro on the next race).'),
    0x0003f1d0: ('race_clock_reset', 'Not modes 3 / 4: the race clock to 0 (the grid intro ends, the countdown starts).'),
    0x000c38b0: ('countdown_sound_three', 'Once (+0x8ec): SF_<track>_three.'),
    0x000c3920: ('countdown_sound_two', 'Once (+0x8ed): SF_<track>_two.'),
    0x000c3990: ('countdown_sound_one', 'Once (+0x8ee): SF_<track>_one.'),
    0x000c3a00: ('countdown_sound_go', 'SF_<track>_go.'),
    0x00078270: ('pad_set_control', 'ESI = pad: control flag 0x3510 + pad (with its vibration setting).'),
    0x000ae550: ('camera_set_fov', 'EAX = camera: fov (+0x2d0) and the projection (+0x1e0); aspect +0x2d4 / 0.75 with g_widescreen.'),
    0x000ae5c0: ('camera_set_projection', 'EAX = camera: fov, aspect, near, far (+0x2d0..); aspect / 0.75 with g_widescreen; 0x52c8bc mirrors x.'),
    0x000ae640: ('camera_set_eye', 'ESI = camera: eye +0x36c (countdown: (0, 0, -10)).'),
    0x000f32f8: ('matrix_perspective_fov_rh', '(out, fovy, aspect, near, far): right-handed (m23 -1).'),
    0x000f3224: ('matrix_look_at_lh', '(out, eye, at, up): D3DXMatrixLookAtLH.'),
    0x000abb80: ('hud_model_load', 'EAX = hud model, EBX = path: loads the cmm, world matrix +0x7f0 = identity.'),
}
data_labels = {
    0x00298ecc: ('g_widescreen', 'Widescreen flag (dashboard setting): camera aspects / 0.75 (16:9), UI layouts.'),
}


def add_comment(f, cmt):
    old = f.getComment() or ''
    if cmt not in old:
        f.setComment((old + '\n' if old else '') + cmt)


tx = currentProgram.startTransaction('QR race intro labels')
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
    for a, (n, cmt) in sorted(data_labels.items()):
        createLabel(toAddr(a), n, True, SourceType.USER_DEFINED)
        setEOLComment(toAddr(a), cmt)
        done += 1
finally:
    currentProgram.endTransaction(tx, True)
print('QR race intro labels applied: %d' % done)
