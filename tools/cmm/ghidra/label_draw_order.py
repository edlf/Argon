# Ghidra Jython: the "Diffuse Specular Bump Map" effect. See docs/models/effects.md "Effects"; ported in
# shaders/mesh_shading.glsl (kind 5).
from ghidra.program.model.symbol import SourceType
fm = currentProgram.getFunctionManager()
names = {
    0x0007d5c0: ('fx_dsbm_prepare', 'Diffuse Specular Bump Map: 0x50-byte vertices (pos, normal, uv0, basis, colour, uv1).'),
    0x0007d6e0: ('fx_dsbm_draw', 'Constants: c4 1, c5 0, c6 viewport, c7 -light dir, c10 eye; ps c0 ambient, c1 light, c2 2 x light; fog off.'),
    0x000a26c0: ('fx_dsbm_bind', 'Pixel shader 0x2907f8 and the two clamped lookup textures (stages 2 / 3).'),
    0x000a2dd0: ('fx_basis_build_50', 'Tangent basis for 0x50-byte vertices: T, B summed (0xa2bb0), normalised, N = T x B to the normal.'),
    0x0007ba90: ('fx_specbump_prepare', 'Specular Bump: 0x44-byte vertices.'),
    0x0007bb80: ('fx_specbump_draw', 'Specular Bump constants (c4 eye, c6 viewport, c15 stage-0 scroll).'),
    0x0002f250: ('vec3_normalize_fast', 'Normalise with the rsqrt table.'),
    0x000b80f0: ('cmm_draw_model_track', 'File order (no render list) or render list entries; translucent (blend bit 1) keyed by the eye x / z distance to the farthest box edge.'),
    0x000b9270: ('cmm_build_mesh_boxes', 'Per mesh vertex bounds (min xyz, max xyz, centre, radius), 0x28 each at model +0x214; mesh +0x5c points to its entry.'),
    0x000afcf0: ('scene_draw_track_race', 'Sector-visible meshes; see-through ones keyed by the eye distance to mesh +0x4c (flag 0x80) or the far box edge.'),
    0x0003d5a0: ('race_craft_lap', 'A craft completes a lap: turbo +1.5 + 0.5 x upgrade (player, Turbo Up) or a speed-class amount (AI); lap records.'),
    0x000720e0: ('craft_speed_cap_player', 'Cap at absolute_max_speed, or turbo_max_speed while +0x9f3 (cleared once back under).'),
    0x0006f9f0: ('drone_turbo', 'AI turbo: cooldown +0x9c4, then as craft_turbo.'),
    0x000b4800: ('pad_read', 'XInput pad; analog buttons to bits +0x86: A 0x10 B 0x20 X 0x40 Y 0x80 White 4 Black 8 LT 1 RT 2.'),
    0x00050670: ('craft_jets_setup', 'JET / JFLARE bone lists; jet size 0.1 idle, 0.5 full throttle, 1.0 turbo, rate 10.'),
    0x00051480: ('craft_jets_tint', 'Instance tint of the JET bones.'),
    0x00051520: ('craft_flares_tint', 'Instance tint of the JFLARE bones.'),
    0x000515c0: ('craft_jets_scale', 'JET bones scaled (1, 1, size).'),
    0x000e10f0: ('ai_turbo_decision', 'Drone turbo button: progress past HeadStartDistance, turbo_good node, triggers +0x47d / +0x47e (never set), 0.5 s timer, steer error within 0.05.'),
    0x000e0f40: ('CAIControl_flags', 'Node flags: corner slow / straight / hairpin, turbo good (0x2000 << speed) / never (0x100 << speed).'),
    0x0006f8e0: ('drone_turbo_button', '+0x9f0 = +0x91f.'),
    0x000c4a70: ('sound_find_playing', 'Playing instance of a craft sound id, or 0.'),
    0x000bbb70: ('renderlist_add_mesh', 'Bucket by kind: 0 opaque (0xbb700), 4 translucent (0xbb820), ...'),
    0x000bb820: ('renderlist_insert_sorted', 'Sorted far to near; equal keys before the existing ones (reverse add order).'),
    0x000bbc40: ('renderlist_draw_opaque', 'Buckets 0..2, pass 0.'),
    0x000bbf60: ('renderlist_draw_translucent', 'Bucket 4, pass 1.'),
    0x000ba370: ('surface_setup_1', 'Lighting off, track fog (or the block +0x44), texture wrap.'),
    0x000ba1d0: ('surface_setup_2', 'Lighting on unless flags 0x20, fog at +0x14.'),
    0x000a8410: ('rs_set_ambient', 'EDI -> D3DCOLOR: D3DRS_AMBIENT (cache 0x1188ec, NV2A 0xa10). Surface setup flag 4 without 0x100: the ship flare turbo glow.'),
    0x000a82d0: ('rs_set_light0_colour', 'EAX -> D3DCOLOR: light 0 diffuse (ESI device +0x60b8), SetLight 0. Surface setup flag 4 with 0x100.'),
    0x000529f0: ('craft_parts_anim_a', 'Ship-class bone animation (nozzles / airbrakes); turbo +0x9f1 tilts parts. Called from 0x52860.'),
    0x000546a0: ('craft_parts_anim_b', 'Ship-class bone animation; turbo +0x9f1 tilts parts. Called from 0x54560.'),
    0x00057220: ('craft_parts_anim_c', 'Ship-class bone animation; turbo +0x9f1 tilts parts. Called from 0x570e0.'),
    0x00052860: ('craft_class_update_a', 'Ship-class vtable update: wake emitter 0x50960 + part animation 0x529f0.'),
    0x00054560: ('craft_class_update_b', 'Ship-class vtable update: wake emitter 0x50960 + part animation 0x546a0.'),
    0x000570e0: ('craft_class_update_c', 'Ship-class vtable update: wake emitter 0x50960 + part animation 0x57220.'),
    0x000a63b0: ('occl_points_create', '256 points uniform in a unit sphere (rand, rejection) -> VB 0x292d40, stride 12: the flare occlusion test points.'),
    0x000a64f0: ('occl_test_draw', 'Visibility test: N points of 0x292d40 through the world matrix with axes x radius, depth test, no depth write; EndVisibilityTest(device +0x6040).'),
    0x000a6750: ('occl_test_result', 'GetVisibilityTestResult: visible pixel count of a test.'),
    0x000bb210: ('surface_occl_points', 'Occlusion point count: setup 1 surface +0x50, setup 2 +0x20.'),
    0x000bb240: ('surface_occl_radius', 'Occlusion radius: setup 1 surface +0x4c, setup 2 +0x1c (else 0).'),
    0x000b6e00: ('mesh_bone_record', 'EAX mesh -> its bone record (bone index at mesh +0, records 0x1a0 at model +0x220[instance]), 0 if out of range.'),
    0x000b6e30: ('bone_record_at', 'EAX bone index -> bone record of model instance param_1 (no lower bound check). Surface flag 0x80000: parent +0x138 copied.'),
    0x000b9540: ('neon_model_load_skeleton', 'Per bone record (0x1a0): +0x138 intensity = 1.0, +0x104 tint = 0xffffffff, +0x108 visibility query = 999999, +0x13c / +0x130 = 0, flags +0x102; links each bone to its meshes.'),
    0x000b96e0: ('model_instance_skeleton', 'New instance slot (model +0x220 + 4 n, n 1..0x5f): a copy of the template bone records.'),
    0x000a3df0: ('cmm_skin_setup', 'Skinned mesh (flag 0x40000): char*bone.xvu vertex shader, constants c100.. (light colours 0x3910f4 / 0x391100, direction 0x39110c), bone matrices from c136; light colour black -> all 4 stages colour op code 0 (MODULATE).'),
    0x000a7ea0: ('fe_set_light1', 'Light 1 colour (ARGB) and direction (EAX vector), SetLight(1).'),
    0x00082a70: ('upgrade_prices_init', 'EAX = 0x1bab30: three rows of 7 prices at +0x19e8 (0x1bc518): 50000 + 75000 x level.'),
    0x0009dfb0: ('upgrademenu_build', 'UPGRADEMENU elements: title 0x3fe, balance (320,103), rows 0x44c.. (62,140+35n), Continue (62,270), Clear Changes (62,305).'),
    0x0009e7d0: ('upgrademenu_update', 'UPGRADEMENU ON (cursor on Continue), cursor 5/6 wrapping 0..4, A: buy / leave / Clear Changes (restore 0x1bc87c..).'),
    0x0009fd90: ('upgrademenu_row_text', 'EAX = upgrade: "<name> - Upgrade Cost: <price>" or "<name> - Maximum".'),
    0x0009ffc0: ('upgrademenu_leave', 'Back to WINMENU / LOSEMENU.'),
    0x0003f3d0: ('race_setup_caps', 'Per craft pickup caps and shield level: AI by speed class and +0x9f8/+0x9f9; the player from its upgrades (0x3f650, shield 0x52cd24).'),
    0x0003f650: ('weapon_caps_from_level', 'EAX = Weapon Strength level, ESI = craft: caps red/yellow/blue/special 0:1/1/1/0 .. 7:3/3/3/1.'),
    0x00064030: ('shield_size', '20 x level + 100 / ((100 - armour) x 0.01), armour = pilot block +4 (weapon.nwp).'),
    0x00019ed0: ('cheat_full_upgrade', 'Every unlocked character: levels 7 / 7 / 7.'),
}


def add_comment(f, cmt):
    old = f.getComment() or ''
    if cmt not in old:
        f.setComment((old + chr(10) if old else '') + cmt)


tx = currentProgram.startTransaction('QR draw order, bump effect and turbo labels')
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
tx = currentProgram.startTransaction('QR draw order comments')
try:
    setPreComment(toAddr(0xbad5b), 'Light 0 colour (0x50e2f8) black and no surface flag 0x2000: all 4 stages colour op '
                  'code 0 (MODULATE). Locked character silhouettes on FE CHARSELECT (checked live in xemu).')
    setPreComment(toAddr(0xa449c), 'Skinned: shell light colour (0x391100..08) black -> all 4 stages code 0 (MODULATE).')
    for flagTest in (0xb02d1, 0xb7efb):
        setPreComment(toAddr(flagTest), 'Surface flag 0x20000: inserted with sort key 0 (others at least 0.1, 0x1a3508), '
                      "so drawn after every other mesh of its bucket (Draco's sand over its grass). docs/models/surface-block.md.")
finally:
    currentProgram.endTransaction(tx, True)
print('QR draw order, bump effect and turbo labels applied: %d' % done)
