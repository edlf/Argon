# Ghidra Jython: FE COURSE SELECT (track select) and the .cmm bone animation.
# See docs/frontend/race-setup.md "Track select" and docs/models/meshes.md "Animation"; ported in src/ui/charsel.cpp
# (course page) and src/model/cmm.cpp (Cmm::pose / Cmm::skin).
from ghidra.program.model.symbol import SourceType
fm = currentProgram.getFunctionManager()
names = {
    0x00075400: ('fe_course_ctor', 'FE COURSE SELECT (vtable 0x19de18): 9 row texts (+0x1d4, 0x20c each) and 2 route texts (+0x162c / +0x1838).'),
    0x0001cfb0: ('fe_course_build', 'Rows at (62, 100 + 30 i) scale 0.6 with panels; route buttons right-aligned at 568, y 300 / 330; list cursor +0x1440.'),
    0x0001da10: ('fe_course_input', 'B: close the route choice / back to FE CHARSELECT. A: rows -> fe_course_pick; route buttons -> id i / i + 8 (Mars 0x10 / 0x11) -> fe_course_leave. Y: FE TRACK INFO (name from 0x1b9538).'),
    0x0001e410: ('fe_course_cursor_row', 'Row under the cursor: 0..7 tracks, -1 Mars row (or a route button while Mars), -2 none.'),
    0x0001e380: ('fe_course_ticker', 'Description: empty; locked Draco fe 0x183, locked Mars fe 0x184.'),
    0x0001e5e0: ('fe_course_pick', 'Locked: buzz. Second route unlocked (mask bit id + 8; Mars always): open the route choice, else leave with the track.'),
    0x0001e4d0: ('fe_course_route_choice', 'Shows the two route buttons: fe 900 + i / 0x38c + i ("1: Tara\'s Route" / "2: Alicia\'s Route"), Mars 0x394 / 0x395.'),
    0x0001e6a0: ('fe_course_slide_rows', 'Rows slide between x 640 and 62 (52 + 10), 10 frames; arg 1 = in.'),
    0x0001e920: ('fe_course_leave', 'Rows out, sequence 7 -> FE SPEEDSELECT (fe_course_update).'),
    0x00019f20: ('profile_unlock_all_tracks', 'Track mask 0x1bbbfc = 0xffffffff, Mars 0x1bbc00 = 1.'),
    0x000b8630: ('cmm_bone_animate', 'Per bone (track index bone +0x100): frame = clock (+0x5e4, or bone +0x188 with flag 0x80) x fps; keys floor % n and +1, lerp / slerp (0xf360b) -> 0xf3035, rows x bone +0x11c..; world = local x parent. Recurses to child (+0x182, flag 0x100) and sibling (+0x184, flag 8).'),
    0x000f2ec4: ('mat_rotation_x', 'D3D row-vector rotation about X.'),
    0x000f2f3e: ('mat_rotation_y', 'D3D row-vector rotation about Y.'),
    0x000f2fb9: ('mat_rotation_z', 'D3D row-vector rotation about Z.'),
    0x000f2936: ('mat_multiply', 'out = a x b (4x4).'),
    0x000f33fe: ('quat_from_matrix', 'Quaternion from a rotation matrix (D3DX formula).'),
    0x000f360b: ('quat_slerp', 'Slerp of two quaternions.'),
    0x000f3035: ('matrix_from_quat', 'Rotation matrix from a quaternion.'),
}
labels = {
    0x001b9a18: 'g_course_globe_turn',
    0x001b9538: 'g_track_folder',
    0x001bbbfc: 'g_track_unlock_mask',
    0x001bbc00: 'g_mars_unlocked',
    0x0050e430: 'g_fe_clock_seconds',
}

def add_comment(f, cmt):
    old = f.getComment() or ''
    if cmt not in old:
        f.setComment((old + '\n' if old else '') + cmt)


tx = currentProgram.startTransaction('QR course select + animation labels')
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
    f = fm.getFunctionAt(toAddr(0x000b6070))
    if f: add_comment(f, 'rec80 = animation tracks: +4 flags (1 pos 12 B/key, 4 Euler deg 16 B/key -> quaternion in place, 2 scale 12 B/key), +8 keys, +0xc fps, +0x10 bone name; keys after all records.')
    for a, n in labels.items():
        createLabel(toAddr(a), n, True, SourceType.USER_DEFINED)
finally:
    currentProgram.endTransaction(tx, True)
print('QR course select + animation labels applied: %d' % done)
