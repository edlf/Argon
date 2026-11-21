bl_info = {
    "name": "Quantum Redshift track tools",
    "author": "QR modding",
    "version": (0, 1),
    "blender": (4, 0, 0),
    "location": "3D View > Sidebar > QR",
    "description": "Load QR tracks with start grid, AI nodes, pickups, cameras, sprites "
                   "and fog/light settings; write marker and mesh edits back to the game files",
    "category": "Import-Export",
}

# Install: Edit > Preferences > Add-ons > Install from Disk > this file
# (keep it in the project's tools\cmm\blender folder: it finds game_data from here).
#
# Coordinates: game (x, y, z) [right-handed in effect, Y up]  <->  Blender (x, -z, y),
# i.e. exactly what Blender's glTF importer does to the (unmirrored) .glb exports.

import math, os, shutil, sys
import bpy
from mathutils import Matrix, Vector

HERE = os.path.dirname(os.path.abspath(__file__))
TOOLS = os.path.dirname(HERE)
if TOOLS not in sys.path:
    sys.path.insert(0, TOOLS)
import trackdata as T   # noqa: E402

QR_MODEL = 'qr_model'   # custom property: the model.cmm an imported object came from (the track's or its sky's)
COLL = {'start': 'QR starts', 'ai': 'QR AI nodes', 'pup': 'QR pickups',
        'cam': 'QR cameras', 'spr': 'QR sprites'}


def g2b(p):
    return Vector((p[0], -p[2], p[1]))


def b2g(v):
    return [v[0], v[2], -v[1]]


# Blender = Q * game (a rotation about X, no mirror)
Q = Matrix(((1, 0, 0, 0), (0, 0, -1, 0), (0, 1, 0, 0), (0, 0, 0, 1)))
QT = Q.transposed()


def game_matrix_to_blender(m):
    M = Matrix([list(r) for r in m]).transposed()   # row-vector -> column-vector
    return Q @ M @ QT


def blender_to_game_rot3(mw):
    """Blender world matrix -> game row-vector 3x3 rotation."""
    return (QT @ mw.to_3x3().normalized().to_4x4() @ Q).transposed()


def blender_to_game_rot(mw, near=(0, 0, 0)):
    """Game euler (x, y, z) degrees for Rx*Ry*Rz; of the two equivalent
    solutions, return the one closest to `near` (the original angles)."""
    R = blender_to_game_rot3(mw)
    y = math.asin(max(-1.0, min(1.0, -R[0][2])))
    sols = []
    for yy in (y, math.pi - y):
        cy = math.cos(yy) or 1e-9
        p = math.atan2(R[1][2] / cy, R[2][2] / cy)
        r = math.atan2(R[0][1] / cy, R[0][0] / cy)
        sols.append([math.degrees(p), math.degrees(yy), math.degrees(r)])
    def unwrap(a, ref):
        return a + 360.0 * round((ref - a) / 360.0)
    sols = [[unwrap(a, n) for a, n in zip(s, near)] for s in sols]
    return min(sols, key=lambda s: sum((a - n) ** 2 for a, n in zip(s, near)))


def collection(name, clear=False):
    c = bpy.data.collections.get(name)
    if c is None:
        c = bpy.data.collections.new(name)
        bpy.context.scene.collection.children.link(c)
    elif clear:
        for o in list(c.objects):
            bpy.data.objects.remove(o, do_unlink=True)
    return c


def empty(name, coll, loc, shape='PLAIN_AXES', size=2.0, **props):
    o = bpy.data.objects.new(name, None)
    o.empty_display_type, o.empty_display_size = shape, size
    o.location = loc
    for k, v in props.items():
        o[k] = v
    coll.objects.link(o)
    return o


def tracks():
    root = os.path.join(T.GAME, 'data', 'tracks')
    return sorted(d for d in os.listdir(root) if os.path.isfile(os.path.join(root, d, 'model.cmm')))


# ------------------------------------------------------------------ import
def import_track_model(t, sky=True):
    import cmm2gltf
    for sub, src in (('tracks', T.track_dir(t)), ('tracks_sky', os.path.join(T.track_dir(t), 'sky'))):
        if sub == 'tracks_sky' and not sky:
            continue
        glb = os.path.join(TOOLS, 'export', sub, t + '.glb')
        if not os.path.exists(glb):
            os.makedirs(os.path.dirname(glb), exist_ok=True)
            cmm2gltf.convert(src, glb)
        before = set(bpy.data.objects)
        bpy.ops.import_scene.gltf(filepath=glb)
        for o in set(bpy.data.objects) - before:
            o[QR_MODEL] = os.path.join(src, 'model.cmm')


# ------------------------------------------------------------------ write the models
def write_models(out_dir=None):
    """Write the imported models' meshes back into their model.cmm (the geometry only: tools/cmm/gltf2cmm.py).
    out_dir=None writes the game files in place (the first write keeps a .orig copy next to each)."""
    import tempfile
    import gltf2cmm
    written = []
    for model in sorted({o[QR_MODEL] for o in bpy.data.objects if QR_MODEL in o}):
        dest = os.path.join(out_dir, os.path.basename(os.path.dirname(model)) + '.cmm') if out_dir else model
        if out_dir:
            os.makedirs(out_dir, exist_ok=True)
        else:
            _backup(model)
        with tempfile.TemporaryDirectory() as folder:
            glb = os.path.join(folder, 'edited.glb')
            export_model_objects(model, glb)
            gltf2cmm.write(model, glb, dest)
        written.append(dest)
    return written


def export_model_objects(model, glb):
    """The objects imported from one model as a .glb, with the meshes' qr_mesh extras."""
    bpy.ops.object.select_all(action='DESELECT')
    for o in bpy.data.objects:
        if o.get(QR_MODEL) == model:
            o.select_set(True)
    bpy.ops.export_scene.gltf(filepath=glb, export_format='GLB', use_selection=True, export_extras=True,
                              export_materials='NONE', export_vertex_color='ACTIVE', export_yup=True)


def import_markers(t):
    sc = bpy.context.scene
    sc['qr_track'] = t
    c = collection(COLL['start'], True)
    for i, L in enumerate(T.read_locators(t)):
        o = empty('start_%d_%s' % (i, L.name), c, (0, 0, 0), 'SINGLE_ARROW', 4.0, qr_index=i)
        o.matrix_world = game_matrix_to_blender(L.matrix)
        o['qr_scale'] = L.scale

    c = collection(COLL['ai'], True)
    if os.path.exists(T.config_path('ai', t)):
        nodes = T.read_ai(t)
        for i, n in nodes.items():
            empty('ai_%03d' % i, c, g2b(n['pos']), 'SPHERE', 1.5, qr_index=i,
                  qr_flags=' '.join(n['flags']))
        build_ai_links(t, nodes)

    c = collection(COLL['pup'], True)
    if os.path.exists(T.config_path('pup', t)):
        for i, p in enumerate(T.read_pups(t)):
            empty('pup_%02d_%s' % (i, p['type']), c, g2b(p['pos']), 'CUBE', 2.0, qr_index=i,
                  qr_type=p['type'])

    c = collection(COLL['cam'], True)
    if os.path.exists(T.config_path('camera', t)):
        for i, cam in enumerate(T.read_cameras(t)):
            if cam['pos'] is None:
                continue
            cd = bpy.data.cameras.new('cam_%02d' % i)
            if cam['fov']:
                cd.angle = math.radians(cam['fov'])
            cd.clip_end = 20000
            co = bpy.data.objects.new('cam_%02d_%s' % (i, cam['name']), cd)
            co.location, co['qr_index'] = g2b(cam['pos']), i
            c.objects.link(co)
            if cam['target']:
                tg = empty('cam_%02d_target' % i, c, g2b(cam['target']), 'CONE', 1.0, qr_index=i)
                con = co.constraints.new('TRACK_TO')
                con.target, con.track_axis, con.up_axis = tg, 'TRACK_NEGATIVE_Z', 'UP_Y'

    c = collection(COLL['spr'], True)
    for i, s in enumerate(T.read_sprites(t)):
        empty('spr_%02d_%s' % (i, 'add' if s['additive'] else 'blend'), c, g2b(s['pos']),
              'CIRCLE', 3.0, qr_index=i)

    # fog / light settings as custom properties on one empty
    st = bpy.data.objects.get('QR_track_settings') or empty('QR_track_settings', sc.collection, (0, 0, 0))
    for k, v in T.read_prop(t).items():
        st['prop_' + k] = v


def build_ai_links(t, nodes=None):
    """Poly-line mesh showing forward links between AI nodes (display only)."""
    nodes = nodes or T.read_ai(t)
    objs = {o['qr_index']: o for o in collection(COLL['ai']).objects if 'qr_index' in o and o.type == 'EMPTY'}
    me = bpy.data.meshes.get('qr_ai_links') or bpy.data.meshes.new('qr_ai_links')
    ids = sorted(nodes)
    idx = {i: k for k, i in enumerate(ids)}
    verts = [tuple(objs[i].location) if i in objs else tuple(g2b(nodes[i]['pos'])) for i in ids]
    edges = [(idx[i], idx[f]) for i in ids for f in nodes[i]['forward'] if f in idx]
    me.clear_geometry(); me.from_pydata(verts, edges, [])
    o = bpy.data.objects.get('QR_ai_links')
    if o is None:
        o = bpy.data.objects.new('QR_ai_links', me)
        collection(COLL['ai']).objects.link(o)
    o.hide_select = True


# ------------------------------------------------------------------- write
def _backup(path):
    if os.path.exists(path) and not os.path.exists(path + '.orig'):
        shutil.copy2(path, path + '.orig')


def write_markers(t, out_dir=None):
    """Write marker objects back. out_dir=None writes the game files in place
    (first write keeps a .orig copy next to each file)."""
    def dest(path):
        if out_dir:
            os.makedirs(out_dir, exist_ok=True)
            return os.path.join(out_dir, os.path.basename(path))
        _backup(path)
        return path
    objs = lambda key: {o['qr_index']: o for o in collection(COLL[key]).objects
                        if 'qr_index' in o and not o.name.endswith('_target')}
    report = []

    locs = T.read_locators(t)
    for i, o in objs('start').items():
        mw = o.matrix_world
        new_pos = b2g(mw.translation)
        R = blender_to_game_rot3(mw)
        old_R = [[c / s for c in row[:3]] for row, s in zip(locs[i].matrix[:3], locs[i].scale)]
        rot_changed = max(abs(R[r][c] - old_R[r][c]) for r in range(3) for c in range(3)) > 1e-4
        if rot_changed:
            locs[i].rot = blender_to_game_rot(mw, near=locs[i].rot)
        if rot_changed or max(abs(a - b) for a, b in zip(new_pos, locs[i].pos)) > 1e-3:
            locs[i].pos = new_pos
    T.write_locators(t, locs, dest(os.path.join(T.track_dir(t), 'locator.dat')))
    report.append('starts')

    if os.path.exists(T.config_path('ai', t)):
        nodes = T.read_ai(t)
        for i, o in objs('ai').items():
            nodes[i]['pos'] = b2g(o.location)
        moved = T.write_ai(t, nodes, dest(T.config_path('ai', t)))
        report.append('AI (%d nodes moved)' % len(moved))

    if os.path.exists(T.config_path('pup', t)):
        pups = T.read_pups(t)
        for i, o in objs('pup').items():
            pups[i]['pos'] = b2g(o.location)
            pups[i]['type'] = o.get('qr_type', pups[i]['type'])
        T.write_pups(t, pups, dest(T.config_path('pup', t)))
        report.append('pickups')

    if os.path.exists(T.config_path('camera', t)):
        cams = T.read_cameras(t)
        targets = {o['qr_index']: o for o in collection(COLL['cam']).objects if o.name.endswith('_target')}
        for i, o in objs('cam').items():
            cams[i]['pos'] = b2g(o.location)
            if i in targets:
                cams[i]['target'] = b2g(targets[i].location)
        T.write_cameras(t, cams, dest(T.config_path('camera', t)))
        report.append('cameras')

    sprs = T.read_sprites(t)
    for i, o in objs('spr').items():
        sprs[i]['pos'] = b2g(o.location)
    T.write_sprites(t, sprs, dest(os.path.join(T.track_dir(t), 'sprite.dat')))
    report.append('sprites')

    st = bpy.data.objects.get('QR_track_settings')
    if st:
        props = {k[5:]: (list(v) if hasattr(v, '__len__') and not isinstance(v, str) else v)
                 for k, v in st.items() if k.startswith('prop_')}
        T.write_prop(t, props, dest(os.path.join(T.track_dir(t), 'prop.dat')))
        report.append('fog/light')
    if 'QR_ai_links' in bpy.data.objects:
        build_ai_links(t)
    return report


# ---------------------------------------------------------------------- UI
def _track_items(self, ctx):
    return [(t, t, '') for t in tracks()]


class QR_OT_import(bpy.types.Operator):
    bl_idname, bl_label = 'qr.import_track', 'Import track + markers'
    def execute(self, ctx):
        t = ctx.scene.qr_track_pick
        import_track_model(t, ctx.scene.qr_with_sky)
        import_markers(t)
        self.report({'INFO'}, 'Loaded %s' % t)
        return {'FINISHED'}


class QR_OT_markers(bpy.types.Operator):
    bl_idname, bl_label = 'qr.import_markers', 'Reload markers only'
    def execute(self, ctx):
        import_markers(ctx.scene.qr_track_pick)
        return {'FINISHED'}


class QR_OT_links(bpy.types.Operator):
    bl_idname, bl_label = 'qr.ai_links', 'Refresh AI link lines'
    def execute(self, ctx):
        build_ai_links(ctx.scene.get('qr_track', ctx.scene.qr_track_pick))
        return {'FINISHED'}


class QR_OT_write(bpy.types.Operator):
    bl_idname, bl_label = 'qr.write_markers', 'Write markers to game files'
    bl_description = 'Writes starts, AI nodes, pickups, cameras, sprites and fog/light back (keeps .orig backups)'
    def invoke(self, ctx, ev):
        return ctx.window_manager.invoke_confirm(self, ev)
    def execute(self, ctx):
        t = ctx.scene.get('qr_track')
        if not t:
            self.report({'ERROR'}, 'Import a track first'); return {'CANCELLED'}
        self.report({'INFO'}, 'Wrote %s: %s' % (t, ', '.join(write_markers(t))))
        return {'FINISHED'}


class QR_OT_write_models(bpy.types.Operator):
    bl_idname, bl_label = 'qr.write_models', 'Write track model to game files'
    bl_description = "Writes the track's and the sky's mesh edits into their model.cmm (keeps .orig backups)"
    def invoke(self, ctx, ev):
        return ctx.window_manager.invoke_confirm(self, ev)
    def execute(self, ctx):
        written = write_models()
        if not written:
            self.report({'ERROR'}, 'Import a track first'); return {'CANCELLED'}
        self.report({'INFO'}, 'Wrote %s' % ', '.join(written))
        return {'FINISHED'}


class QR_PT_panel(bpy.types.Panel):
    bl_label, bl_space_type, bl_region_type, bl_category = 'Quantum Redshift', 'VIEW_3D', 'UI', 'QR'
    def draw(self, ctx):
        l = self.layout
        l.prop(ctx.scene, 'qr_track_pick', text='Track')
        l.prop(ctx.scene, 'qr_with_sky')
        l.operator('qr.import_track'); l.operator('qr.import_markers')
        l.separator(); l.operator('qr.ai_links'); l.operator('qr.write_markers', icon='EXPORT')
        l.operator('qr.write_models', icon='EXPORT')
        if ctx.scene.get('qr_track'):
            l.label(text='Loaded: %s' % ctx.scene['qr_track'])


CLASSES = (QR_OT_import, QR_OT_markers, QR_OT_links, QR_OT_write, QR_OT_write_models, QR_PT_panel)


def register():
    for c in CLASSES:
        bpy.utils.register_class(c)
    bpy.types.Scene.qr_track_pick = bpy.props.EnumProperty(name='Track', items=_track_items)
    bpy.types.Scene.qr_with_sky = bpy.props.BoolProperty(name='Include sky', default=True)


def unregister():
    for c in reversed(CLASSES):
        bpy.utils.unregister_class(c)
    del bpy.types.Scene.qr_track_pick, bpy.types.Scene.qr_with_sky


if __name__ == '__main__':
    register()
