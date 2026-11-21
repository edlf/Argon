# Ghidra Jython: craft physics + AI pilot names (see docs\physics\README.md, ported in src\craft\craftsim.cpp).
from ghidra.program.model.symbol import SourceType
fm = currentProgram.getFunctionManager()
names = {
    0x00072390: ('character_stats_parse', 'Parses config\\character_stats.txt into 16 records of 0x3a4 at 0x593358 (+0xc4 = Shield_Strength).'),
    0x00071490: ('craft_update', 'Player craft Update(dt) (vtable 0x19f3e8 slot 11): +0x1140 = stats, object update 0x5aeb0, revs/gears, turn/airbrake smoothing.'),
    0x0006dd90: ('drone_update', 'AI craft Update(dt) (vtable 0x1a1c10 slot 11): +0x1140 = stats record 0, AI tactics/pilot, revs/gears 0x6ed90, turn smoothing 0x6f0b0,\n'
                 'speed cap 0x6f900; every 2 s: moved < 5 m -> respawn flag +5.'),
    0x0005aeb0: ('object_update', 'CGameObject update: 0x70430 state copy, debug message queue, controller->vtable[2], respawn 0x5cb30, race info vtable 0x74.'),
    0x00070430: ('object_end_step', 'Copies the latest state into the cached axes (+0x250 fwd, +0x25c up, +0x268 right, +0x27c pos) and 4x4 +0x1d0; clamps.'),
    0x0005d670: ('craft_physics_step', 'vtable[0](state, h): clears F/T, then thrust, hover, gravity, brakes, dive/climb, steer, side friction, airbrakes, aero,\n'
                 'angular damping, nosedrop, turbo, water (each followed by 0x5d5e0 clamp +-5e6).'),
    0x0005d5e0: ('clamp_vec_5e6', 'Clamps a force/torque vector to +-5e6.'),
    0x0005e370: ('craft_thrust', 'Engine thrust: power/gear ratio (set 1), rev-rise limiter (engine_gain), falloff when coasting, engine_noseup torque.'),
    0x0005e6d0: ('craft_hover', '4 hover rays (box corners 0-3) vs collision 0xd4770: spring normalgravity*mass*depth, rebound damping, tilt + cross(r,n)*d*5 torque,\n'
                 'surface type from 3 agreeing contacts, water fallback 0xce6b0.'),
    0x0005e600: ('craft_gravity', 'g = (k*normalgravity + (4-k)*flightgravity)/4 for k hover contacts.'),
    0x0005fe50: ('craft_dive_climb', 'Airborne pitch torque from +0x8fc dive / +0x900 climb.'),
    0x0005f9d0: ('craft_steer', 'Steering torque: ground turn_max*s^2/100 about up, air flyingyaw/flyingroll; stiffening by speed.'),
    0x0006fb20: ('drone_steer', 'AI steering: 0x5f9d0 + yaw torque from steering error +0x924 (40/100/200, 200/500/1000 when correcting).'),
    0x0005ffd0: ('craft_side_friction', 'Lateral friction -v.right*mass*side_factor[surface].'),
    0x0006f720: ('drone_side_friction', 'AI lateral friction with friction_AI_*_side_factor.'),
    0x00060230: ('craft_airbrakes', 'Airbrake force/torque from smoothed +0x978/+0x97c.'),
    0x00060450: ('craft_brakes', 'Brake force from +0x980 (AI zeroes it) and nose-up.'),
    0x00060760: ('craft_aero', 'Downforce, airborne dartiness/flightcontrol/autolevelling, quadratic drag (cod per surface or air).'),
    0x0005f800: ('craft_angular_damping', 'T -= sum((a x w) x a) * dampening_ground/air.'),
    0x00060e40: ('craft_nosedrop', 'Airborne nose drop after nosedrop_delay.'),
    0x00061340: ('craft_water', 'Buoyancy at the 8 box corners, water_noseup, surface = water when > 3 under.'),
    0x0005e270: ('craft_turbo', 'Turbo force turbo_force[speed] along forward while +0x8e4 > 0.'),
    0x0006ed90: ('drone_revs_gears', 'Forward speed +0x990, |v| +0x9a4, revs +0x940 = v/ratio, gear shifts at engine_maxrevs / 0.85, engine sound value +0x9ac.'),
    0x0006f0b0: ('drone_turn_smooth', 'Smooths steering +0x908/+0x90c into +0x970/+0x974 with turn_gain/turn_falloff (record 0).'),
    0x0006f900: ('drone_speed_cap', 'Caps forward speed at absolute_max_speed (km/h) by scaling P and v.'),
    0x0006f280: ('drone_hull_push', 'Boundary-plane push from AI hulls; the d<5 && d>5 test makes it dead code.'),
    0x0006fdc0: ('object_set_box_mass', 'Collision box from 8 corners, mass = density*48 (*FLOAT 48), diagonal box inertia + inverse at +0x148.'),
    0x00070340: ('craft_make_box', 'Builds the 4x2x6 box corners (0-3 bottom, 2/3 rear) and calls 0x6fdc0 with mass_density.'),
    0x000cf630: ('physics_world_step', 'Per object: substeps h = 1/max(|v.fwd|/3, |v.right|/2, |v.up|*1.2) clamped 0.004..0.02 -> 0xcffb0; then 0xcf920.'),
    0x000cffb0: ('physics_substep', 'vtable[0] forces, 0xcedd0 integrate, 0xd1620 shape, matrix clamp, 0xd3c70 world contacts, 0xd05c0 response, swap states.'),
    0x000cedd0: ('rigid_integrate', 'Semi-implicit Euler: R += h*[Iw L]x R (z term doubled), x += h v, P += h F, v = P/m, L += h T, w = Ib^-1 L; orthonormalise.'),
    0x000ceb30: ('mat3_star_zx2', 'Cross-product matrix of w with the z terms doubled.'),
    0x000cecd0: ('mat3_mul_ba', 'out = B * A (row major).'),
    0x000cead0: ('mat3_transpose', ''),
    0x000cec60: ('mat3_mul_vec', 'out = M * v.'),
    0x000ceba0: ('mat3_normalise_columns', ''),
    0x000eb9f0: ('mat3_reorthogonalise_cycle', 'Rebuilds one row pair by cross products, cycling via 0x5a216c.'),
    0x000d3c70: ('physics_world_contacts', 'Box (vertices/edges) vs collision mesh contacts for one object.'),
    0x000d5140: ('contact_response', 'P -= (1.2,1.03,1.2)*m(v.n) n, friction 0.1*((-P^.n)*0.8+0.2)*min(|P|,70), x nudge 0.005, L -= (r x n)*m(v.n)*0.0043.'),
    0x000d05c0: ('contact_bind_state', 'Points the contact handler at the next state block before responses.'),
    0x00042350: ('vec3_cross', 'Standard a x b.'),
    0x00058360: ('vec3_normalise_len', 'Normalises in place, returns the length.'),
    0x0005cb30: ('craft_respawn', 'Respawn when +5: restart pos (0x597330 + slot*0x88), ray +2..-20, placed 7.5 above the hit.'),
    0x0005ce70: ('craft_race_info', 'Wrong-way timer, race positions, HUD strings (surface/pilot names).'),
    0x000e0810: ('ai_pilot_control', 'AI controls into craft +0x8f4: aim = fwd + 0.6*v*f^2/100, error vs lerp(P1,P2,f), yaw damping 0.018, dead zone 0.02,\n'
                 'steer gains 500/200, airbrake at |e| > 0.4 or hairpin, throttle when |e| < 0.45.'),
    0x000e0f40: ('CAIControl_flags', 'Corner flags of the tactics points: 0x40000 slow, 0x100000 straight, 0x80000 hairpin, turbo good/never per speed.'),
    0x000e1080: ('ai_nose_up_recovery', 'Airborne and forward.y < -0.6: climb = -50*forward.y.'),
    0x000e10f0: ('ai_turbo_decision', ''),
    0x000e4be0: ('ai_tactics', 'Targets P1/P2/P3 at radius 1, 1+e, 21+e along the look-ahead (e = 0/20/40 by speed, hairpin 0, slow /2) + overtaking offsets.'),
    0x000e51a0: ('ai_point_at_radius', 'Point on the look-ahead polyline at a given distance from the craft.'),
    0x000e4730: ('ai_lookahead_update', 'Refreshes the 8 look-ahead node lists when the current node changes.'),
    0x000e3460: ('ai_branch_choice', 'At a branch picks the forward link by route-table value for the surface priority.'),
    0x000e2520: ('ai_parse_nodes', 'ai_<t>.txt nodes: id, pos (+8), down (+0x14), links, flags (2 first, 4 restart, 0x100<<s turbo_never, 0x2000<<s turbo_good,\n'
                 '0x40000 corner_slow, 0x80000 hairpin, 0x100000 straight).'),
    0x000e0770: ('ai_bind_dataports', 'Binds "ai tacticsDP %d", "craft%d posorient" (+0x274), "craft%d velinertia" (+0x16f4).'),
    0x00060f80: ('craft_bind_dataports', 'Registers craft dataports incl. "craft%d sound" (gear +0x93c, revs +0x940, airbrakes, speed +0x990 ...).'),
}
tx = currentProgram.startTransaction('QR physics labels')
done = 0
try:
    for a, (n, cmt) in names.items():
        f = fm.getFunctionAt(toAddr(a))
        if f is None:
            disassemble(toAddr(a)); f = createFunction(toAddr(a), None)
        if f is None:
            print('no function at %x' % a); continue
        f.setName(n, SourceType.USER_DEFINED)
        if cmt:
            old = f.getComment() or ''
            if cmt not in old:
                f.setComment((old + '\n' if old else '') + cmt)
        done += 1
    print('QR physics labels applied: %d' % done)
finally:
    currentProgram.endTransaction(tx, True)
