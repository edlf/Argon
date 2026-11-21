# Ghidra Jython: ship effects (shadow, trail, wake spray, droplets, sprites, HUD power-up panel) and the player
# controls (pad, brake, airbrakes, side friction). See docs\effects\README.md and docs\physics\README.md; ported in
# src\app\app_effects.cpp, app_hud.cpp, app_drive.cpp and craftsim.cpp.
from ghidra.program.model.symbol import SourceType
fm = currentProgram.getFunctionManager()
names = {
    # --- ship shadow
    0x0009b820: ('shadow_render_ship', 'Per ship: orthographic 13.5 x 13.5 silhouette from ship - 100 x sun dir into a render target\n'
                 '(camera via 0xbe230), then up to 64 nearby light-facing collision triangles as receivers, vertex grey\n'
                 '= clamp(35 - |dy|, 0, 30). See docs\\effects\\README.md.'),
    0x000be230: ('shadow_set_ortho_camera', 'Ortho projection (size param 7, z 0.001..3000) + view from param 5/6 into 0x50a0d0, viewport, SetTransform.'),
    0x0009c380: ('shadow_draw_receivers', 'Draws the shadow receiver triangles: blend ONE / ONE, op REVERSE_SUBTRACT (0x800b).'),
    # --- trail
    0x0009da30: ('trail_add_point', 'Adds a point to one of the 12 x 96-point trail rings: +-0.5 x up and +-right cross sections,\n'
                 'colour written 3 updates late.'),
    0x0009db10: ('trail_fade', 'Fades every trail colour by 1 per update.'),
    0x0009dca0: ('trail_draw', 'Draws the trails: newtrail.dds on stages 0 and 1, additive, v scroll -0.5 / -2 per frame.\n'
                 'Sets no stage combine: inherits the last translucent mesh state (flush 0xbbf60 -> 0xba660).'),
    0x00050960: ('craft_wake_emitter', 'Per craft: two trail points at -+2 x right - 4 x forward (colour from speed x 1.1, x 2 on water);\n'
                 'on water (!0x52afac, craft+0x90 > 1, !craft+0x94) walks 2 m steps from the last position (+0xd0):\n'
                 'spray puff each side (0x9d620) and 50 % a droplet sheet each side (0x9d2c0).'),
    0x0004ff70: ('fx_trail_bone_cb', 'TRAIL mesh-effect callback (no shipped model has the bone).'),
    0x000501c0: ('fx_watertrail_bone_cb', 'WATERTRAIL mesh-effect callback (no shipped model has the bone).'),
    # --- wake spray / droplets / sprites
    0x0009d620: ('spray_puff_add', 'Adds a spray puff (pool 0x240540, 4096 x 0x50, bitset 0x1f0330): pos, life, colours, W/H,\n'
                 'mode 1 height = sin(age / life x pi) x max height. Sprite cell (0, 384) of quadsprite.dds.'),
    0x0009c540: ('spray_puff_update', 'Ages / moves the spray puffs, fade over the last 15 % of life.'),
    0x0009d2c0: ('spray_drop_add', 'Adds a droplet sheet (pool 0x1f0538, bitset 0x240538). EAX = atlas cell, EBX = side vector;\n'
                 'args pos, vel, life, start / end colour, hold, streak length, gravity, list, fade-in, ?.'),
    0x0009ca50: ('particles_update', 'Runs spray_puff_update, then updates the droplet sheets inline: pos += dt x vel, vel.y += dt x g,\n'
                 'corners pos -+ side and pos + normalize(vel) x streak -+ side, colour held then blended to the end colour.'),
    0x000bd8f0: ('sprite_render_lists', 'Sprite pass: list A alpha test + blend mode 1, list B blend mode 2. Stage combine not set (inherited).'),
    # --- HUD
    0x0004eb20: ('hud_powerup_panel_draw', 'Power-up panel 128 x 64 at ((L + R) / 2 - 64, T): weaponbase (mode 1), weapon<colour><1..3>\n'
                 'layers (mode 2, additive), weaponspecial (mode 1).'),
    # --- player controls
    0x00070ac0: ('player_pad_input', 'this = craft, pad state from 0xb4800. RT x 0.390625 -> throttle +0x8f4 (0..100); d-pad / stick X\n'
                 'past +-12288 -> steer +0x908 (left) / +0x90c (right) = (|x| - 12288) x 0.00488; stick Y -> dive / climb\n'
                 '+0x8fc / +0x900; LT -> airbrake +0x910 / +0x914 only on the side being steered. Never sets the brake +0x918.'),
    0x00071fe0: ('player_brake_ramp', 'Brake value +0x980 += dt x brake_gain x 60 while +0x918 is held (max 100), else 0. At 100 below\n'
                 '5 m/s runs timer +0x984 (never read); clears reverse flag +0x988 / strength +0x98c.'),
    0x00071bc0: ('player_airbrake_ramp', '+0x978 / +0x97c follow +0x910 / +0x914: up dt x airbrake_force_gain x 60, down dt x falloff x 60.'),
    0x00061aa0: ('craft_record_inputs', 'Packs the control slots into the bitmask +0x838 (replay / network).'),
    0x00061c60: ('craft_replay_inputs', 'Replays the control slots from the bitmask +0x838.'),
    0x0005d590: ('craft_clear_inputs', 'Clears the control slots +0x8f4..+0x918.'),
}
comments = {   # functions that already have names: add comments only
    0x00060450: 'Player brake: only with +0x92c. F -= normalize(v) x +0x980 x brake_amount (each component <= v x 25);\n'
                'speed > 0.1: T += 2 x right x +0x980 x engine_noseup. Reverse push if byte +0x988 == 1:\n'
                'F -= (fwd speed + 20) x forward x mass x +0x98c, but nothing sets +0x988.',
    0x00060230: 'Airbrakes: s = clamp(fwd speed, 0, 50); F +-= right x (front + rear) x a x s x 0.01,\n'
                'T +-= up x (front - rear) x a x s x 0.01 (+ for +0x978, - for +0x97c). Sideways only.',
    0x0005ffd0: 'Player side friction: k = friction_<surface>_side_factor (+0x344 + surface x 4); airbrake level +0xa18\n'
                '(max of +0x978 / +0x97c, decays 150 / s); k\' = (101 - level) / 101 x k; below 100 m/s with k < 2\n'
                'blended g x k\' + 2 (1 - g), g = (100 - speed) / 100. F -= right x (v . right) x mass x k\'.',
    0x0009d890: 'Trail setup (newtrail.dds). 12 trails x 96 points.',
}
labels = {
    0x00240540: 'g_spray_puffs',
    0x001f0330: 'g_spray_puff_used',
    0x001f0538: 'g_spray_drops',
    0x00240538: 'g_spray_drop_used',
}


def add_comment(f, cmt):
    old = f.getComment() or ''
    if cmt not in old:
        f.setComment((old + '\n' if old else '') + cmt)


tx = currentProgram.startTransaction('QR effects + player control labels')
done = 0
try:
    for a, (n, cmt) in sorted(names.items()):
        f = fm.getFunctionAt(toAddr(a))
        if f is None:
            f = createFunction(toAddr(a), n)
            if f is None:
                print('could not create a function at %x' % a)
                continue
            print('created function %s at %x' % (n, a))
        f.setName(n, SourceType.USER_DEFINED)
        add_comment(f, cmt)
        done += 1
    for a, cmt in sorted(comments.items()):
        f = fm.getFunctionAt(toAddr(a))
        if f is None:
            print('no function at %x' % a)
            continue
        add_comment(f, cmt)
        done += 1
    st = currentProgram.getSymbolTable()
    for a, n in sorted(labels.items()):
        st.createLabel(toAddr(a), n, SourceType.USER_DEFINED)
        done += 1
    print('QR effects labels applied: %d' % done)
    # docs moved from _tools\cmm to docs\ (2026-11-11): fix the paths in older comments
    import re
    pat = re.compile(r'_tools\\+cmm\\+(AUDIO|FORMAT|PHYSICS|TRACKS|WATER)\.md')
    fixed = 0
    for f in fm.getFunctions(True):
        c = f.getComment()
        if c and pat.search(c):
            f.setComment(pat.sub(lambda m: 'docs\\' + m.group(1) + '.md', c))
            fixed += 1
    print('comment paths updated: %d' % fixed)
finally:
    currentProgram.endTransaction(tx, True)
