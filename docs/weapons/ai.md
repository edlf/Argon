# Weapons: the AI's firing and the nemesis

## AI firing (drone 0x6e9e0, each frame while racing)
- Wants (red, blue, yellow) by speed class (0x52cd04): 0..1 0x6e950, 2..4 0x6e8d0. Target checks only when the drone
  is less than 5 % of a lap behind the player (+0x1808 = its laps + % - the player's) or it is the nemesis (+0x9f9,
  then the player only: masks 1 / -2 passed to 0x6e660 / 0x6e380, the others -1 / 0); the nemesis also runs 0x6e1e0
  at speeds 0..1:
  - 0x6e660: per colour with a lock-on weapon, each of its lock targets (+0x60, count +0x4c / +0x50 / +0x54; the
    special's with red's count) whose racer +0x74 > 2.7 and +0x1c == 0 -> wanted. The player in the list sets +0x1800.
  - 0x6e380: each listed target (+0x58) with racer +0x74 > 3, within 500 m and less than 5 m off the line (its offset
    along the drone's right axis) -> every colour whose weapon neither locks nor is a shield is wanted (the special:
    red).
  - 0x6e1e0 (speed 2..4, and the nemesis): locked on (+0xb4) -> the colours holding a shield; 0x6e2b0: energy under 40
    -> the same.
  These read the record one level up (index level x 3 + colour), the same weapon in the data.
- Trigger: red / blue clocks (+0x17f0 / +0x17f4) count while wanted (else 0) and request a shot once past their delay
  (+0x17f8 / +0x17fc, 0 at the start); yellow at once. With the player listed and finished (+0x1800, 0x597314), red
  and blue are dropped. After any request both delays become (rand % 100) x 0.02 + 2.5 (speed 0..1), x 0.01 + 2.4 / 2.2
  / 2.0 (speed 2 / 3 / 4). +0x8ec flags a shot about to be due (not decoded further).

## Drones and the nemesis race
- Kind (craft setup 0x58e40, a jump table per pilot id 0x52cdc0[craft]): +0x9f9 nemesis = the player's pilot (slot 0)
  is this pilot's nemesis (id +- 8); +0x9f8 home circuit = the race's track (0x52ccf8) is the pilot's id. The player's
  craft gets the flags too (unused for it).
- Race setup 0x3f3d0, every craft: the caps red / blue / yellow (one value, 0x52cd0c / 12 / 18), the special cap
  (0x52cd1e) and the Shield level (0x52cd24) by speed class 0x52cd04, nemesis checked first (caps, special cap, shield
  level):

| speed | nemesis | home circuit | general |
|---|---|---|---|
| 0 | 1, 0, 1 | 0, 0, 0 | 0, 0, 0 |
| 1 | 1, 0, 3 | 1, 0, 2 | 0, 0, 1 |
| 2 | 3, 0, 5 | 2, 0, 4 | 1, 0, 3 |
| 3 | 3, 1, 7 | 3, 0, 6 | 2, 0, 5 |
| 4 | 3, 1, 7 | 3, 1, 7 | 3, 0, 6 |

- Then slot 0 (not in split screen, 0x52ccfc != 4) gets the player's Weapon Strength caps (0x3f650) and Shield
  upgrade. So a Novice general drone never holds a weapon.
- Nemesis race (0x791f0, `this` = the game state 0x5293b0): a tournament (+0x394c = 0) at Amateur / Expert with 3 races
  won (+0x1bb0f4 progress), Master with 4, Redshift with 0. Craft 1 (+0x794, the grid's nemesis slot) then takes half
  of a hit's shield damage (0x6b085, after the leech and the Infinite Shield check, only when positive), half the
  corrode (0x6d313) and half its shield launch recharge (0x6a439); the player's damage-dealt stat counts hits on it
  twice (0x6b11c). Other callers (0x7915c, 0x79273, 0x8068a..0x818b7, 0xe139b) are the grid, results and AI.
