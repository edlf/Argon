# Ghidra Jython: NMS sequences (data\movies\*.nms). See docs/sequences/README.md; ported in src/movie/nms_sequence.cpp.
from ghidra.program.model.symbol import SourceType
fm = currentProgram.getFunctionManager()
names = {
    0x00095e70: ('seq_load_track_sequences', 'Loads data\\movies\\<t>\\<t>_glory.nms (0x1ba9e8) and <t>_racestart.nms (0x1ba9f4).'),
    0x00095ef0: ('neon_start_glory_sequence', 'Starts the glory sequence (the loading screen flyby) via seq_start.'),
    0x00095fa0: ('seq_reset', 'Frees the previous sequence and its actors / speech.'),
    0x00096230: ('seq_start_copy', 'Copies a sequence into "NMS Sequence" memory and links objects, curves, CMS data, tracks.'),
    0x00096690: ('seq_start', 'Links a loaded sequence: objects 0x7f0 from +0x1210 (count +0x204), curves, dopesheet tracks.'),
    0x000969c0: ('seq_load_track_model', 'Loads tracks\\%s and its sky for the sequence.'),
    0x000971d0: ('seq_track_segment', 'Segment at time t (last of all but the final one); its object and remapped time.'),
    0x000972a0: ('seq_track_segment_index', 'Index of the segment at time t.'),
    0x000972f0: ('seq_track_update', 'Kind 1 cameras (eye / look / fov, transition on a cut), 2 actor animation, 3 speech.'),
    0x00097740: ('seq_rotation_matrix', 'Scale x rotation (degrees) into the local matrix.'),
    0x00097d40: ('seq_local_matrix', 'Local matrix from +0x370 scale / rotation / translation.'),
    0x00097e90: ('seq_world_matrix', 'World = local x parent world, recursing children and siblings.'),
    0x00098030: ('seq_object_matrix', 'Camera position aimed at its interest, then the local matrix.'),
    0x00098190: ('seq_object_length', 'Object length +0x3ac = last key time (min 0.1), raised into the camera.'),
    0x000984e0: ('seq_curve_slot', 'Curve pointer of slot 1..10 (+0x3c0..).'),
    0x000985e0: ('seq_curve_range', 'First / last key time into curve +0x10 / +0x14.'),
    0x000986c0: ('seq_bezier', 'Cubic Bezier.'),
    0x00098720: ('seq_curve_eval', 'Curve value at t: clamped ends, Bezier between keys with weighted slopes / 3.'),
    0x00098900: ('seq_object_eval', 'Object (and partner) curves -> +0x370..+0x398.'),
    0x000988b0: ('seq_eval_children', 'seq_object_eval over a child chain.'),
    0x00098b70: ('seq_update', 'Each frame: t = fmod(clock, first track length), tracks, curves, world matrices.'),
}


def add_comment(f, cmt):
    old = f.getComment() or ''
    if cmt not in old:
        f.setComment((old + '\n' if old else '') + cmt)


tx = currentProgram.startTransaction('QR NMS sequence labels')
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
print('QR NMS sequence labels applied: %d' % done)
