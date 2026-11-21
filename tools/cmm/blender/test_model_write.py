# Headless test of track_tools' model writing (Blender -> model.cmm through tools/cmm/gltf2cmm.py):
#   blender -b --factory-startup -P test_model_write.py -- <track> <out_dir>
# Prints TEST lines: an unchanged write keeps every mesh's triangles and colours; a moved mesh comes back moved.
import bpy, sys, os, importlib.util
argv = sys.argv[sys.argv.index('--') + 1:]
track, out = argv[0], argv[1]
here = os.path.dirname(os.path.abspath(__file__))
spec = importlib.util.spec_from_file_location('track_tools', os.path.join(here, 'track_tools.py'))
Q = importlib.util.module_from_spec(spec); spec.loader.exec_module(Q)
from cmm import CMM   # noqa: E402 (the add-on put tools\cmm on the path)

ROUND = 3   # mm: the glTF round trip goes through Blender's float32 Z-up conversion


def triangle_corners(model, mesh):
    """{triangle's corner positions (rounded, ordered): the corners' D3DCOLOR bytes of every such triangle, sorted}."""
    vertices, colours = model.vertices(mesh), model.colours(mesh)
    out = {}
    for t in model.triangles(mesh):
        corners = sorted((tuple(round(x, ROUND) for x in vertices[i][0]), colours[i]) for i in t)
        out.setdefault(tuple(p for p, _ in corners), []).append(tuple(c for _, c in corners))
    return {key: sorted(found) for key, found in out.items()}


def colour_error(before, after):
    """The largest channel difference between the corners of triangles at the same place (duplicates included)."""
    return max((abs(a - b) for key, colours in before.items() for ta, tb in zip(colours, after[key])
                for ca, cb in zip(ta, tb) for a, b in zip(ca, cb)), default=0)


def same_triangles(before, after):
    return {k: len(v) for k, v in before.items()} == {k: len(v) for k, v in after.items()}


bpy.ops.wm.read_factory_settings(use_empty=True)
Q.import_track_model(track, sky=False)
original = os.path.join(Q.T.track_dir(track), 'model.cmm')
before = CMM(original)
exported = [m for m in before.meshes if m.vert_ofs is not None and not m.flags & 0x40000]

written = Q.write_models(os.path.join(out, 'unchanged'))
after = CMM(written[0])
corners = {m.index: (triangle_corners(before, m), triangle_corners(after, after.meshes[m.index])) for m in exported}
moved_triangles = [i for i, (b, a) in corners.items() if not same_triangles(b, a)]
print('TEST unchanged triangles:', 'PASS' if not moved_triangles else 'FAIL meshes %s' % moved_triangles[:10])
worst = max(colour_error(b, a) for b, a in corners.values())
print('TEST unchanged colours: worst channel difference %d' % worst, 'PASS' if worst <= 1 else 'FAIL')
print('TEST bytes after the index data kept:',
      'PASS' if before.data[before.index_data_end:] == after.data[after.index_data_end:] else 'FAIL')

target = next(o for o in bpy.data.objects if o.type == 'MESH' and 'qr_mesh' in o.data)
index = int(target.data['qr_mesh'])
for v in target.data.vertices:
    v.co.z += 1.0   # Blender Z = the game's Y (up)
edited = CMM(Q.write_models(os.path.join(out, 'edited'))[0])
lift = [round(a[0][1] - b[0][1], ROUND) for a, b in
        zip(sorted(edited.vertices(edited.meshes[index])), sorted(before.vertices(before.meshes[index])))]
print('TEST mesh %d lifted 1 m:' % index, 'PASS' if lift and all(abs(d - 1.0) < 1e-3 for d in lift) else 'FAIL %s' % lift[:5])
