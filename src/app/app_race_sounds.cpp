// Argon: the App's side of the race's sounds (RaceSounds, race_sounds.hpp): what they ask of the race, the
// listener's filter by the camera, a launch's travel loop key.
#include "app/app.hpp"

RaceSounds::RaceState App::raceSoundState() const
{
    return RaceSounds::RaceState{multiplayer.players(), session.raceEnd == kRacing, replayRace.playing(), raceHeld(), simClock};
}

// camera_set_eye 0xae640 / 0xc8500: the underwater filter while the eye is under the water, else the internal camera's.
// (sound_frame_update 0xc4f35 also picks a reverb preset by the player's AI node, which no voice feeds: docs\audio\README.md.)
void App::selectListenerFilter()
{
    argon::WorldEffects::Filter filter = driveView.internalTripod() ? argon::WorldEffects::Filter::Internal : argon::WorldEffects::Filter::None;
    if (cameraUnderWater())
    {
        filter = argon::WorldEffects::Filter::Underwater;
    }
    audio.selectListenerFilter(filter);
}

