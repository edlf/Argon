# Sequences: argon's port

## Argon
- `src\movie\nms_sequence.cpp` / `nms_reader.cpp` (reading and playing), `src\movie\story_scenes.cpp` (which scene),
  `src\movie\speech_lines.cpp` (the table 0x1a8768, generated from the XBE), `src\model\cms_animation.cpp`,
  `src\movie\story_scene.cpp` (the scene: models, speech through the movie voice, the overlay),
  `src\movie\head_turn.cpp` / `src\model\bone_aim.cpp` (head turns), `src\movie\scene_light.cpp` (light 1).
- `src\app\app_sequence.cpp`: `<t>_glory.nms` once the loading screen has loaded the track, under its text until A.
- Tournament races (ui\loader.cpp): "Loading..." while the track loads, then the scene (A / START skips) and its
  12-frame hold (shell steps), then the loading screen with the flyby. A missing scene file (only with a damaged disc):
  the loading screen as in the other modes. The race end (App::quitRaceTo -> playRaceEndScene): the scene over the race's track, then FE
  MAIN / FE AWARDS.
- Test flags: `--tourney-race <level> --tourney-done <n> --story-at <t>` (the scene's clock from t),
  `--tourney-race 3 --tourney-done 4 --race-go --race-end-scene` (the tournament won, the race left).
- Not ported: the shadows / sprites of 0x98c30's environment mode (0x439f62, the editor's); whether light 1 stays on
  into the race after a pre-race scene (the leave 0x99fb0 not traced; argon turns it off). The subtitle font
  (font 0 at 1.0 here) and line spacing are not checked against the game. The pilots keep alpha reference 0.5 as the
  front end's characters.

Not an NMS sequence: the front end's idle "movie" (attract demo) is a replay of `d:\replays\replay1..3` through
the event buffer; see [frontend/intro-title.md](../frontend/intro-title.md) "Attract demo".
