# Check markers sit on the track: raycast down from each start / AI node / pickup.
#   blender -b --factory-startup -P test_alignment.py -- <track>
import bpy, sys, os, importlib.util
from mathutils import Vector
t = sys.argv[sys.argv.index('--') + 1:][0]
here = os.path.dirname(os.path.abspath(__file__))
spec = importlib.util.spec_from_file_location('track_tools', os.path.join(here, 'track_tools.py'))
Q = importlib.util.module_from_spec(spec); spec.loader.exec_module(Q)
bpy.ops.wm.read_factory_settings(use_empty=True)
Q.import_track_model(t, sky=False)
Q.import_markers(t)
dg = bpy.context.evaluated_depsgraph_get()
for key in ('start', 'ai', 'pup'):
    ds, miss = [], 0
    for o in bpy.data.collections[Q.COLL[key]].objects:
        if o.type != 'EMPTY':
            continue
        p = o.matrix_world.translation + Vector((0, 0, 20))
        hit, loc, *_ = bpy.context.scene.ray_cast(dg, p, Vector((0, 0, -1)), distance=200)
        if hit:
            ds.append((p - loc).length - 20)
        else:
            miss += 1
    ds.sort()
    if ds:
        print('ALIGN %-5s n=%3d  height above surface: median %.2f  90%% %.2f  no-hit %d'
              % (key, len(ds), ds[len(ds) // 2], ds[int(len(ds) * .9)], miss))
