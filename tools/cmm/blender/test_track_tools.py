# Headless test of track_tools:
#   blender -b --factory-startup -P test_track_tools.py -- <track> <out_dir>
import bpy, sys, os, filecmp, math, importlib.util
argv = sys.argv[sys.argv.index('--') + 1:]
t, out = argv[0], argv[1]
here = os.path.dirname(os.path.abspath(__file__))
spec = importlib.util.spec_from_file_location('track_tools', os.path.join(here, 'track_tools.py'))
Q = importlib.util.module_from_spec(spec); spec.loader.exec_module(Q)
T = Q.T

bpy.ops.wm.read_factory_settings(use_empty=True)
Q.import_markers(t)

# 1) unchanged write must be byte-identical
d1 = os.path.join(out, 'unchanged')
Q.write_markers(t, d1)
srcs = [os.path.join(T.track_dir(t), f) for f in ('locator.dat', 'sprite.dat', 'prop.dat')] + \
       [T.config_path(k, t) for k in ('ai', 'pup', 'camera')]
bad = [os.path.basename(s) for s in srcs if os.path.exists(s) and
       not filecmp.cmp(s, os.path.join(d1, os.path.basename(s)), shallow=False)]
print('TEST unchanged round-trip:', 'PASS' if not bad else 'FAIL %s' % bad)

# 2) edits
ai = bpy.data.objects['ai_010']; ai.location.x += 5.0
st = [o for o in bpy.data.collections['QR starts'].objects if o['qr_index'] == 0][0]
st.rotation_euler.z += math.radians(10)
pup = [o for o in bpy.data.collections['QR pickups'].objects if o['qr_index'] == 0][0]
pup.location.z += 3.0      # Blender Z = game Y (up)
bpy.context.view_layer.update()
d2 = os.path.join(out, 'edited')
print('TEST wrote', Q.write_markers(t, d2))

# read edited copies back through trackdata by pointing it at the out dir
orig_track_dir, orig_cfg = T.track_dir, T.config_path
T.track_dir = lambda _t: d2
T.config_path = lambda k, _t: os.path.join(d2, os.path.basename(orig_cfg(k, _t)))
n_old, n_new = T.read_ai.__wrapped__(t) if hasattr(T.read_ai, '__wrapped__') else None, T.read_ai(t)
T.track_dir, T.config_path = orig_track_dir, orig_cfg
old = T.read_ai(t)
print('TEST ai_010 x: %.2f -> %.2f' % (old[10]['pos'][0], n_new[10]['pos'][0]))
T.track_dir = lambda _t: d2
T.config_path = lambda k, _t: os.path.join(d2, os.path.basename(orig_cfg(k, _t)))
L_new, p_new = T.read_locators(t)[0], T.read_pups(t)[0]
T.track_dir, T.config_path = orig_track_dir, orig_cfg
L_old, p_old = T.read_locators(t)[0], T.read_pups(t)[0]
print('TEST start0 yaw: %.2f -> %.2f   pos same: %s' % (L_old.rot[1], L_new.rot[1],
      max(abs(a - b) for a, b in zip(L_old.pos, L_new.pos)) < 1e-3))
print('TEST pickup0 y: %.2f -> %.2f' % (p_old['pos'][1], p_new['pos'][1]))
