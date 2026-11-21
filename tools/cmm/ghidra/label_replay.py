# Ghidra Jython: the replays' weapon entries, the pause menu's ghost replays and the replay cameras' director (CReplay,
# replay.cpp). See docs/replays/README.md "Weapon entry", docs/replays/users.md "Users" and "Replay cameras".
from ghidra.program.model.symbol import SourceType
fm = currentProgram.getFunctionManager()
names = {
    0x000641f0: ('craft_weapon_replay_entry', 'WeaponEvents callback: kind 1 a projectile state, kind 2 a signal through the table 0x646a8: 1 shield value (0x6ac90), 2 launch (0x64e20), 3 done (0x65e60), 4 impact (0x65f80, +0x28 the kind), 6 power level +0x9c, 7 fire (0x6a390), 8 / 9 pickup taken / back, 0xb retaliate (0x6a6c0), 0xc 0x64b30 then impact.'),
    0x0006ac90: ('craft_weapon_set_shield', 'Weapon +0 = the value; at 0 or below (and the craft not dead) knocked out (0x6a8a0) by the given killer.'),
    0x000424d0: ('timeattack_message', 'GAMELOGIC messages (id 7): +0x20 = 3 replay the loaded ghost (+0xd2bc = 1), 4 the session ghost (+0xd2bc = 2), 0 +0xd340, 5 +0xd410 = 0.'),
    0x00043460: ('timeattack_restart', 'Clock 0; with +0xd2bc: the CReplay "replay" from z:\\eventbuffer\\preloaded (1, craft +0xce74) / fastest (2, +0xce70), camera0 "replay on", hud0 "set none".'),
    0x00043770: ('timeattack_ghost_replay_update', 'All three crafts hidden (+0x1129) but the replayed ghost; the player craft replayed from the buffer and its state blocks copied into the ghost craft; ends (vtable +0x14) at the buffer end after 1 s or an entry 0.1 s old.'),
    0x00043920: ('timeattack_ghost_replay_end', 'The buffer closed, 0x3d890, clock 0, the crafts on player_start0..2, camera0 "replay off": the run starts again.'),
    0x0008b560: ('pausemenu_replay_ghost', 'Closes the pause (0x782d0) and sets 0x448e50 = 3 (the pause state, as Continue / Restart); the item handlers set +0x626 / +0x627, sent as GAMELOGIC 3 / 4.'),
    0x000a3030: ('camera_update_modes', 'camera0: messages attach tripod / start flymode / stop flymode / replay on (mode 0 on the named output tripod) / replay off / coll on; mode 0 copies the tripod (eye +8, look +0x20, fov +0x38).'),
    0x000d5b60: ('creplay_init', 'CReplay (replay.cpp): the "replay%doutput" tripod, a camera object per config\\camera_<t>.txt camera by type (0 static 0x198830, 1 moving 0x1988a8, 2 ship 0x198858, 3 heli 0x198880).'),
    0x000d5f10: ('creplay_parse_cameras', 'config\\camera_<t>.txt: num cameras, then each camera by <type=static|heli|moving|ship>.'),
    0x000d6340: ('creplay_update', 'Each frame: the first static camera to start; every camera scored (vtable +4) with the lap %; the best takes over once the current one is not held (+0x58 <= 0): leave +0x18, enter +0x10; "Changed to cam %d with score %.1f"; the current one updates the output (+0x14).'),
    0x000d7b00: ('replaycam_static_score', 'ONLYFORWARD / ONLYINFRAME (dot < -0.8), the panrange window (wrapping), held 100, else 100 x (range - distance) / range, 0 out of range.'),
    0x000d76e0: ('replaycam_static_update', 'Hold -= dt; DYNAMICTRACKING look eases (10 / s) to the craft + speed-class lead x velocity; KERB shake (racers within 10 m of the eye, level +0xcc up to 0.6, phase +0xc8 at 3 / s, look = target + (sin 15p, cos 13p, sin 10p) x level x 0.02); DYNAMICFOV fov -> max(5, 90 - 0.7 d) at 2 / s.'),
    0x000d75f0: ('replaycam_static_enter', 'Hold = holdtime (at least 1 s), fov = the camera fov.'),
    0x000d6e50: ('replaycam_moving_score', '100 inside the panrange window, else -100.'),
    0x000d68a0: ('replaycam_moving_update', 'The slide t (11 tries 0.003 apart) facing the craft best, eased 4 / s; look follows the craft with sine wobbles; DYNAMICFOV 90 - 0.6 d, TIGHTFOV 30 - d, PRESETFOV fov1 .. fov2.'),
    0x000d7020: ('replaycam_ship_score', 'Score 5 once its rest (+0x80, 15 after leaving, -0.01 a frame) is over, else decaying 0.1 a frame.'),
    0x000d7110: ('replaycam_ship_update', 'Eye and look at offsets in the craft space; fov the camera fov; hold 1.5 s (0xd70f0).'),
}


def add_comment(f, cmt):
    old = f.getComment() or ''
    if cmt not in old:
        f.setComment((old + '\n' if old else '') + cmt)


tx = currentProgram.startTransaction('QR replay labels')
done = 0
try:
    for a, (n, cmt) in sorted(names.items()):
        f = fm.getFunctionAt(toAddr(a))
        if f is None:
            disassemble(toAddr(a))
            f = createFunction(toAddr(a), n)
            if f is None:
                print('could not create a function at %x' % a)
                continue
        f.setName(n, SourceType.USER_DEFINED)
        add_comment(f, cmt)
        done += 1
finally:
    currentProgram.endTransaction(tx, True)
print('QR replay labels applied: %d' % done)
